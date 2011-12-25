//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Property::Property()
    {
    }

    Property::Property(Type const& reflectedType, Detail::PropertyContext const* const context, InternalKey)
        : _reflectedType(reflectedType),
          _context(context)
    {
        Detail::VerifyNotNull(context);
        Detail::Verify([&]{ return reflectedType.IsInitialized(); });
        Detail::Verify([&]{ return context->IsInitialized();      });
    }

    bool Property::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _context.Get() != nullptr;
    }

    bool Property::operator!() const
    {
        return !IsInitialized();
    }

    void Property::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    Detail::PropertyContext const& Property::GetContext(InternalKey) const
    {
        VerifyInitialized();
        return *_context.Get();
    }

    Type Property::GetDeclaringType() const
    {
        VerifyInitialized();
        MetadataLoader          const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_context.Get()->GetDeclaringType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetDeclaringType().AsRowReference(), InternalKey());
    }

    Type Property::GetReflectedType() const
    {
        VerifyInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Property const& lhs, Property const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
    }

    bool operator<(Property const& lhs, Property const& rhs)
    {
        return std::less<Detail::PropertyContext const*>()(lhs._context.Get(), rhs._context.Get());
    }

}
