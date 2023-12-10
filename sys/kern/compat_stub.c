/* $NetBSD: compat_stub.c,v 1.22.4.1 2023/12/10 13:06:16 martin Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifdef _KERNEL_OPT
#include "opt_ntp.h"
#endif

#include <sys/systm.h>
#include <sys/compat_stub.h>

#ifdef NTP
#include <sys/timespec.h>
#include <sys/timex.h>
#endif

/*
 * Routine vectors for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

#ifdef NTP
void (*vec_ntp_gettime)(struct ntptimeval *) = ntp_gettime;
int (*vec_ntp_timestatus)(void) = ntp_timestatus;
void (*vec_ntp_adjtime1)(struct timex *) = ntp_adjtime1;
#else
void (*vec_ntp_gettime)(struct ntptimeval *) = NULL;
int (*vec_ntp_timestatus)(void) = NULL;
void (*vec_ntp_adjtime1)(struct timex *) = NULL;
#endif

/*
 * Routine vectors for sctp (called from within rtsock)
 *
 * MP-hooks not needed since the SCTP code is not modular
 *
 * For now, just point these at NULL.  Network initialization code
 * in if.c will overwrite these with correct values.  This is needed
 * to enable building of rumpkern library without creating circular
 * dependency with rumpnet library
 */

void (*vec_sctp_add_ip_address)(struct ifaddr *) = NULL;
void (*vec_sctp_delete_ip_address)(struct ifaddr *) = NULL;


/*
 * usb device_info compatibility
 */
struct usb_subr_fill_30_hook_t usb_subr_fill_30_hook;
struct usb_subr_copy_30_hook_t usb_subr_copy_30_hook;

/*
 * ccd device compatibility ioctl
 */

struct ccd_ioctl_60_hook_t ccd_ioctl_60_hook;

/*
 * clockctl device compatibility ioctl
 */

struct clockctl_ioctl_50_hook_t clockctl_ioctl_50_hook;

/*
 * if_sppp device compatibility ioctl subroutine
 */

struct sppp_params_50_hook_t sppp_params_50_hook;

/*
 * cryptodev compatibility ioctl
 */

struct ocryptof_50_hook_t ocryptof_50_hook;

/*
 * raidframe compatibility
 */
struct raidframe_ioctl_50_hook_t raidframe_ioctl_50_hook;
struct raidframe_ioctl_80_hook_t raidframe_ioctl_80_hook;
struct raidframe_netbsd32_ioctl_hook_t raidframe_netbsd32_ioctl_hook;

/*
 * puffs compatibility
 */

struct puffs_out_50_hook_t puffs_out_50_hook;
struct puffs_in_50_hook_t puffs_in_50_hook;

/*
 * wsevents compatibility
 */
struct wscons_copyout_events_50_hook_t wscons_copyout_events_50_hook;

/*
 * sysmon_power compatibility
 */
struct compat_sysmon_power_40_hook_t compat_sysmon_power_40_hook;

/*
 * compat_bio compatibility
 */
struct compat_bio_30_hook_t compat_bio_30_hook;

/*
 * vnd ioctl compatibility
 */
struct compat_vndioctl_30_hook_t compat_vndioctl_30_hook;
struct compat_vndioctl_50_hook_t compat_vndioctl_50_hook;

/*
 * ieee80211 ioctl compatibility
 */
struct ieee80211_ioctl_20_hook_t ieee80211_ioctl_20_hook;

/*
 * if_43 compatibility
 */
struct if_cvtcmd_43_hook_t if_cvtcmd_43_hook;
struct if_ifioctl_43_hook_t if_ifioctl_43_hook;

/*
 * if43_cvtcmd_20 compatibility
 */
struct if43_cvtcmd_20_hook_t if43_cvtcmd_20_hook;

/* 
 * tty 43 ioctl compatibility
 */
struct tty_ttioctl_43_hook_t tty_ttioctl_43_hook;

/*
 * uipc_syscalls_40 compatibility
 */
struct uipc_syscalls_40_hook_t uipc_syscalls_40_hook;

/*
 * uipc_socket_50 compatibility
 */
struct uipc_socket_50_setopt1_hook_t uipc_socket_50_setopt1_hook;
struct uipc_socket_50_getopt1_hook_t uipc_socket_50_getopt1_hook;
struct uipc_socket_50_sbts_hook_t uipc_socket_50_sbts_hook;

/*
 * uipc_syscalls_50 compatibility
 */
struct uipc_syscalls_50_hook_t uipc_syscalls_50_hook;

/*
 * rtsock 14 compatibility
 */
