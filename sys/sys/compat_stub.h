/*	$NetBSD: compat_stub.h,v 1.25.18.1 2023/12/10 13:06:16 martin Exp $	*/

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

#ifndef _SYS_COMPAT_STUB_H
#define _SYS_COMPAT_STUB_H

#include <sys/module_hook.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sigtypes.h>

/*
 * NOTE: If you make changes here, please remember to update the
 * kernel version number in sys/param.h to ensure that kernel
 * and modules stay in sync.
 */

/*
 * Routine hooks for compat_50___sys_ntp_gettime
 *
 * MP-hooks not needed since the NTP code is not modular
 */

struct ntptimeval;
struct timex;

extern void (*vec_ntp_gettime)(struct ntptimeval *);
extern int (*vec_ntp_timestatus)(void);
extern void (*vec_ntp_adjtime1)(struct timex *);

/*
MODULE_HOOK(ntp_gettime_hook, int, (struct ntptimeval *));
MODULE_HOOK(ntp_timestatus_hook, int, (void);
MODULE_HOOK(ntp_adjtime1_hook, int, (struct timex *));
*/

/*
 * Routine hooks for SCTP code - used by rtsock
 *
 * MP-hooks not needed since the SCTP code is not modular
 */
struct ifaddr;
extern void (*vec_sctp_add_ip_address)(struct ifaddr *);
extern void (*vec_sctp_delete_ip_address)(struct ifaddr *);

/*
MODULE_HOOK(sctp_add_ip_address, int, struct ifaddr *);
MODULE_HOOK(sctp_delete_ip_address, int, struct ifaddr *);
*/


/*
 * usb devinfo compatibility
 */

struct usbd_device;
struct usb_device_info_old;
struct usb_event;
struct usb_event_old;
struct uio;
MODULE_HOOK(usb_subr_fill_30_hook, int,
    (struct usbd_device *, struct usb_device_info_old *, int,
      void (*)(struct usbd_device *, char *, size_t, char *, size_t, int, int),
      int (*)(char *, size_t, int)));
MODULE_HOOK(usb_subr_copy_30_hook, int,
    (struct usb_event *, struct usb_event_old *, struct uio *));

/*
 * Routine vector for dev/ccd ioctl()
 */

MODULE_HOOK(ccd_ioctl_60_hook, int, (dev_t, u_long, void *, int, struct lwp *,
    int (*f)(dev_t, u_long, void *, int, struct lwp *)))

/*
 * Routine vector for dev/clockctl ioctl()
 */

MODULE_HOOK(clockctl_ioctl_50_hook, int,
    (dev_t, u_long, void *, int, struct lwp *));

/*
 * if_sppp device compatibility ioctl subroutine
 */

struct sppp;
MODULE_HOOK(sppp_params_50_hook, int, (struct sppp *, u_long, void *));

/*
 * cryptodev compatibility ioctl
 */

struct fcrypt;
struct session_op;
struct csession;
struct crypt_op;
struct crypt_n_op;
struct kmutex_t;
MODULE_HOOK(ocryptof_50_hook, int, (struct file *, u_long, void *));

/*
 * raidframe compatibility
 */
struct raid_softc;
MODULE_HOOK(raidframe_ioctl_50_hook, int,
    (struct raid_softc *, u_long, void *));
MODULE_HOOK(raidframe_ioctl_80_hook, int,
    (struct raid_softc *, u_long, void *));
MODULE_HOOK(raidframe_netbsd32_ioctl_hook, int,
    (struct raid_softc *, u_long, void *));

/*
 * puffs compatibility
 */

struct puffs_req;
MODULE_HOOK(puffs_out_50_hook, int,
    (struct puffs_req *, struct puffs_req **, ssize_t *));	/* outgoing */
MODULE_HOOK(puffs_in_50_hook, void,
    (struct puffs_req *, struct puffs_req *));			/* incoming */

/*
 * wsevents compatibility
 */

struct wscons_event;
struct uio;
MODULE_HOOK(wscons_copyout_events_50_hook, int,
    (const struct wscons_event *, int, struct uio *));

/*
 * sysmon_power compatibility
 */

struct power_event;
struct sysmon_pswitch;
MODULE_HOOK(compat_sysmon_power_40_hook, void,
    (struct power_event *, struct sysmon_pswitch *, int));

/*
 * compat_bio indirect function pointer
 */

