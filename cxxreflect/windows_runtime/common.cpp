
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/common.hpp"

#include <inspectable.h>

namespace cxxreflect { namespace windows_runtime {

    auto inspectable_deleter::operator()(IInspectable* inspectable) -> void
    {
        inspectable->Release();
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
