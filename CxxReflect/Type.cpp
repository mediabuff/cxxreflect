//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/MetadataSignature.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace { namespace Private {

    using namespace CxxReflect;

    bool IsSystemAssembly(Assembly const& assembly)
    {
        // The system assembly has no assembly references; it is usually mscorlib.dll, but it could
        // be named something else (e.g., Platform.winmd in WinRT?)
        return assembly.GetReferencedAssemblyCount() == 0;
    }

    bool IsSystemType(Type const& type,
                      StringReference const& typeNamespace,
                      StringReference const& typeName)
    {
        return IsSystemAssembly(type.GetAssembly())
            && type.GetNamespace() == typeNamespace
            && type.GetName() == typeName;
    }

    bool IsDerivedFromSystemType(Type const& type,
                                 StringReference const& typeNamespace,
                                 StringReference const& typeName,
                                 bool const includeSelf)
    {
        Type currentType(type);
        if (!includeSelf && currentType)
        {
            currentType = type.GetBaseType();
        }

        while (currentType)
        {
            if (IsSystemType(currentType, typeNamespace, typeName))
            {
                return true;
            }

            currentType = currentType.GetBaseType();
        }

        return false;
    }

} }

namespace CxxReflect {

    bool const TodoNotYetImplementedFlag = false;

    Type::Type()
    {
    }

    Type::Type(Assembly const& assembly, Metadata::TableReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::TableOrBlobReference(type))
    {
        Detail::Verify([&] { return assembly.IsInitialized(); });

        // If we were initialized with an empty type, do not attempt to do any type resolution.
        if (!type.IsInitialized())
            return;

        switch (type.GetTable())
        {
        case Metadata::TableId::TypeDef:
        {
            // Wonderful news!  We have a TypeDef and we don't need to do any further work.
            break;
        }

        case Metadata::TableId::TypeRef:
        {
            // Resolve the TypeRef into a TypeDef, throwing on failure:
            MetadataLoader const& loader(assembly.GetContext(InternalKey()).GetLoader());
            Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

            Metadata::DatabaseReference const resolvedType(
                loader.ResolveType(Metadata::DatabaseReference(&database, type), InternalKey()));

            Detail::Verify([&]{ return resolvedType.IsInitialized(); });

            _assembly = Assembly(
                &loader.GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()),
                InternalKey());

            _type = Metadata::TableOrBlobReference(resolvedType.GetTableReference());
            Detail::Verify([&]{ return _type.AsTableReference().GetTable() == Metadata::TableId::TypeDef; });

            break;
        }

        case Metadata::TableId::TypeSpec:
        {
            // Get the signature for the TypeSpec token and use that instead:
            Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());
            Metadata::TypeSpecRow const typeSpec(database.GetRow<Metadata::TableId::TypeSpec>(type.GetIndex()));
            _type = Metadata::TableOrBlobReference(typeSpec.GetSignature());

            break;
        }

