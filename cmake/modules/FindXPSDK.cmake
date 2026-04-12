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


if (SWIFT_WIN64)
    add_library(XPSDK::XPLM STATIC IMPORTED GLOBAL)
    add_library(XPSDK::XPWidgets STATIC IMPORTED GLOBAL)
else ()
    add_library(XPSDK::XPLM IMPORTED INTERFACE GLOBAL)
    add_library(XPSDK::XPWidgets IMPORTED INTERFACE GLOBAL)
endif ()

target_include_directories(XPSDK::XPLM INTERFACE ${XP_SDK_PATH}/CHeaders ${XP_SDK_PATH}/CHeaders/XPLM)
target_include_directories(XPSDK::XPWidgets INTERFACE ${XP_SDK_PATH}/CHeaders ${XP_SDK_PATH}/CHeaders/Widgets)

if (SWIFT_WIN64)
    set_target_properties(XPSDK::XPLM PROPERTIES IMPORTED_LOCATION ${XP_SDK_PATH}/Libraries/Win/XPLM_64.lib)
    set_target_properties(XPSDK::XPWidgets PROPERTIES IMPORTED_LOCATION ${XP_SDK_PATH}/Libraries/Win/XPWidgets_64.lib)

elseif (APPLE)
    # XP SDK ships .tbd stub frameworks.  The linker rejects the binary
    # path directly (IMPORTED_LOCATION → "unknown file type").
    # Use LINKER: syntax which expands to -Wl,-framework,Name so the
    # comma-separated args reach ld as two distinct arguments; no spaces
    # are involved so CMake quoting cannot break the flag.
    target_link_options(XPSDK::XPLM INTERFACE
        "LINKER:-F${XP_SDK_PATH}/Libraries/Mac"
        "LINKER:-framework,XPLM"
    )
    target_link_options(XPSDK::XPWidgets INTERFACE
        "LINKER:-F${XP_SDK_PATH}/Libraries/Mac"
        "LINKER:-framework,XPWidgets"
    )

endif ()

set(XP_SDK_FOUND TRUE)
CheckPackageFound("Found")
