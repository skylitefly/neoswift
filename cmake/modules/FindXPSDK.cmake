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
    # XP SDK ships .tbd stub frameworks.  Passing the binary path directly
    # (IMPORTED_LOCATION) fails with "unknown file type" on Xcode 16+.
    # find_library locates the .framework bundle and CMake automatically
    # expands the path to -F <dir> -framework <name> at link time.
    find_library(XPLM_FRAMEWORK XPLM
        PATHS "${XP_SDK_PATH}/Libraries/Mac"
        NO_DEFAULT_PATH)
    find_library(XPWIDGETS_FRAMEWORK XPWidgets
        PATHS "${XP_SDK_PATH}/Libraries/Mac"
        NO_DEFAULT_PATH)
    if(XPLM_FRAMEWORK)
        target_link_libraries(XPSDK::XPLM INTERFACE ${XPLM_FRAMEWORK})
    endif()
    if(XPWIDGETS_FRAMEWORK)
        target_link_libraries(XPSDK::XPWidgets INTERFACE ${XPWIDGETS_FRAMEWORK})
    endif()

endif ()

set(XP_SDK_FOUND TRUE)
CheckPackageFound("Found")
