# SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1
#
# FindVATSIMAuth – builds OpenVatsimAuth from source as a static library.
# No external dependencies: MD5 and RNG are inlined in openvatsimauth.cpp.

set(_OPENVATSIMAUTH_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/openvatsimauth")

if(NOT TARGET VATSIMAuth_impl)
    add_library(VATSIMAuth_impl STATIC "${_OPENVATSIMAUTH_DIR}/openvatsimauth.cpp")

    target_include_directories(VATSIMAuth_impl PUBLIC "${_OPENVATSIMAUTH_DIR}")

    if(WIN32)
        target_link_libraries(VATSIMAuth_impl PUBLIC iphlpapi ws2_32)
    endif()

    set_target_properties(VATSIMAuth_impl PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )

    add_library(VATSIMAuth::VATSIMAuth ALIAS VATSIMAuth_impl)
endif()

set(VATSIM_AUTH_FOUND TRUE)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VATSIMAuth REQUIRED_VARS VATSIM_AUTH_FOUND)
