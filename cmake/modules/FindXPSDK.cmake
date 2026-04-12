# SPDX-FileCopyrightText: Copyright (C) swift Project Community / Contributors
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1


macro(CheckPackageFound MSG)
    include(FindPackageHandleStandardArgs)

    find_package_handle_standard_args(XPSDK
            REQUIRED_VARS XP_SDK_FOUND
            FAIL_MESSAGE "XP SDK not found. ${MSG}"
    )
endmacro()

if (NOT DEFINED XP_SDK_PATH)
    CheckPackageFound("XP_SDK_PATH not set")
    return()
endif ()

if (NOT EXISTS ${XP_SDK_PATH})
    CheckPackageFound("XP_SDK_PATH does not exist")
    return()
endif ()

if (NOT EXISTS "${XP_SDK_PATH}/CHeaders" OR NOT EXISTS "${XP_SDK_PATH}/Libraries")
    CheckPackageFound("XP_SDK_PATH content does not look like XP SDK")
    return()
endif ()

# Use plain (non-imported) INTERFACE libraries so that CMake never tries to
# resolve a binary file path.  Expose them under the XPSDK:: namespace via
# ALIAS targets; the double-colon names are supported for aliases in CMake.
add_library(_xpsdk_xplm INTERFACE)
add_library(_xpsdk_xpwidgets INTERFACE)
add_library(XPSDK::XPLM ALIAS _xpsdk_xplm)
add_library(XPSDK::XPWidgets ALIAS _xpsdk_xpwidgets)

target_include_directories(_xpsdk_xplm INTERFACE
    ${XP_SDK_PATH}/CHeaders
    ${XP_SDK_PATH}/CHeaders/XPLM)
target_include_directories(_xpsdk_xpwidgets INTERFACE
    ${XP_SDK_PATH}/CHeaders
    ${XP_SDK_PATH}/CHeaders/Widgets)

if (SWIFT_WIN64)
    # Windows: link against the static import libraries shipped with the SDK.
    target_link_libraries(_xpsdk_xplm INTERFACE
        ${XP_SDK_PATH}/Libraries/Win/XPLM_64.lib)
    target_link_libraries(_xpsdk_xpwidgets INTERFACE
        ${XP_SDK_PATH}/Libraries/Win/XPWidgets_64.lib)

elseif (APPLE)
    # XP SDK ships .tbd stub frameworks.  The linker rejects the binary path
    # directly ("unknown file type").  Pass -Wl,-framework,Name so the
    # comma-separated args reach ld as two distinct arguments.  Using plain
    # INTERFACE (non-imported) targets ensures CMake never attaches a binary
    # file path to the link command.
    target_link_options(_xpsdk_xplm INTERFACE
        "-Wl,-F${XP_SDK_PATH}/Libraries/Mac"
        "-Wl,-framework,XPLM"
    )
    target_link_options(_xpsdk_xpwidgets INTERFACE
        "-Wl,-F${XP_SDK_PATH}/Libraries/Mac"
        "-Wl,-framework,XPWidgets"
    )

endif ()

set(XP_SDK_FOUND TRUE)
CheckPackageFound("Found")