struct rtsock_oifmsg_14_hook_t rtsock_oifmsg_14_hook;
struct rtsock_iflist_14_hook_t rtsock_iflist_14_hook;

/*
 * rtsock 50 compatibility
 */
struct rtsock_iflist_50_hook_t rtsock_iflist_50_hook;
struct rtsock_oifmsg_50_hook_t rtsock_oifmsg_50_hook;
struct rtsock_rt_missmsg_50_hook_t rtsock_rt_missmsg_50_hook;
struct rtsock_rt_ifmsg_50_hook_t rtsock_rt_ifmsg_50_hook;
struct rtsock_rt_ifannouncemsg_50_hook_t rtsock_rt_ifannouncemsg_50_hook;
struct rtsock_rt_addrmsg_rt_50_hook_t rtsock_rt_addrmsg_rt_50_hook;
struct rtsock_rt_addrmsg_src_50_hook_t rtsock_rt_addrmsg_src_50_hook;
struct rtsock_rt_addrmsg_50_hook_t rtsock_rt_addrmsg_50_hook;
struct rtsock_rt_ieee80211msg_50_hook_t rtsock_rt_ieee80211msg_50_hook;

/*
 * rtsock 70 compatibility
 */
struct rtsock_newaddr_70_hook_t rtsock_newaddr_70_hook;
struct rtsock_iflist_70_hook_t rtsock_iflist_70_hook;

/*
 * modctl handler for old style OSTAT
 */
struct compat_modstat_80_hook_t compat_modstat_80_hook;

/*
 * mask for kern_sig_43's killpg (updated by compat_09)
 */
int kern_sig_43_pgid_mask = ~0;

/*
 * hook for kern_proc_32
 */
struct kern_proc32_copyin_hook_t kern_proc32_copyin_hook;
struct kern_proc32_base_hook_t kern_proc32_base_hook;

/*
 * Hook for sparc fpu code to check if a process is running 
 * under sunos emulation
 */
struct get_emul_sunos_hook_t get_emul_sunos_hook;

/*
 * Hook for rnd_ioctl_50 callbacks
 */
struct rnd_ioctl_50_hook_t rnd_ioctl_50_hook;
struct rnd_ioctl32_50_hook_t rnd_ioctl32_50_hook;

/* 
 * Hooks for compat_60 ttioctl and ptmioctl
 */
struct tty_ttioctl_60_hook_t tty_ttioctl_60_hook;
struct tty_ptmioctl_60_hook_t tty_ptmioctl_60_hook;

/*
 * Hook for compat_10 openat
 */
struct vfs_openat_10_hook_t vfs_openat_10_hook;

/*
 * Hook for compat_70_unp_addsockcred
 */
struct uipc_unp_70_hook_t uipc_unp_70_hook;

/*
 * Hook for sysvipc50_sysctl
 */
struct sysvipc_sysctl_50_hook_t sysvipc_sysctl_50_hook;

/*
 * ifmedia_80 compatibility
 */
struct ifmedia_80_pre_hook_t ifmedia_80_pre_hook;
struct ifmedia_80_post_hook_t ifmedia_80_post_hook;

/*
 * Hook for 32-bit machine name
 *
 * This probably would be better placed in compat/netbsd32/netbsd32_mod.c
 * but the consumer code in linux32_exec_elf32.c is sometimes included in
 * the main kernel, and not in a compat_netbsd32 module.  (In particular,
 * this is true for i386 and sgimips.)
 */
struct netbsd32_machine32_hook_t netbsd32_machine32_hook;
struct netbsd32_reg_validate_hook_t netbsd32_reg_validate_hook;

/*
 * Hook for sendsig_sigcontext_16
 */
struct sendsig_sigcontext_16_hook_t sendsig_sigcontext_16_hook;

/*
 * Hooks for coredumps
 */
struct coredump_hook_t coredump_hook;
struct coredump_offset_hook_t coredump_offset_hook;
struct coredump_write_hook_t coredump_write_hook;
struct coredump_netbsd_hook_t coredump_netbsd_hook;
struct coredump_netbsd32_hook_t coredump_netbsd32_hook;
struct coredump_elf32_hook_t coredump_elf32_hook;
struct coredump_elf64_hook_t coredump_elf64_hook;
struct uvm_coredump_walkmap_hook_t uvm_coredump_walkmap_hook;
struct uvm_coredump_count_segs_hook_t uvm_coredump_count_segs_hook;

/*
 * Hooks for interacting with remnants of netinet6/nd6 removal
 */
struct net_inet6_nd_90_hook_t net_inet6_nd_90_hook;

