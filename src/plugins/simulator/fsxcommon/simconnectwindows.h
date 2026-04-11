// SPDX-FileCopyrightText: Copyright (C) 2018 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

// in P3Dv4 the simconnect.h does not include windows.h
// here we include windows.h first

#ifndef SWIFT_SIMPLUGIN_FSX_SIMCONNECTWINDOWS_H
#define SWIFT_SIMPLUGIN_FSX_SIMCONNECTWINDOWS_H

#ifndef NOMINMAX
#    define NOMINMAX
#endif

// clash with struct interface in objbase.h used to happen
#pragma push_macro("interface")
#undef interface

// clang-format off
#include <Windows.h>

// When consuming SimConnect wrappers from outside fsxcommon.dll (i.e. any
// plugin that links fsxcommon), the wrapper functions must be imported from
// fsxcommon.dll rather than exported from the consumer.  Pre-define
// SIMCONNECTAPI before SimConnect.h so that its own conditional definition
// (which may expand to dllexport when SIMCONNECT_H_NOMANIFEST is set) does
// not take effect.  Also suppress the auto-link pragma.
#ifndef BUILD_FSXCOMMON_LIB
#  ifndef SIMCONNECT_H_NOMANIFEST
#    define SIMCONNECT_H_NOMANIFEST
#  endif
#  ifndef SIMCONNECTAPI
#    define SIMCONNECTAPI extern "C" __declspec(dllimport)
#  endif
#endif

#include <SimConnect.h>
// clang-format on

#include <QtGlobal>

#pragma pop_macro("interface")

#ifndef Q_OS_WIN64
//! adding struct SIMCONNECT_DATA_PBH not existing in SimConnect FSX
struct SIMCONNECT_DATA_PBH
{
    double Pitch; //!< pitch
    double Bank; //!< bank
    double Heading; //!< heading
};
#endif

#endif // SWIFT_SIMPLUGIN_FSX_SIMCONNECTWINDOWS_H
