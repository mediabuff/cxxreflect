//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/WinRTIntegration.hpp"

#ifdef CXXREFLECT_ENABLE_FEATURE_WINRT

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Type.hpp"

#include <hstring.h>
#include <inspectable.h>
#include <rometadataresolution.h>
#include <windows.h>
#include <winstring.h>

namespace CxxReflect { namespace { namespace Private {

    // TODO These statics require initialization on the main thread, otherwise there may be an 
    // initialization race.  We should synchronize access to them during initialization.
    bool                                         _globalWinRTUniverseInitializationStarted;
    std::future<std::unique_ptr<MetadataLoader>> _globalWinRTUniverseFuture;
    std::unique_ptr<MetadataLoader>              _globalWinRTUniverse;

    MetadataLoader& GetGlobalWinRTUniverse()
    {
        if (_globalWinRTUniverse == nullptr)
            _globalWinRTUniverse = _globalWinRTUniverseFuture.get();

        return *_globalWinRTUniverse;
    }

    // An RAII wrapper around the HSTRING type, providing a container-like interface to the HSTRING.
    class SmartHString
    {
    public:

        typedef wchar_t           value_type;
        typedef std::size_t       size_type;
        typedef std::ptrdiff_t    difference_type;

        typedef value_type const& reference;
        typedef value_type const& const_reference;
        typedef value_type const* pointer;
        typedef value_type const* const_pointer;

        typedef pointer                               iterator;
        typedef const_pointer                         const_iterator;
        typedef std::reverse_iterator<iterator>       reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        SmartHString()
        {
        }

        SmartHString(const_pointer const s)
        {
            Detail::ThrowOnFailure(::WindowsCreateString(s, static_cast<UINT32>(::wcslen(s)), &_value.Get()));
        }

