/*	$NetBSD: linux32_types.h,v 1.4.22.2 2008/01/19 12:15:00 bouyer Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX32_TYPES_H
#define _LINUX32_TYPES_H

#ifdef __amd64__
#include <compat/linux32/arch/amd64/linux32_types.h>
#endif

typedef unsigned short linux32_gid16_t;
typedef unsigned short linux32_uid16_t;

typedef netbsd32_pointer_t linux32_oldmmapp;
typedef netbsd32_pointer_t linux32_utsnamep;
typedef netbsd32_pointer_t linux32_stat64p;
typedef netbsd32_pointer_t linux32_statp;
typedef netbsd32_pointer_t linux32_statfsp;
typedef netbsd32_pointer_t linux32_sigactionp_t;
typedef netbsd32_pointer_t linux32_sigsetp_t;
typedef netbsd32_pointer_t linux32___sysctlp_t;
typedef netbsd32_pointer_t linux32_direntp_t;
typedef netbsd32_pointer_t linux32_dirent64p_t;
typedef netbsd32_pointer_t linux32_timep_t;
typedef netbsd32_pointer_t linux32_tmsp_t;
typedef netbsd32_pointer_t linux32_sched_paramp_t;
typedef netbsd32_pointer_t linux32_utimbufp_t;
typedef netbsd32_pointer_t linux32_oldold_utsnamep_t;
typedef netbsd32_pointer_t linux32_uid16p_t;
typedef netbsd32_pointer_t linux32_gid16p_t;
typedef netbsd32_pointer_t linux32_oldselectp_t;
typedef netbsd32_pointer_t linux32_sysinfop_t;
typedef netbsd32_pointer_t linux32_oldutsnamep_t;

struct linux32_sysctl {
	netbsd32_intp name;
	int nlen;
	netbsd32_voidp oldval;
	netbsd32_size_tp oldlenp;
	netbsd32_voidp newval;
	netbsd32_size_t newlen;
	unsigned int0[4];
};

struct linux32_tms {
	linux32_clock_t ltms32_utime;
	linux32_clock_t ltms32_stime;
	linux32_clock_t ltms32_cutime;
	linux32_clock_t ltms32_cstime;
};

struct linux32_oldselect {
        int nfds;
        netbsd32_fd_setp_t readfds;
        netbsd32_fd_setp_t writefds;
        netbsd32_fd_setp_t exceptfds;
        netbsd32_timevalp_t timeout;
};

struct linux32_sysinfo {
        netbsd32_long uptime;
        netbsd32_u_long loads[3];
        netbsd32_u_long totalram;
        netbsd32_u_long freeram;
        netbsd32_u_long sharedram;
        netbsd32_u_long bufferram; 
        netbsd32_u_long totalswap;
        netbsd32_u_long freeswap;
        unsigned short procs; 
        netbsd32_u_long totalbig;
        netbsd32_u_long freebig;
        unsigned int mem_unit;
        char _f[20-2*sizeof(netbsd32_long)-sizeof(int)];	
};

#endif /* !_LINUX32_TYPES_H */
