/*	$NetBSD: compat_mod.h,v 1.1.42.11 2018/03/28 07:51:09 pgoyette Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_COMPAT_MOD_H
#define	_COMPAT_MOD_H

#include <sys/sysctl.h>

void compat_sysctl_init(void);
void compat_sysctl_fini(void);

void compat_sysctl_vfs(struct sysctllog **);

#ifdef COMPAT_80
int compat_80_init(void);
int compat_80_fini(void);
#endif

#ifdef COMPAT_70
int compat_70_init(void);
int compat_70_fini(void);
#endif

#ifdef COMPAT_60
int compat_60_init(void);
int compat_60_fini(void);
int kern_time_60_init(void);
int kern_time_60_fini(void);
int kern_sa_60_init(void);
int kern_sa_60_fini(void);
void kern_tty_60_init(void);
void kern_tty_60_fini(void);
#endif

#ifdef COMPAT_50
int compat_50_init(void);
int compat_50_fini(void);
int kern_50_init(void);
int kern_50_fini(void);
int kern_time_50_init(void);
int kern_time_50_fini(void);
int kern_select_50_init(void);
int kern_select_50_fini(void);
void uvm_50_init(void);
void uvm_50_fini(void);
int vfs_syscalls_50_init(void);
int vfs_syscalls_50_fini(void);
void if_50_init(void);
void if_50_fini(void);
#endif

#ifdef COMPAT_40
int compat_40_init(void);
int compat_40_fini(void);
void if_40_init(void);
void if_40_fini(void);
int vfs_syscalls_40_init(void);
int vfs_syscalls_40_fini(void);
void sysmon_power_40_init(void);
void sysmon_power_40_fini(void);
#endif

#ifdef COMPAT_30
int compat_30_init(void);
int compat_30_fini(void);
int kern_time_30_init(void);
int kern_time_30_fini(void);
int vfs_syscalls_30_init(void);
int vfs_syscalls_30_fini(void);
int uipc_syscalls_30_init(void);
int uipc_syscalls_30_fini(void);
void bio_30_init(void);
void bio_30_fini(void);
void vnd_30_init(void);
void vnd_30_fini(void);
#endif

#endif /* !_COMPAT_MOD_H_ */
