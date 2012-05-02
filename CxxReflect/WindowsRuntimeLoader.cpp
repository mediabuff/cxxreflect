
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeLoader.hpp"
#include "CxxReflect/WindowsRuntimeUtility.hpp"


namespace CxxReflect { namespace WindowsRuntime {

    PackageAssemblyLocator::PackageAssemblyLocator(String const& packageRoot)
        : _packageRoot(packageRoot)
    {
        std::vector<String> const metadataFiles(Internal::EnumeratePackageMetadataFiles(packageRoot.c_str()));

        std::transform(begin(metadataFiles), end(metadataFiles),
                       std::inserter(_metadataFiles, _metadataFiles.end()),
                       [](String const& fileName) -> PathMap::value_type
        {
            // TODO We definitely need error checking here.
            auto first(std::find(fileName.rbegin(), fileName.rend(), L'\\').base());
            auto last (std::find(fileName.rbegin(), fileName.rend(), L'.' ).base());

            String const simpleName(first, std::prev(last));

            return std::make_pair(Detail::MakeLowercase(simpleName), Detail::MakeLowercase(fileName));
        });
    }

    AssemblyLocation PackageAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // We special-case mscorlib and platform to point to our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return AssemblyLocation(ConstByteRange(
                Detail::BeginWinRTTypeSystemSupportEmbedded(),
                Detail::EndWinRTTypeSystemSupportEmbedded()));
        }

        // TODO We are not expecting to have to locate an assembly without a type name, so we expect
        // the other overload of LocateAssembly to be called instead.  If we find that this function
        // is actually called, we should see whether we need to implement further logic here.
        throw LogicError(L"Not yet implemented");
    }

    AssemblyLocation PackageAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName,
                                                            String       const& fullTypeName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return AssemblyLocation(ConstByteRange(
                Detail::BeginWinRTTypeSystemSupportEmbedded(),
                Detail::EndWinRTTypeSystemSupportEmbedded()));
        }

        // The name of the assembly must be a prefix of the name of the type.  TODO This may not
        // actually be the case for some scenarios, notably hybrid WinMDs produced by the managed
        // build system.  We should be sure to investigate further.
        String const lowercaseFullTypeName(Detail::MakeLowercase(fullTypeName));
        if (!Detail::StartsWith(lowercaseFullTypeName.c_str(), simpleName.c_str()))
            throw RuntimeError(L"Provided assembly/type pair does not match Windows Runtime naming rules");

        String namespaceName(fullTypeName);
        Internal::RemoveRightmostTypeNameComponent(namespaceName);
        if (namespaceName.empty())
            throw RuntimeError(L"Provided type has no namespace to resolve");

        return AssemblyLocation(FindMetadataForNamespace(namespaceName));
    }


    PackageAssemblyLocator::PathMap PackageAssemblyLocator::GetMetadataFiles() const
    {
        Lock const lock(_sync);
        return _metadataFiles;
    }

    AssemblyLocation PackageAssemblyLocator::FindMetadataForNamespace(String const& namespaceName) const
    {
        String const lowercaseNamespaceName(Detail::MakeLowercase(namespaceName));

        // First, search the metadata files we got from RoResolveNamespace:
        {
            Lock const lock(_sync);

            String enclosingNamespaceName(lowercaseNamespaceName);
            while (!enclosingNamespaceName.empty())
            {
                auto const it(_metadataFiles.find(enclosingNamespaceName));
                if (it != _metadataFiles.end())
                    return AssemblyLocation(it->second);

                Internal::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
            }
        }

        // WORKAROUND:  If the above failed, we can try to search in the package root.  However,
        // this should not be necessary.  RoResolveNamespace should return all of the resolvable
        // metadata files.
        // String enclosingNamespaceName(lowercaseNamespaceName);
        // while (!enclosingNamespaceName.empty())
        // {
        //     String const winmdPath(Detail::MakeLowercase(_packageRoot + enclosingNamespaceName + L".winmd"));
        //     if (Externals::FileExists(winmdPath.c_str()))
        //     {
        //         Lock const lock(_sync);
        //
        //         _metadataFiles.insert(std::make_pair(enclosingNamespaceName, winmdPath));
        //         return winmdPath;
        //     }
        // 
        //     Internal::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
        // }

        // If the type is in the 'Platform' or 'System' namespace, we special case it and use our
        // Platform metadata.  This heuristic isn't perfect, but it should be sufficient for non-
        // pathological type names.
        if (Detail::StartsWith(lowercaseNamespaceName.c_str(), L"platform") ||
            Detail::StartsWith(lowercaseNamespaceName.c_str(), L"system"))
            return AssemblyLocation(ConstByteRange(
                Detail::BeginWinRTTypeSystemSupportEmbedded(),
                Detail::EndWinRTTypeSystemSupportEmbedded()));

        // Otherwise, we failed to locate the metadata file.  Rats.
        throw RuntimeError(L"Failed to locate metadata file for provided namespace");
    }





    String LoaderConfiguration::TransformNamespace(String const& namespaceName)
    {
        // If the requested namespace is the System namespace, we redirect it to the Platform
        // namespace.  This allows us to handle fundamental types like System.Object, which would
        // otherwise be unresolvable because they are located in mscorlib, which is never loaded in
        // a native project.
        if (namespaceName == L"System")
            return L"Platform";

        return namespaceName;
    }





    LoaderContext::LoaderContext(std::unique_ptr<Loader>&& loader)
        : _loader(std::move(loader))
    {
        Detail::Verify([&]{ return _loader != nullptr; });
    }

    Loader const& LoaderContext::GetLoader() const
    {
        return *_loader;
    }

    LoaderContext::Locator const& LoaderContext::GetLocator() const
    {
        Loader const& loader(GetLoader());

        IAssemblyLocator const& locator(loader.GetAssemblyLocator(InternalKey()));

        Detail::Assert([&]{ return dynamic_cast<Locator const*>(&locator) != nullptr; });
        return *static_cast<Locator const*>(&locator);
    }

    Type LoaderContext::GetType(StringReference const typeFullName) const
    {
        Detail::Verify([&]{ return !typeFullName.empty(); });

        auto const endOfNamespaceIt(std::find(typeFullName.rbegin(), typeFullName.rend(), '.').base());

        Detail::Verify([&]{ return endOfNamespaceIt != typeFullName.rend().base(); });

        String const namespaceName(typeFullName.begin(), std::prev(endOfNamespaceIt));
        String const typeSimpleName(endOfNamespaceIt, typeFullName.end());

        return GetType(namespaceName.c_str(), typeSimpleName.c_str());
    }

    Type LoaderContext::GetType(StringReference const namespaceName, StringReference const typeSimpleName) const
    {
        Loader  const& loader (GetLoader() );
        Locator const& locator(GetLocator());
             
        AssemblyLocation const metadataLocation(locator.FindMetadataForNamespace(namespaceName.c_str()));
        if (metadataLocation.GetKind() == AssemblyLocation::Kind::Uninitialized)
            return Type();

        // TODO We need a non-throwing LoadAssembly.
        Assembly const assembly(loader.LoadAssembly(metadataLocation));
        if (!assembly.IsInitialized())
            return Type();

        return assembly.GetType(namespaceName.c_str(), typeSimpleName.c_str());
    }

    std::vector<Type> LoaderContext::GetImplementers(Type const& interfaceType) const
    {
        Detail::Verify([&]{ return interfaceType.IsInitialized(); });

        // HACK:  We only include Windows types if the interface name is from Windows.  This should
        // be correct, but if we improve our filtering below, we should be able to remove this hack
        // and not impact performance.
        bool const includeWindowsTypes(Detail::StartsWith(interfaceType.GetNamespace().c_str(), L"Windows"));

        Loader  const& loader (GetLoader() );
        Locator const& locator(GetLocator());

        std::vector<Type> implementers;

        typedef PackageAssemblyLocator::PathMap             Sequence;
        typedef PackageAssemblyLocator::PathMap::value_type Element;

        Sequence const metadataFiles(locator.GetMetadataFiles());
        std::for_each(begin(metadataFiles), end(metadataFiles), [&](Element const& f)
        {
            if (!includeWindowsTypes && Detail::StartsWith(f.first.c_str(), L"windows"))
                return;

            // TODO We can do better filtering than this by checking assembly references.
            // TODO Add caching of the obtained data.
            Assembly const a(loader.LoadAssembly(AssemblyLocation(f.second.c_str())));
            std::for_each(a.BeginTypes(), a.EndTypes(), [&](Type const& t)
            {
                if (std::find(t.BeginInterfaces(), t.EndInterfaces(), interfaceType) != t.EndInterfaces())
                    implementers.push_back(t);
            });
        });

        return implementers;
    }

    Type LoaderContext::GetActivationFactoryType(Type const& type)
    {
        Detail::Verify([&]{ return type.IsInitialized(); });

        Method const activatableConstructor(GetActivatableAttributeFactoryConstructor());

        auto const activatableAttributeIt(std::find_if(type.BeginCustomAttributes(), type.EndCustomAttributes(),
        [&](CustomAttribute const& a)
        {
            return a.GetConstructor() == activatableConstructor;
        }));

        Detail::Verify([&]{ return activatableAttributeIt != type.EndCustomAttributes(); });

        String const factoryTypeName(activatableAttributeIt->GetSingleStringArgument());

        return GetType(factoryTypeName.c_str());
    }

    Guid LoaderContext::GetGuid(Type const& type)
    {
        Detail::Verify([&]{ return type.IsInitialized(); });

        Type const guidAttributeType(GetGuidAttributeType());

        // TODO We can cache the GUID Type and compare using its identity instead, for performance.
        auto const it(std::find_if(type.BeginCustomAttributes(),
                                    type.EndCustomAttributes(),
                                    [&](CustomAttribute const& attribute)
        {
            return attribute.GetConstructor().GetDeclaringType() == guidAttributeType;
        }));

        // TODO We need to make sure that a type has only one GuidAttribute.
        return it != type.EndCustomAttributes() ? it->GetSingleGuidArgument() : Guid();
    }

    #define CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY(XTYPE, XNAME, ...)  \
        XTYPE LoaderContext::Get ## XNAME() const                                       \
        {                                                                               \
            Lock const lock(_sync);                                                     \
            if (!_delayInit ## XNAME ## Initialized.Get())                              \
            {                                                                           \
                _delayInit ## XNAME = (__VA_ARGS__)();                                  \
                _delayInit ## XNAME ## Initialized.Get() = true;                        \
            }                                                                           \
            return _delayInit ## XNAME;                                                 \
        }

    #define CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY_TYPE(XNAME, XNAMESPACE, XTYPENAME) \
        CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY(Type, XNAME, [&]() -> Type             \
        {                                                                                              \
            Type const type(GetType(XNAMESPACE, XTYPENAME));                                           \
            Detail::Verify([&]{ return type.IsInitialized(); }, L"Failed to find type");               \
            return type;                                                                               \
        })

    CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY_TYPE(
        ActivatableAttributeType,
        L"Windows.Foundation.Metadata",
        L"ActivatableAttribute");

    CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY_TYPE(
        GuidAttributeType,
        L"Windows.Foundation.Metadata",
        L"GuidAttribute");

    CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY(Method, ActivatableAttributeFactoryConstructor,
    [&]() -> Method
    {
        Type const attributeType(GetActivatableAttributeType());

        BindingFlags const bindingFlags(BindingAttribute::Public | BindingAttribute::Instance);
        auto const firstConstructor(attributeType.BeginConstructors(bindingFlags));
        auto const lastConstructor(attributeType.EndConstructors());

        auto const constructorIt(std::find_if(firstConstructor, lastConstructor, [&](Method const& constructor)
        {
            // TODO We should also check parameter types.
            return Detail::Distance(constructor.BeginParameters(), constructor.EndParameters()) == 2;
        }));

        Detail::Verify([&]{ return constructorIt != attributeType.EndConstructors(); });

        return *constructorIt;

    });

    #undef CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY
    #undef CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DEFINE_PROPERTY_TYPE



    GlobalLoaderContext::InitializedFlag     GlobalLoaderContext::_initialized;
    GlobalLoaderContext::UniqueContextFuture GlobalLoaderContext::_context;

    void GlobalLoaderContext::Initialize(UniqueContextFuture&& context)
    {
        // Ensure that we only initialize the global instance once:
        bool expected(false);
        if (!_initialized.compare_exchange_strong(expected, true))
            throw LogicError(L"Global Loader was already initialized");

        _context = std::move(context);
    }

    LoaderContext& GlobalLoaderContext::Get()
    {
        if (!_initialized.load())
            throw LogicError(L"Global Loader not yet initialized");

        LoaderContext* const context(_context.get().get());
        if (context == nullptr)
            throw LogicError(L"Global Loader was initialized successfully but is null");

        return *context;
    }

    bool GlobalLoaderContext::HasInitializationBegun()
    {
        return _initialized.load();
    }

    bool GlobalLoaderContext::IsInitialized()
    {
        return _context.valid();
    }





    void BeginInitialization()
    {
        if (GlobalLoaderContext::HasInitializationBegun())
            throw LogicError(L"Initialization has already begun");

        Externals::Initialize<Platform::WinRT>();

        // Start initialization in the background.  Note:  we explicitly want to specify an async
        // launch here.  This cannot run on an STA thread when /ZW is used.
        GlobalLoaderContext::Initialize(std::async(std::launch::async, []() -> std::unique_ptr<LoaderContext>
        {
            String const currentPackageRoot(Internal::GetCurrentPackageRoot());

            std::unique_ptr<PackageAssemblyLocator> resolver(new PackageAssemblyLocator(currentPackageRoot));
            PackageAssemblyLocator const& rawResolver(*resolver.get());

            std::unique_ptr<ILoaderConfiguration> configuration(new LoaderConfiguration());

            std::unique_ptr<Loader> loader(new Loader(std::move(resolver), std::move(configuration)));

            typedef PackageAssemblyLocator::PathMap             Sequence;
            typedef PackageAssemblyLocator::PathMap::value_type Element;

            Sequence const metadataFiles(rawResolver.GetMetadataFiles());
            std::for_each(begin(metadataFiles), end(metadataFiles), [&](Element const& e)
            {
                loader->LoadAssembly(AssemblyLocation(e.second));
            });

            std::unique_ptr<LoaderContext> context(new LoaderContext(std::move(loader)));
            return context;
        }));
    }

    bool HasInitializationBegun()
    {
        return GlobalLoaderContext::HasInitializationBegun();
    }

    bool IsInitialized()
    {
        return GlobalLoaderContext::IsInitialized();
    }

    void WhenInitializedCall(std::function<void()> const callable)
    {
        Concurrency::task<void>([&]{ GlobalLoaderContext::Get(); }).then(callable);
    }

} }