        default:
        {
            Detail::VerifyFail();
            break;
        }
        }
    }

    Type::Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::TableOrBlobReference(type))
    {
        Detail::Verify([&] { return assembly.IsInitialized(); });

        Metadata::TypeSignature const signature(assembly
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(type)
            .As<Metadata::TypeSignature>());

        if (signature.GetKind() == Metadata::TypeSignature::Kind::Primitive)
        {
            String primitiveTypeName;
            switch (signature.GetPrimitiveElementType())
            {
            case Metadata::ElementType::Boolean: primitiveTypeName = L"System.Boolean"; break;
            case Metadata::ElementType::Char:    primitiveTypeName = L"System.Char";    break;
            case Metadata::ElementType::I1:      primitiveTypeName = L"System.SByte";   break;
            case Metadata::ElementType::U1:      primitiveTypeName = L"System.Byte";    break;
            case Metadata::ElementType::I2:      primitiveTypeName = L"System.Int16";   break;
            case Metadata::ElementType::U2:      primitiveTypeName = L"System.UInt16";  break;
            case Metadata::ElementType::I4:      primitiveTypeName = L"System.Int32";   break;
            case Metadata::ElementType::U4:      primitiveTypeName = L"System.UInt32";  break;
            case Metadata::ElementType::I8:      primitiveTypeName = L"System.Int64";   break;
            case Metadata::ElementType::U8:      primitiveTypeName = L"System.UInt64";  break;
            case Metadata::ElementType::R4:      primitiveTypeName = L"System.Short";   break;
            case Metadata::ElementType::R8:      primitiveTypeName = L"System.Double";  break;
            case Metadata::ElementType::I:       primitiveTypeName = L"System.IntPtr";  break;
            case Metadata::ElementType::U:       primitiveTypeName = L"System.UIntPtr"; break;
            case Metadata::ElementType::Object:  primitiveTypeName = L"System.Object";  break;
            case Metadata::ElementType::String:  primitiveTypeName = L"System.String";  break;
            case Metadata::ElementType::Void:    primitiveTypeName = L"System.Void";    break;

            case Metadata::ElementType::TypedByRef:
            default:
                Detail::VerifyFail("Not Yet Implemented");
                break;
            }

            // Find the system assembly:
            Type systemObject(*assembly.BeginTypes());
            while (systemObject.GetBaseType())
                systemObject = systemObject.GetBaseType();

            Type const primitiveType(systemObject.GetAssembly().GetType(StringReference(primitiveTypeName.c_str())));

            _assembly = primitiveType.GetAssembly();
            _type = Metadata::TableReference::FromToken(primitiveType.GetMetadataToken());
        }
    }

    bool Type::AccumulateFullNameInto(OutputStream& os) const
    {
        // TODO ENSURE WE ESCAPE THE TYPE NAME CORRECTLY

        if (IsTypeDef())
        {
            if (IsNested())
            {
                GetDeclaringType().AccumulateFullNameInto(os);
                os << L'+' << GetName();
            }
            else if (GetNamespace().size() > 1) // TODO REMOVE NULL TERMINATOR FROM COUNT
            {
                os << GetNamespace() << L'.' << GetName();
            }
            else
            {
                os << GetName();
            }
        }
        else
        {
            Metadata::TypeSignature const signature(GetTypeSpecSignature());

            // A TypeSpec for an uninstantiated generic type has no name:
            if (Metadata::ClassVariableSignatureInstantiator::RequiresInstantiation(signature))
                return false;

            switch (signature.GetKind())
            {
            case Metadata::TypeSignature::Kind::GenericInst:
            {
                if (std::any_of(signature.BeginGenericArguments(), signature.EndGenericArguments(), [&](Metadata::TypeSignature const& sig)
                {
                    return sig.GetKind() == Metadata::TypeSignature::Kind::Var;
                }))
                {
                    return false;
                }

                Type const genericType(_assembly, signature.GetGenericTypeReference(), InternalKey());
                genericType.AccumulateFullNameInto(os);

                os << L'[';
                std::for_each(
                    signature.BeginGenericArguments(),
                    signature.EndGenericArguments(),
                    [&](Metadata::TypeSignature const& argumentSignature)
                {
                    os << L'[';
                    Type const argumentType(
                        _assembly,
                        Metadata::BlobReference(
                            argumentSignature.BeginBytes() - _assembly.GetContext(InternalKey()).GetDatabase().GetBlobs().Begin(),
                            argumentSignature.EndBytes() - argumentSignature.BeginBytes()), InternalKey());
                    argumentType.AccumulateAssemblyQualifiedNameInto(os);
                    os << L']';
                });
                os << L']';
                break;
            }
            case Metadata::TypeSignature::Kind::ClassType:
            {
                Type const classType(_assembly, signature.GetTypeReference(), InternalKey());
                classType.AccumulateFullNameInto(os);
                break;
            }
            case Metadata::TypeSignature::Kind::SzArray:
            {
                Type const classType(
                    _assembly,
                    Metadata::BlobReference(
                        signature.GetArrayType().BeginBytes() - _assembly.GetContext(InternalKey()).GetDatabase().GetBlobs().Begin(),
                        signature.GetArrayType().EndBytes() - signature.BeginBytes()),
                    InternalKey());

                classType.AccumulateFullNameInto(os);
                os << L"[]";
                break;
            }
            case Metadata::TypeSignature::Kind::Var:
            {
                // TODO MVAR?
                os << L"Var!" << signature.GetVariableNumber();
                break;
            }
            default:
            {
                Detail::VerifyFail("Not yet implemented");
                break;
            }
            }
        }

        // TODO TYPESPEC SUPPORT
        return true;
    }

    void Type::AccumulateAssemblyQualifiedNameInto(OutputStream& os) const
    {
        if (AccumulateFullNameInto(os))
        {
            os << L", " << _assembly.GetName().GetFullName();
        }
    }

    Metadata::TypeDefRow Type::GetTypeDefRow() const
    {
        Detail::Verify([&]{ return IsTypeDef(); });

        return _assembly
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeDef>(_type.AsTableReference().GetIndex());
    }

    Metadata::TypeSignature Type::GetTypeSpecSignature() const
    {
        Detail::Verify([&]{ return IsTypeSpec(); });

        return _assembly
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(_type.AsBlobReference())
            .As<Metadata::TypeSignature>();
    }

    Detail::MethodTableAllocator::Range Type::GetOrCreateMethodTable() const
    {
        // TODO We need to handle TypeSpec Type objects here.
        typedef Detail::MethodTableAllocator::Range Range;

        Detail::AssemblyContext const& context(_assembly.GetContext(InternalKey()));
        Range const existingRange(context.GetMethodTableForType(_type.AsTableReference().GetIndex()));

        if (existingRange.IsInitialized())
            return existingRange;

        std::vector<Detail::MethodReference> methods;
        return existingRange; // TODO
    }

    Type::MethodIterator Type::BeginMethods(BindingFlags flags) const
    {
        // TODO TYPESPEC SUPPORT
        Metadata::TypeDefRow const typeDef(GetTypeDefRow());
        return MethodIterator(*this, typeDef.GetFirstMethod(), typeDef.GetLastMethod(), flags);
    }

    Type::MethodIterator Type::EndMethods() const
    {
        return MethodIterator();
    }

    Type::NextMethodScopeResult Type::InternalNextMethodScope(Type const& currentScope)
    {
        Type const baseType(currentScope.GetBaseType());
        if (!baseType || !baseType.IsTypeDef()) // TODO HANDLE TYPESPECS
        {
            return NextMethodScopeResult();
        }

        return NextMethodScopeResult(
            baseType,
            baseType.GetTypeDefRow().GetFirstMethod(),
            baseType.GetTypeDefRow().GetLastMethod());
    }

    bool Type::InternalFilterMethod(Method const& method, BindingFlags const& flags)
    {
        // Constructors are never returned during method iteration
        if (method.GetName() == L".ctor" || method.GetName() == L".cctor")
            return false;

        if (method.GetAttributes().WithMask(MethodAttribute::MemberAccessMask) != MethodAttribute::Public &&
            !flags.IsSet(BindingAttribute::NonPublic))
            return false;

        if (method.GetDeclaringType() != method.GetReflectedType())
        {
            MethodFlags const attributes(method.GetAttributes());
            //if (attributes.IsSet(MethodAttribute::Static))
            //    return false;

            //if (attributes.IsSet(MethodAttribute::Virtual) &&
            //    attributes.WithMask(MethodAttribute::VTableLayoutMask) == MethodAttribute::ReuseSlot)
            //    return false;

            if (attributes.WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Private)
                return false;
        }

        return true;
    }

    Type Type::GetBaseType() const
    {
        return ResolveTypeDefTypeAndCall([&](Type const& t) -> Type
        {
            Metadata::TableReference extends(t.GetTypeDefRow().GetExtends());
            if (!extends.IsValid())
                return Type();

            switch (extends.GetTable())
            {
            case Metadata::TableId::TypeDef:
            case Metadata::TableId::TypeRef:
            case Metadata::TableId::TypeSpec:
                return Type(_assembly, extends, InternalKey());

            default:
                throw std::logic_error("wtf");
            }
        });
    }

    Type Type::GetDeclaringType() const
    {
        if (IsNested())
        {
            Metadata::Database const& database(_assembly.GetContext(InternalKey()).GetDatabase());
            auto const it(std::lower_bound(
                database.Begin<Metadata::TableId::NestedClass>(),
                database.End<Metadata::TableId::NestedClass>(),
                _type,
                [](Metadata::NestedClassRow const& r, Metadata::TableOrBlobReference const index)
            {
                return r.GetNestedClass() < index;
            }));

            if (it == database.End<Metadata::TableId::NestedClass>() || it->GetNestedClass() != _type)
                throw std::logic_error("wtf");

            // TODO IS THE TYPE DEF CHECK DONE AT THE PHYSICAL LAYER?
            Metadata::TableReference const enclosingType(it->GetEnclosingClass());
            if (enclosingType.GetTable() != Metadata::TableId::TypeDef)
                throw std::logic_error("wtf");

            return Type(_assembly, enclosingType, InternalKey());
        }

        // TODO OTHER KINDS OF DECLARING TYPES
        return Type();
    }

    String Type::GetAssemblyQualifiedName() const
    {
        std::wostringstream oss;
        AccumulateAssemblyQualifiedNameInto(oss);
        return oss.str();
    }

    String Type::GetFullName() const
    {
        std::wostringstream oss;
        AccumulateFullNameInto(oss);
        return oss.str();
    }

    StringReference Type::GetName() const
    {
        VerifyInitialized();

        if (IsTypeDef())
            return GetTypeDefRow().GetName();

        return L""; // TODO
    }

    StringReference Type::GetNamespace() const
    {
        if (IsNested())
        {
            return GetDeclaringType().GetNamespace();
        }
        
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetNamespace();
        });
    }

    bool Type::IsAbstract() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Abstract);
        });
    }

    bool Type::IsAnsiClass() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::AnsiClass;
        });
    }

    bool Type::IsArray() const
    {
        VerifyInitialized();
        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsAutoClass() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::AutoClass;
        });
    }

    bool Type::IsAutoLayout() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::AutoLayout;
        });
    }

    bool Type::IsByRef() const
    {
        VerifyInitialized();

        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsClass() const
    {
        VerifyInitialized();

        return !IsInterface() && !IsValueType();
    }

    bool Type::IsComObject() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Private::IsDerivedFromSystemType(t, L"System", L"__ComObject", true);
        });
    }

    bool Type::IsContextful() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Private::IsDerivedFromSystemType(t, L"System", L"ContextBoundObject", true);
        });
    }

    bool Type::IsEnum() const
    {
        if (!IsTypeDef()) { return false; }

        return Private::IsDerivedFromSystemType(*this, L"System", L"Enum", false);
    }

    bool Type::IsExplicitLayout() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask)
                == TypeAttribute::ExplicitLayout;
        });
    }

    bool Type::IsGenericParameter() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsGenericType() const
    {
        // TODO THIS IS WRONG
        if (IsNested() && GetDeclaringType().IsGenericType())
        {
            return true;
        }

        StringReference const name(GetName());
        return std::find(name.begin(), name.end(), L'`') != name.end();
    }

    bool Type::IsGenericTypeDefinition() const
    {
        // TODO THIS IS WRONG
        return IsGenericType();
    }

    bool Type::IsImport() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Import);
        });
    }

    bool Type::IsInterface() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::ClassSemanticsMask)
                == TypeAttribute::Interface;
        });
    }

    bool Type::IsLayoutSequential() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask)
                == TypeAttribute::SequentialLayout;
        });
    }

    bool Type::IsMarshalByRef() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Private::IsDerivedFromSystemType(t, L"System", L"MarshalByRefObject", true);
        });
    }

    bool Type::IsNested() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                >  TypeAttribute::Public;
        });
    }

    bool Type::IsNestedAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedAssembly;
        });
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyAndAssembly;
        });

    }
    bool Type::IsNestedFamily() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamily;
        });
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyOrAssembly;
        });
    }

    bool Type::IsNestedPrivate() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPrivate;
        });
    }

    bool Type::IsNestedPublic() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPublic;
        });
    }

    bool Type::IsNotPublic() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NotPublic;
        });
    }

    bool Type::IsPointer() const
    {
        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsPrimitive() const
    {
        if (!IsTypeDef()) { return false; }

        if (!Private::IsSystemAssembly(_assembly)) { return false; }

        if (GetTypeDefRow().GetNamespace() != L"System") { return false; }

        StringReference const& name(GetTypeDefRow().GetName());
        switch (name[0])
        {
        case L'B': return name == L"Boolean" || name == L"Byte";
        case L'C': return name == L"Char";
        case L'D': return name == L"Double";
        case L'I': return name == L"Int16" || name == L"Int32" || name == L"Int64" || name == L"IntPtr";
        case L'S': return name == L"SByte" || name == L"Single";
        case L'U': return name == L"UInt16" || name == L"UInt32" || name == L"UInt64" || name == L"UIntPtr";
        }

        return false;
    }

    bool Type::IsPublic() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::Public;
        });
    }

    bool Type::IsSealed() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Sealed);
        });
    }

    bool Type::IsSerializable() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Serializable)
                || t.IsEnum()
                || Private::IsDerivedFromSystemType(t, L"System", L"MulticastDelegate", true);
        });
    }

    bool Type::IsSpecialName() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::SpecialName);
        });
    }

    bool Type::IsUnicodeClass() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::UnicodeClass;
        });
    }

    bool Type::IsValueType() const
    {
        return Private::IsDerivedFromSystemType(*this, L"System", L"ValueType", false)
            && !Private::IsSystemType(*this, L"System", L"Enum");
    }

    bool Type::IsVisible() const
    {
        if (IsTypeDef())
        {
            if (IsNested() && !GetDeclaringType().IsVisible())
                return false;

            switch (GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask).GetEnum())
            {
            case TypeAttribute::Public:
            case TypeAttribute::NestedPublic:
                return true;

            default:
                return false;
            }
        }
        // TODO CHECK BEHAVIOR FOR TYPESPECS

        return TodoNotYetImplementedFlag;
    }

}
