//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Field::Field()
    {
    }

    Field::Field(Type const& reflectedType, Detail::FieldContext const* const context, InternalKey)
        : _reflectedType(reflectedType),
          _context(context)
    {
        Detail::VerifyNotNull(context);
        Detail::Verify([&]{ return reflectedType.IsInitialized(); });
        Detail::Verify([&]{ return context->IsInitialized();      });
    }

    bool Field::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _context.Get() != nullptr;
    }

    bool Field::operator!() const
    {
        return !IsInitialized();
    }

    void Field::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    Detail::FieldContext const& Field::GetContext(InternalKey) const
    {
        VerifyInitialized();
        return *_context.Get();
    }

    Type Field::GetDeclaringType() const
    {
        VerifyInitialized();
        MetadataLoader          const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_context.Get()->GetDeclaringType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetDeclaringType().AsRowReference(), InternalKey());
    }

    Type Field::GetReflectedType() const
    {
        VerifyInitialized();
        return _reflectedType.Realize();
    }

    FieldFlags Field::GetAttributes() const
    {
        return GetContext(InternalKey()).GetMemberRow().GetFlags();
    }

    Type Field::GetFieldType() const
    {
        return Type(
            GetDeclaringType().GetAssembly(),
            GetContext(InternalKey()).GetMemberRow().GetSignature(),
            InternalKey());
    }

    SizeType Field::GetMetadataToken() const
    {
        return GetContext(InternalKey()).GetMemberRow().GetSelfReference().GetToken();
    }

    StringReference Field::GetName() const
    {
        return GetContext(InternalKey()).GetMemberRow().GetName();
    }

    bool Field::IsAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Assembly;
    }

    bool Field::IsFamily() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Family;
    }

    bool Field::IsFamilyAndAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::FamilyAndAssembly;
    }

    bool Field::IsFamilyOrAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::FamilyOrAssembly;
    }

    bool Field::IsInitOnly() const
    {
        return GetAttributes().IsSet(FieldAttribute::InitOnly);
    }

    bool Field::IsLiteral() const
    {
        return GetAttributes().IsSet(FieldAttribute::Literal);
    }

    bool Field::IsNotSerialized() const
    {
        return GetAttributes().IsSet(FieldAttribute::NotSerialized);
    }

    bool Field::IsPinvokeImpl() const
    {
        return GetAttributes().IsSet(FieldAttribute::PInvokeImpl);
    }

    bool Field::IsPrivate() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Private;
    }

    bool Field::IsPublic() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Public;
    }

    bool Field::IsSpecialName() const
    {
        // TODO Do we need to check for RuntimeSpecialName too?
        return GetAttributes().IsSet(FieldAttribute::SpecialName);
    }

    bool Field::IsStatic() const
    {
        return GetAttributes().IsSet(FieldAttribute::Static);
    }

    bool operator==(Field const& lhs, Field const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
    }

    bool operator<(Field const& lhs, Field const& rhs)
    {
        return std::less<Detail::FieldContext const*>()(lhs._context.Get(), rhs._context.Get());
    }

}