        SmartHString(StringReference const s)
        {
            Detail::ThrowOnFailure(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
        }

        SmartHString(String const& s)
        {
            Detail::ThrowOnFailure(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
        }

        SmartHString(SmartHString const& other)
        {
            Detail::ThrowOnFailure(::WindowsDuplicateString(other._value.Get(), &_value.Get()));
        }

        SmartHString& operator=(SmartHString other)
        {
            swap(other);
            return *this;
        }

        ~SmartHString()
        {
            Detail::VerifySuccess(::WindowsDeleteString(_value.Get()));
        }

        void swap(SmartHString& other)
        {
            using std::swap;
            swap(_value, other._value);
        }

        const_iterator begin()  const { return get_buffer_begin(); }
        const_iterator end()    const { return get_buffer_end();   }
        const_iterator cbegin() const { return get_buffer_begin(); }
        const_iterator cend()   const { return get_buffer_end();   }

        const_reverse_iterator rbegin()  const { return reverse_iterator(get_buffer_end());    }
        const_reverse_iterator rend()    const { return reverse_iterator(get_buffer_begin());  }

        const_reverse_iterator crbegin() const { return reverse_iterator(get_buffer_end());    }
        const_reverse_iterator crend()   const { return reverse_iterator(get_buffer_begin());  }

        size_type size()     const { return end() - begin();                         }
        size_type length()   const { return size();                                  }
        size_type max_size() const { return std::numeric_limits<std::size_t>::max(); }
        size_type capacity() const { return size();                                  }
        bool      empty()    const { return size() == 0;                             }

        const_reference operator[](size_type const n) const
        {
            return get_buffer_begin()[n];
        }

        const_reference at(size_type const n) const
        {
            if (n >= size())
                throw std::out_of_range("n");

            return get_buffer_begin()[n];
        }

        const_reference front() const { return *get_buffer_begin();     }
        const_reference back()  const { return *(get_buffer_end() - 1); }

        const_pointer c_str() const { return get_buffer_begin(); }
        const_pointer data()  const { return get_buffer_begin(); }

        class ReferenceProxy
        {
        public:

            ReferenceProxy(SmartHString* const value)
                : _value(value), _proxy(value->_value.Get())
            {
                Detail::VerifyNotNull(value);
            }

            ~ReferenceProxy()
            {
                if (_value.Get()->_value.Get() == _proxy.Get())
                    return;

                SmartHString newString;
                newString._value.Get() = _proxy.Get();
                
                _value.Get()->swap(newString);
            }

            operator HSTRING*()
            {
                return &_proxy.Get();
            }

        private:

            // Note that this type is copyable though it is not intended to be copied, aside from
            // when it is returned from SmartHString::proxy().
            ReferenceProxy& operator=(ReferenceProxy const&);

            Detail::ValueInitialized<HSTRING>       _proxy;
            Detail::ValueInitialized<SmartHString*> _value;
        };

        ReferenceProxy proxy()
        {
            return ReferenceProxy(this);
        }

        HSTRING value() const
        {
            return _value.Get();
        }

        friend bool operator==(SmartHString const& lhs, SmartHString const& rhs)
        {
            return compare(lhs, rhs) ==  0;
        }

        friend bool operator< (SmartHString const& lhs, SmartHString const& rhs)
        {
            return compare(lhs, rhs) == -1;
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(SmartHString)

    private:

        const_pointer get_buffer_begin() const
        {
            const_pointer const result(::WindowsGetStringRawBuffer(_value.Get(), nullptr));
            return result == nullptr ? L"" : result;
        }

        const_pointer get_buffer_end() const
        {
            std::uint32_t length(0);
            const_pointer const first(::WindowsGetStringRawBuffer(_value.Get(), &length));
            return first + length;
        }

        static int compare(SmartHString const& lhs, SmartHString const& rhs)
        {
            std::int32_t result(0);
            Detail::ThrowOnFailure(::WindowsCompareStringOrdinal(lhs._value.Get(), rhs._value.Get(), &result));
            return result;
        }

        Detail::ValueInitialized<HSTRING> _value;
    };

    String ToString(HSTRING hstring)
    {
        wchar_t const* const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
        return buffer == nullptr ? L"" : buffer;
    }

    // An RAII wrapper for an array of HSTRINGs.
    class RaiiHStringArray
    {
    public:

        RaiiHStringArray()
        {
        }

        ~RaiiHStringArray()
        {
            Detail::Verify([&]{ return _count.Get() == 0 || _array.Get() != nullptr; });

            // Exception-safety note:  if the deletion fails, something has gone horribly wrong
            for (DWORD i(0); i < _count.Get(); ++i)
                Detail::VerifySuccess(::WindowsDeleteString(_array.Get()[i]));

            ::CoTaskMemFree(_array.Get());
        }

        DWORD&    GetCount() { return _count.Get(); }
        HSTRING*& GetArray() { return _array.Get(); }

        HSTRING*  begin() const { return _array.Get();                }
        HSTRING*  end()   const { return _array.Get() + _count.Get(); }

    private:

        RaiiHStringArray(RaiiHStringArray const&);
        RaiiHStringArray& operator=(RaiiHStringArray const&);

        Detail::ValueInitialized<DWORD>    _count;
        Detail::ValueInitialized<HSTRING*> _array;
    };

    // Contains logic for invoking a virtual function on an object via vtable lookup.  Currently this
    // only supports functions that take no arguments or that take one or two reference-type arguments.
    // TODO Add support for value-type arguments.  This may or may not be very easy.
    class Invoker
    {
    public:

        // Note that each type has an extra argument for the 'this' pointer:
        typedef HRESULT (__stdcall* ReferenceOnly0Args)(void*);
        typedef HRESULT (__stdcall* ReferenceOnly1Args)(void*, void*);
        typedef HRESULT (__stdcall* ReferenceOnly2Args)(void*, void*, void*);

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::VerifyNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly0Args>(index, correctThis.Get()));
            Detail::VerifyNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get());
        }

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis,
                                                     void*     const  arg0)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::VerifyNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly1Args>(index, correctThis.Get()));
            Detail::VerifyNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get(), arg0);
        }

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis,
                                                     void*     const  arg0,
                                                     void*     const  arg1)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::VerifyNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly2Args>(index, correctThis.Get()));
            Detail::VerifyNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get(), arg0, arg1);
        }

    private:

        // A smart QI'ing pointer for finding and owning the 'this' pointer for the Invoke functions.
        class ThisPointer
        {
        public:

            ThisPointer(IID const& interfaceId, IUnknown* const unknownThis)
            {
                Detail::VerifyNotNull(unknownThis);

                // Note that we cannot direct-initialize here due to a bug in the Visual C++ compiler.
                void** const voidUnknown = reinterpret_cast<void**>(&_unknown.Get());
                Detail::ThrowOnFailure(unknownThis->QueryInterface(interfaceId, voidUnknown));
                Detail::VerifyNotNull(_unknown.Get());
            }

            ~ThisPointer()
            {
                Detail::VerifyNotNull(_unknown.Get());
                _unknown.Get()->Release();
            }

            IUnknown* Get() const
            {
                return _unknown.Get();
            }

        private:

            ThisPointer(ThisPointer const&);
            ThisPointer& operator=(ThisPointer const&);

            Detail::ValueInitialized<IUnknown*> _unknown;
        };

        Invoker(Invoker const&);
        Invoker& operator=(Invoker&);

        // Computes a function pointer, given the type of the function pointer (TFunctionPointer),
        // the 'this' pointer of the object on which the function will be invoked, and the index of
        // the function in the vtable.  The 'this' pointer must point to the correct vtable (i.e., 
        // before calling this, you must be sure to QueryInterface to get the right 'this' pointer.
        template <typename TFunctionPointer>
        static TFunctionPointer ComputeFunctionPointer(unsigned const index, void* const thisptr)
        {
            Detail::VerifyNotNull(thisptr);

            // There are three levels of indirection:  'this' points to an object, the first element
            // of which points to a vtable, which contains slots that point to functions:
            return reinterpret_cast<TFunctionPointer>((*reinterpret_cast<void***>(thisptr))[index]);
        }
    };

    void EnumerateUniverseMetadataFilesInto(SmartHString const rootNamespace, std::vector<String>& result)
    {
        RaiiHStringArray filePaths;
        RaiiHStringArray nestedNamespaces;

        Detail::ThrowOnFailure(::RoResolveNamespace(
            rootNamespace.value(),
            nullptr,
            0,
            nullptr,
            rootNamespace.empty() ? nullptr : &filePaths.GetCount(),
            rootNamespace.empty() ? nullptr : &filePaths.GetArray(),
            &nestedNamespaces.GetCount(),
            &nestedNamespaces.GetArray()));

        std::transform(filePaths.begin(), filePaths.end(), std::back_inserter(result), [&](HSTRING path)
        {
            return ToString(path);
        });

        String const baseNamespace(String(rootNamespace.c_str()) + (rootNamespace.empty() ? L"" : L"."));
        std::for_each(nestedNamespaces.begin(), nestedNamespaces.end(), [&](HSTRING nestedNamespace)
        {
            EnumerateUniverseMetadataFilesInto(baseNamespace + ToString(nestedNamespace), result);
        });
    }

    std::vector<String> EnumerateUniverseMetadataFiles()
    {
        std::vector<String> result;

        EnumerateUniverseMetadataFilesInto(SmartHString(), result);

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

    StringReference RemoveRightmostTypeNameComponent(StringReference const typeName)
    {
        Detail::Verify([&]{ return !typeName.empty(); });

        return StringReference(
            typeName.begin(),
            std::find(typeName.rbegin(), typeName.rend(), L'.').base());
    }

    void RemoveRightmostTypeNameComponent(String& typeName)
    {
        Detail::Verify([&]{ return !typeName.empty(); });

        // TODO This does not handle generics.  Does it need to handle generics?
        typeName.erase(std::find(typeName.rbegin(), typeName.rend(), L'.').base(), typeName.end());
    }

} } }