MODULE_HOOK(compat_bio_30_hook, int,
    (void *, u_long, void *, int(*)(void *, u_long, void *)));

/*
 * vnd_30 ioctl compatibility
 */
struct vattr;
MODULE_HOOK(compat_vndioctl_30_hook, int, (u_long, struct lwp *, void *, int,
    struct vattr *, int (*)(struct lwp *, void *, int, struct vattr *)));

/*
 * vnd_50 ioctl compatibility
 */
struct vattr;
MODULE_HOOK(compat_vndioctl_50_hook, int, (u_long, struct lwp *, void *, int,
    struct vattr *, int (*)(struct lwp *, void *, int, struct vattr *)));

/*
 * ieee80211 ioctl compatibility
 */
struct ieee80211com;

MODULE_HOOK(ieee80211_ioctl_20_hook, int,
    (struct ieee80211com *, u_long, void *));

/*
 * if_43 compatibility
 */
struct socket;

MODULE_HOOK(if_cvtcmd_43_hook, int, (u_long *, u_long));
MODULE_HOOK(if_ifioctl_43_hook, int,
    (struct socket *, u_long, u_long, void *, struct lwp *));

/*
 * if43_cvtcmd_20 compatibility
 */
MODULE_HOOK(if43_cvtcmd_20_hook, int, (u_long));

/*
 * tty 43 ioctl compatibility
 */
struct tty;

MODULE_HOOK(tty_ttioctl_43_hook, int,
    (struct tty *, u_long, void *, int, struct lwp *));

/*
 * uipc_syscalls_40 compatibility
 */

MODULE_HOOK(uipc_syscalls_40_hook, int, (u_long, void *));

/*
 * uipc_socket_50 compatibility
 */
struct sockopt;
struct mbuf;

MODULE_HOOK(uipc_socket_50_setopt1_hook, int,
    (int, struct socket *, const struct sockopt *));
MODULE_HOOK(uipc_socket_50_getopt1_hook, int,
    (int, struct socket *, struct sockopt *));
MODULE_HOOK(uipc_socket_50_sbts_hook, int, (int, struct mbuf ***));

/*
 * uipc_syscalls_50 compatibility
 */

MODULE_HOOK(uipc_syscalls_50_hook, int, (struct lwp *, u_long, void *));

/*
 * rtsock 14 compatibility
 */
struct ifnet;
struct rt_walkarg;
struct rt_addrinfo;
MODULE_HOOK(rtsock_oifmsg_14_hook, void, (struct ifnet *));
MODULE_HOOK(rtsock_iflist_14_hook, int,
    (struct ifnet *, struct rt_walkarg *, struct rt_addrinfo *, size_t));

/*
 * Hooks for rtsock_50
 */

struct rtentry;
struct ifaddr;
MODULE_HOOK(rtsock_oifmsg_50_hook, void, (struct ifnet *));
MODULE_HOOK(rtsock_iflist_50_hook, int,
    (struct ifnet *, struct rt_walkarg *, struct rt_addrinfo *, size_t));
MODULE_HOOK(rtsock_rt_missmsg_50_hook, void,
    (int, const struct rt_addrinfo *, int, int));
MODULE_HOOK(rtsock_rt_ifmsg_50_hook, void, (struct ifnet *));
MODULE_HOOK(rtsock_rt_addrmsg_rt_50_hook, void,
    (int, struct ifaddr *, int, struct rtentry *));
MODULE_HOOK(rtsock_rt_addrmsg_src_50_hook, void,
    (int, struct ifaddr *, const struct sockaddr *));
MODULE_HOOK(rtsock_rt_addrmsg_50_hook, void, (int, struct ifaddr *));
MODULE_HOOK(rtsock_rt_ifannouncemsg_50_hook, void, (struct ifnet *, int));
MODULE_HOOK(rtsock_rt_ieee80211msg_50_hook, void,
    (struct ifnet *, int, void *, size_t));

/*
 * Hooks for rtsock_70
 */
struct ifaddr;
MODULE_HOOK(rtsock_newaddr_70_hook, void, (int, struct ifaddr *));
MODULE_HOOK(rtsock_iflist_70_hook, int,
    (struct rt_walkarg *, struct ifaddr *, struct rt_addrinfo *));

/*
 * modctl handler for old style OSTAT
 */
struct iovec;
MODULE_HOOK(compat_modstat_80_hook, int, (int, struct iovec *, void *));

