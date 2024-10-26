/* cti-common.h
 *
 * Copyright (c) 2020 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * Concise Thread Interface for Thread Border router control.
 */

#ifndef __CTI_COMMON_H__
#define __CTI_COMMON_H__

#if OPENTHREAD_PLATFORM_POSIX
#define NO_IOLOOP 1
#endif

typedef struct _cti_connection_t *cti_connection_t;

#ifndef GCCATTR
#ifdef __clang__
#define GCCATTR(x)
#else
#define GCCATTR(x) __attribute__(x)
#endif
#endif

#ifndef NONNULL
#ifdef __clang__
#define NONNULL _Nonnull
#else
#define NONNULL
#endif
#endif

#ifndef NULLABLE
#ifdef __clang__
#define NULLABLE _Nullable
#else
#define NULLABLE
#endif
#endif

#ifndef UNUSED
#ifdef __clang__
#define UNUSED __unused
#else
#define UNUSED  __attribute__((unused))
#endif
#endif

#ifndef ND6_INFINITE_LIFETIME
#define ND6_INFINITE_LIFETIME 0xffffffff
#endif

#define kCTIMessageType_Response                0
#define kCTIMessageType_AddService              1
#define kCTIMessageType_RemoveService           2
#define kCTIMessageType_AddPrefix               3
#define kCTIMessageType_RemovePrefix            4
#define kCTIMessageType_GetTunnelName           5
#define kCTIMessageType_TunnelNameResponse      6
#define kCTIMessageType_RequestStateEvents      7
#define kCTIMessageType_StateEvent              8
#define kCTIMessageType_RequestUInt64PropEvents 9
#define kCTIMessageType_UInt64PropEvent        10
#define kCTIMessageType_RequestRoleEvents      11
#define kCTIMessageType_RoleEvent              12
#define kCTIMessageType_RequestServiceEvents   13
#define kCTIMessageType_ServiceEvent           14
#define kCTIMessageType_RequestPrefixEvents    15
#define kCTIMessageType_PrefixEvent            16

#if defined(POSIX_BUILD) || OPENTHREAD_PLATFORM_POSIX
#define kCTIPropertyPartitionID                 1
#define kCTIPropertyExtendedPANID               2
#else
#define kCTIPropertyPartitionID                 "Network:PartitionID"
#define kCTIPropertyExtendedPANID               "Network:XPANID"
#endif // POSIX_BUILD

typedef enum
{
    kCTIStatus_NoError                   =  0,
    kCTIStatus_UnknownError              = -65537,
    kCTIStatus_NoMemory                  = -65539,   // No Memory
    kCTIStatus_BadParam                  = -65540,   // Client passed invalid arg
    kCTIStatus_Invalid                   = -65549,   // Invalid CTI message
    kCTIStatus_DaemonNotRunning          = -65563,   // Daemon not running: unable to connect
    kCTIStatus_Disconnected              = -65569,   // Server disconnected after successful connection
    kCTIStatus_NotPermitted              = -65571    // Unable to open the daemon socket, or connection not permitted.
} cti_status_t;

// Enum values for kWPANTUNDStateXXX (see wpan-properties.h)
typedef enum {
    kCTI_NCPState_Uninitialized,
    kCTI_NCPState_Fault,
    kCTI_NCPState_Upgrading,
    kCTI_NCPState_DeepSleep,
    kCTI_NCPState_Offline,
    kCTI_NCPState_Commissioned,
    kCTI_NCPState_Associating,
    kCTI_NCPState_CredentialsNeeded,
    kCTI_NCPState_Associated,
    kCTI_NCPState_Isolated,
    kCTI_NCPState_NetWake_Asleep,
    kCTI_NCPState_NetWake_Waking,
    kCTI_NCPState_Unknown
} cti_network_state_t;

typedef enum {
    kCTI_NetworkNodeType_Unknown,
    kCTI_NetworkNodeType_Router,
    kCTI_NetworkNodeType_EndDevice,
    kCTI_NetworkNodeType_SleepyEndDevice,
    kCTI_NetworkNodeType_SynchronizedSleepyEndDevice,
    kCTI_NetworkNodeType_NestLurker,
    kCTI_NetworkNodeType_Commissioner,
    kCTI_NetworkNodeType_Leader,
    kCTI_NetworkNodeType_SleepyRouter,
} cti_network_node_type_t;

#define kCTIRoleDisabled 0
#define kCTIRoleDetached 1
#define kCTIRoleChild    2
#define kCTIRoleRouter   3
#define kCTIRoleLeader   4

// CTI flags.
#define kCTIFlag_Stable                1
#define kCTIFlag_NCP                   2

#endif // _CTI_COMMON_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