namespace CxxReflect {

    WinRTMetadataResolver::WinRTMetadataResolver(String const& packageRoot)
        : _packageRoot(packageRoot)
    {
        auto const metadataFiles(Private::EnumerateUniverseMetadataFiles());

        std::transform(metadataFiles.begin(),
                       metadataFiles.end(),
                       std::inserter(_metadataFiles, _metadataFiles.end()),
                       [&](String const& fileName) -> PathMap::value_type
        {
            // TODO This code requires error checking and is really, really hacked up.
            auto first(std::find(fileName.rbegin(), fileName.rend(), L'\\').base());
            auto last(std::find(fileName.rbegin(), fileName.rend(), L'.').base());

            String const simpleName(first, std::prev(last));

            return std::make_pair(Detail::MakeLowercase(simpleName), Detail::MakeLowercase(fileName));
        });
    }

    String WinRTMetadataResolver::ResolveAssembly(AssemblyName const& assemblyName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + L"CxxReflectPlatform.winmd";
        }

        Detail::VerifyFail("Not Yet Implemented");
        return String();
    }

    String WinRTMetadataResolver::ResolveAssembly(AssemblyName const& assemblyName,
                                                  String       const& namespaceQualifiedTypeName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + L"CxxReflectPlatform.winmd";
        }

        // The assembly name must be a prefix of the namespace-qualified type name, per WinRT rules:
        Detail::Verify([&]() -> bool
        {
            String const lowercaseNamespaceQualifiedTypeName(Detail::MakeLowercase(namespaceQualifiedTypeName));
            return simpleName.size() <= namespaceQualifiedTypeName.size()
                && std::equal(simpleName.begin(), simpleName.end(), lowercaseNamespaceQualifiedTypeName.begin());
        });

        String namespaceName(namespaceQualifiedTypeName);
        Private::RemoveRightmostTypeNameComponent(namespaceName);
        return FindMetadataFileForNamespace(namespaceName);
    }

    WinRTMetadataResolver::PathMap::const_iterator WinRTMetadataResolver::BeginMetadataFiles() const
    {
        return _metadataFiles.begin();
    }

    WinRTMetadataResolver::PathMap::const_iterator WinRTMetadataResolver::EndMetadataFiles() const
    {
        return _metadataFiles.end();
    }

    String WinRTMetadataResolver::FindMetadataFileForNamespace(String const& namespaceName) const
    {
        String const lowercaseNamespaceName(Detail::MakeLowercase(namespaceName));

        // First, search the metadata files we got from RoResolveNamespace:
        String enclosingNamespaceName(lowercaseNamespaceName);
        while (!enclosingNamespaceName.empty())
        {
            auto const it(_metadataFiles.find(enclosingNamespaceName));
            if (it != _metadataFiles.end())
                return it->second;

            Private::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
        }

        // Next, search for metadata files in the package root, using the longest-match-wins rule:
        // TODO We may need to support folders other than the package root.  For example, how do we
        // know where to find the Platform.winmd metadata?
        enclosingNamespaceName = lowercaseNamespaceName;
        while (!enclosingNamespaceName.empty())
        {
            String winmdPath = _packageRoot + enclosingNamespaceName + L".winmd";
            if (Detail::FileExists(winmdPath.c_str()))
            {
                _metadataFiles.insert(std::make_pair(enclosingNamespaceName, winmdPath));
                return winmdPath;
            }

            Private::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
        }

        // If this is the 'Platform' or 'System' namespace, try to use the platform metadata file:
        // TODO This is also suspect:  how do we know to look here?  :'(
        if (lowercaseNamespaceName.substr(0, 8) == L"platform" ||
            lowercaseNamespaceName.substr(0, 6) == L"system")
        {
            return _packageRoot + L"CxxReflectPlatform.winmd";
        }

        // TODO Should we throw here or return an empty string?
        throw RuntimeError("Failed to locate metadata file");
    }

    void WinRTPackageMetadata::BeginInitialization(String const& platformMetadataPath)
    {
        if (Private::_globalWinRTUniverseInitializationStarted)
            return;

        Private::_globalWinRTUniverseFuture = std::async(std::launch::async, [=]() -> std::unique_ptr<MetadataLoader>
        {
            std::unique_ptr<WinRTMetadataResolver> resolver(new WinRTMetadataResolver(platformMetadataPath));
            WinRTMetadataResolver const& rawResolver(*resolver.get());

            std::unique_ptr<MetadataLoader> loader(new MetadataLoader(std::move(resolver)));

            typedef WinRTMetadataResolver::PathMap::value_type Element;
            std::for_each(rawResolver.BeginMetadataFiles(), rawResolver.EndMetadataFiles(), [&](Element const& e)
            {
                loader->LoadAssembly(e.second);
            });

            return loader;
        });
    }

    bool WinRTPackageMetadata::HasInitializationBegun()
    {
        return Private::_globalWinRTUniverseInitializationStarted;
    }

    bool WinRTPackageMetadata::IsInitialized()
    {
        return Private::_globalWinRTUniverseFuture.valid() || Private::_globalWinRTUniverse != nullptr;
    }

    Type WinRTPackageMetadata::GetTypeOf(IInspectable* const inspectable)
    {
        // TODO CHANGE TO USE StringReference EVENTUALLY
        Detail::VerifyNotNull(inspectable);

        Private::SmartHString typeNameHString;
        Detail::VerifySuccess(inspectable->GetRuntimeClassName(typeNameHString.proxy()));
        String const typeName(typeNameHString.begin(), typeNameHString.end());

        auto const endOfNamespaceIt(std::find(typeName.rbegin(), typeName.rend(), '.').base());
        Detail::Verify([&]{ return endOfNamespaceIt != typeName.rend().base(); });

        String const namespaceName(typeName.begin(), std::prev(endOfNamespaceIt));
        
        IMetadataResolver const& resolver(Private::GetGlobalWinRTUniverse().GetResolver());
        WinRTMetadataResolver const& winrtResolver(dynamic_cast<WinRTMetadataResolver const&>(resolver));

        String metadataFileName(winrtResolver.FindMetadataFileForNamespace(namespaceName));
        if (metadataFileName.empty())
            return Type();

        Assembly assembly(Private::GetGlobalWinRTUniverse().LoadAssembly(metadataFileName));
        if (!assembly.IsInitialized())
            return Type();

        return assembly.GetType(StringReference(typeName.c_str()));
    }
}

#endif