/*
 * mask for kern_sig_43's killpg
 */
extern int kern_sig_43_pgid_mask;

/*
 * Hooks for kern_proc.c for netbsd32 compat
 */
struct ps_strings;
MODULE_HOOK(kern_proc32_copyin_hook, int,
    (struct proc *, struct ps_strings *));
MODULE_HOOK(kern_proc32_base_hook, vaddr_t, (char **, size_t));

/*
 * Hook to allow sparc fpu code to see if a process is using sunos
 * emulation, and select proper fup codes
 */
struct emul;
MODULE_HOOK(get_emul_sunos_hook, int, (const struct emul **));

/*
 * Hooks for rnd_ioctl_50
 */
MODULE_HOOK(rnd_ioctl_50_hook, int, (struct file *, u_long, void *));
MODULE_HOOK(rnd_ioctl32_50_hook, int, (struct file *, u_long, void *));

/*
 * Hooks for compat_60 ttioctl and ptmioctl
 */
MODULE_HOOK(tty_ttioctl_60_hook, int,
    (struct tty *, u_long, void *, int, struct lwp *));
MODULE_HOOK(tty_ptmioctl_60_hook, int,
    (dev_t, u_long, void *, int, struct lwp *));

/*
 * Hook for compat_10 openat
 */
struct pathbuf;
MODULE_HOOK(vfs_openat_10_hook, int, (struct pathbuf **));

/*
 * Hook for compat_70_unp_addsockcred
 */
MODULE_HOOK(uipc_unp_70_hook, struct mbuf *,
    (struct lwp *, struct mbuf *));

/*
 * Hook for sysvipc50 sysctl
 */
#include <sys/sysctl.h>
MODULE_HOOK(sysvipc_sysctl_50_hook, int, (SYSCTLFN_PROTO));

/*
 * ifmedia_80 compatibility
 */

struct ifmedia;
struct ifreq;
MODULE_HOOK(ifmedia_80_pre_hook, int, (struct ifreq *, u_long *, bool *));
MODULE_HOOK(ifmedia_80_post_hook, int, (struct ifreq *, u_long));

/* 
 * Hook for 32-bit machine name
 * 
 * This probably would be better placed in compat/netbsd32/netbsd32_mod.c
 * but the consumer code in linux32_exec_elf32.c is sometimes included in
 * the main kernel, and not in a compat_netbsd32 module.  (In particular,
 * this is true for i386 and sgimips.)
 */
struct reg;
MODULE_HOOK(netbsd32_machine32_hook, const char *, (void));
MODULE_HOOK(netbsd32_reg_validate_hook, int,
    (struct lwp *, const struct reg *));

/*
 * Hook for compat_16 sendsig_sigcontext
 */
struct ksiginfo;
MODULE_HOOK(sendsig_sigcontext_16_hook, void,
    (const struct ksiginfo *, const sigset_t *));

/*
 * Hooks for coredumps
 */

struct uvm_coredump_state;
MODULE_HOOK(coredump_hook, int, (struct lwp *, const char *));
MODULE_HOOK(coredump_offset_hook, off_t, (struct coredump_iostate *));
MODULE_HOOK(coredump_write_hook, int,
    (struct coredump_iostate *, enum uio_seg, const void *, size_t));
MODULE_HOOK(coredump_netbsd_hook, int,
    (struct lwp *, struct coredump_iostate *));
MODULE_HOOK(coredump_netbsd32_hook, int,
    (struct lwp *, struct coredump_iostate *));
MODULE_HOOK(coredump_elf32_hook, int,
    (struct lwp *, struct coredump_iostate *));
MODULE_HOOK(coredump_elf64_hook, int,
    (struct lwp *, struct coredump_iostate *));
MODULE_HOOK(uvm_coredump_walkmap_hook, int,
    (struct proc *, int (*)(struct uvm_coredump_state *), void *));
MODULE_HOOK(uvm_coredump_count_segs_hook, int, (struct proc *));

/*
 * Hook for amd64 handler for oosyscall for COMPAT_NETBSD32 && COMPAT_10)
 */
struct proc;
struct trapframe;
MODULE_HOOK(amd64_oosyscall_hook, int, (struct proc *, struct trapframe *));

/*
 * Hook for compat_90 to deal with removal of nd6 from the kernel
 */
MODULE_HOOK(net_inet6_nd_90_hook, int, (int));

#endif	/* _SYS_COMPAT_STUB_H */
