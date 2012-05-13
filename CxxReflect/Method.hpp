#ifndef CXXREFLECT_METHOD_HPP_
#define CXXREFLECT_METHOD_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{





    class Method
    {
    public:

        typedef Detail::InstantiatingIterator<
            Detail::ParameterData, Parameter, Method, Detail::IdentityTransformer, std::forward_iterator_tag
        > ParameterIterator;

        Method();

        Type GetDeclaringType() const;
        Type GetReflectedType() const;

        bool              ContainsGenericParameters() const;
        MethodFlags       GetAttributes()             const;
        CallingConvention GetCallingConvention()      const;
        SizeType          GetMetadataToken()          const;
        StringReference   GetName()                   const;

        bool IsAbstract()                const;
        bool IsAssembly()                const;
        bool IsConstructor()             const;
        bool IsFamily()                  const;
        bool IsFamilyAndAssembly()       const;
        bool IsFamilyOrAssembly()        const;
        bool IsFinal()                   const;
        bool IsGenericMethod()           const;
        bool IsGenericMethodDefinition() const;
        bool IsHideBySig()               const;
        bool IsPrivate()                 const;
        bool IsPublic()                  const;
        bool IsSpecialName()             const;
        bool IsStatic()                  const;
        bool IsVirtual()                 const;

        bool IsInitialized()             const;
        bool operator!()                 const;

        CustomAttributeIterator BeginCustomAttributes() const;
        CustomAttributeIterator EndCustomAttributes()   const;

        ParameterIterator BeginParameters()   const;
        ParameterIterator EndParameters()     const;
        SizeType          GetParameterCount() const;

        Parameter GetReturnParameter() const;
        Type      GetReturnType()      const;

        // Module

        // GetBaseDefinition          -- Non-constructor only
        // GetGenericArguments
        // GetGenericMethodDefinition -- Non-constructor only
        // GetMethodBody
        // GetMethodImplementationFlags
        
        // IsDefined
        // MakeGenericMethod          -- Non-constructor only

        // -- The following members of System.Reflection.MethodInfo are not implemented --
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent
        // MemberType
        // MethodHandle
        //
        // Invoke()

        friend bool operator==(Method const&, Method const&);
        friend bool operator< (Method const&, Method const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Method)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Method)

    public: // Internal Members

        Method(Type const& reflectedType, Detail::MethodContext const* context, InternalKey);

        Detail::MethodContext const& GetContext(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::MethodDefRow GetMethodDefRow() const;

        Detail::TypeHandle                                     _reflectedType;
        Detail::ValueInitialized<Detail::MethodContext const*> _context;
    };

    /// @}

}

#endif
