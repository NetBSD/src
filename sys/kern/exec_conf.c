/*	$NetBSD: exec_conf.c,v 1.55.2.7 2002/02/28 04:14:42 nathanw Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exec_conf.c,v 1.55.2.7 2002/02/28 04:14:42 nathanw Exp $");

#include "opt_execfmt.h"
#include "opt_compat_freebsd.h"
#include "opt_compat_linux.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_irix.h"
#include "opt_compat_sunos.h"
#include "opt_compat_hpux.h"
#include "opt_compat_m68k4k.h"
#include "opt_compat_mach.h"
#include "opt_compat_svr4.h"
#include "opt_compat_netbsd32.h"
#include "opt_compat_aout.h"
#include "opt_compat_aout_m68k.h"
#include "opt_compat_vax1k.h"
#include "opt_compat_pecoff.h"
#include "opt_compat_osf1.h"
#include "opt_compat_ultrix.h"

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/signalvar.h>

#ifdef EXEC_SCRIPT
#include <sys/exec_script.h>
#endif

#ifdef EXEC_AOUT
/*#include <sys/exec_aout.h> -- automatically pulled in */
#endif

#ifdef EXEC_COFF
#include <sys/exec_coff.h>
#endif

#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>
#endif

#if defined(EXEC_ELF32) || defined(EXEC_ELF64)
#include <sys/exec_elf.h>
#define	CONCAT(x,y)	__CONCAT(x,y)
#define	ELF32NAME(x)	CONCAT(elf,CONCAT(32,CONCAT(_,x)))
#define	ELF32NAME2(x,y)	CONCAT(x,CONCAT(_elf32_,y))
#define	ELF64NAME(x)	CONCAT(elf,CONCAT(64,CONCAT(_,x)))
#define	ELF64NAME2(x,y)	CONCAT(x,CONCAT(_elf64_,y))
#ifdef EXEC_ELF32
int ELF32NAME2(netbsd,probe)(struct proc *, struct exec_package *,
    void *, char *, vaddr_t *);
#endif
#ifdef EXEC_ELF64
int ELF64NAME2(netbsd,probe)(struct proc *, struct exec_package *,
    void *, char *, vaddr_t *);
#endif
#endif /* ELF32 || ELF64 */

#ifdef EXEC_MACHO
#include <sys/exec_macho.h>
#endif

#ifdef COMPAT_SUNOS
#include <compat/sunos/sunos_exec.h>
#endif

#if defined(COMPAT_SVR4) || defined(COMPAT_SVR4_32)
#include <compat/svr4/svr4_exec.h>
#endif

#ifdef COMPAT_SVR4_32
#include <compat/svr4_32/svr4_32_exec.h>
#endif

#ifdef COMPAT_IBCS2
#include <sys/exec_coff.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <machine/ibcs2_machdep.h>
#endif

#ifdef COMPAT_LINUX
#include <compat/linux/common/linux_exec.h>
#endif

#ifdef COMPAT_FREEBSD
#include <compat/freebsd/freebsd_exec.h>
#endif

#ifdef COMPAT_HPUX
#include <compat/hpux/hpux_exec.h>
#endif

#ifdef COMPAT_M68K4K
#include <compat/m68k4k/m68k4k_exec.h>
#endif

#ifdef COMPAT_MACH
#include <compat/mach/mach_exec.h>
#endif

#ifdef COMPAT_IRIX
#include <compat/irix/irix_exec.h>
#endif

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_exec.h>
#ifdef COMPAT_SUNOS
#include <compat/sunos32/sunos32_exec.h>
#endif
#endif

#ifdef COMPAT_VAX1K
#include <compat/vax1k/vax1k_exec.h>
#endif

#ifdef COMPAT_PECOFF
#include <sys/exec_coff.h>
#include <compat/pecoff/pecoff_exec.h>
#endif

#ifdef COMPAT_OSF1
#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_exec.h>
#endif

#ifdef COMPAT_ULTRIX
#include <compat/ultrix/ultrix_exec.h>
#endif

extern const struct emul emul_netbsd;
#ifdef COMPAT_AOUT
extern const struct emul emul_netbsd_aout;
#endif
#ifdef COMPAT_AOUT_M68K
extern const struct emul emul_netbsd_aoutm68k;
#endif

const struct execsw execsw_builtin[] = {
#ifdef EXEC_SCRIPT
	/* Shell scripts */
	{ MAXINTERP,
	  exec_script_makecmds,
	  { NULL },
	  NULL,
	  EXECSW_PRIO_ANY, },
#endif /* EXEC_SCRIPT */

#ifdef EXEC_AOUT
#ifdef COMPAT_NETBSD32
	/* 32-bit NetBSD a.out on 64-bit */
	{ sizeof(struct netbsd32_exec),
	  exec_netbsd32_makecmds,
	  { NULL },
	  &emul_netbsd32,
	  EXECSW_PRIO_FIRST,
	  0,
	  netbsd32_copyargs,
	  NULL,
	  coredump_netbsd32 },
#endif

	/* Native a.out */
	{ sizeof(struct exec),
	  exec_aout_makecmds,
	  { NULL },
#ifdef COMPAT_AOUT
	  &emul_netbsd_aout,
#elif defined(COMPAT_AOUT_M68K)
	  &emul_netbsd_aoutm68k,
#else
	  &emul_netbsd,
#endif
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif /* EXEC_AOUT */

#ifdef EXEC_COFF
	/* Native COFF */
	{ COFF_HDR_SIZE,
	  exec_coff_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif /* EXEC_COFF */

#ifdef EXEC_ECOFF
#ifdef COMPAT_OSF1
	/* OSF/1 (Digital Unix) ECOFF */
	{ ECOFF_HDR_SIZE,
	  exec_ecoff_makecmds,
	  { .ecoff_probe_func = osf1_exec_ecoff_probe },
	  &emul_osf1,
	  EXECSW_PRIO_ANY,
  	  howmany(OSF1_MAX_AUX_ENTRIES * sizeof (struct osf1_auxv) +
	    2 * (MAXPATHLEN + 1), sizeof (char *)), /* exec & loader names */
	  osf1_copyargs,
	  cpu_exec_ecoff_setregs,
	  coredump_netbsd },
#endif

	/* Native ECOFF */
	{ ECOFF_HDR_SIZE,
	  exec_ecoff_makecmds,
	  { .ecoff_probe_func = cpu_exec_ecoff_probe },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  cpu_exec_ecoff_setregs,
	  coredump_netbsd},

#ifdef COMPAT_ULTRIX
	/* Ultrix ECOFF */
	{ ECOFF_HDR_SIZE,
	  exec_ecoff_makecmds,
	  { .ecoff_probe_func = ultrix_exec_ecoff_probe },
	  &emul_ultrix,
	  EXECSW_PRIO_LAST, /* XXX probe func alw. succeeds */
  	  0,
  	  copyargs,
  	  cpu_exec_ecoff_setregs,
	  coredump_netbsd },
#endif
#endif /* EXEC_ECOFF */

#ifdef EXEC_ELF32
#ifdef COMPAT_NETBSD32
	/* Elf32 NetBSD on 64-bit */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(netbsd32,probe) },
	  &emul_netbsd32,
	  EXECSW_PRIO_FIRST,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  netbsd32_elf32_copyargs,
	  NULL,
	  coredump_netbsd32 },		/* XXX XXX XXX */
	  /* This one should go first so it matches instead of native */
#endif

	/* Native Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(netbsd,probe) },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  elf32_copyargs,
	  NULL,
	  coredump_elf32 },

#ifdef COMPAT_FREEBSD
	/* FreeBSD Elf32 (probe not 64-bit safe) */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(freebsd,probe) },
	  &emul_freebsd,
	  EXECSW_PRIO_ANY,
	  FREEBSD_ELF_AUX_ARGSIZ,
	  elf32_copyargs,
	  NULL,
	  coredump_elf32 },
#endif

#ifdef COMPAT_LINUX
	/* Linux Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(linux,probe) },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  LINUX_COPYARGS_FUNCTION,
	  NULL,
	  coredump_elf32 },
#endif

#ifdef COMPAT_IRIX 
	/* IRIX Elf32 n32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(irix,probe_n32) },
	  &emul_irix_n32,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  NULL,
	  coredump_netbsd },

	/* IRIX Elf32 o32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(irix,probe_o32) },
	  &emul_irix_o32,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  NULL,
	  coredump_elf32 },
#endif

#ifdef COMPAT_SVR4_32
	/* SVR4 Elf32 on 64-bit */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(svr4_32,probe) },
	  &emul_svr4_32,
	  EXECSW_PRIO_ANY,
	  SVR4_32_AUX_ARGSIZ,
	  svr4_32_copyargs,
	  NULL,
	  coredump_netbsd32 },	/* XXX XXX XXX */
	  /* This one should go first so it matches instead of native */
#endif

#ifdef COMPAT_SVR4
	/* SVR4 Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(svr4,probe) },
	  &emul_svr4,
	  EXECSW_PRIO_ANY,
	  SVR4_AUX_ARGSIZ,
	  svr4_copyargs,
	  NULL,
	  coredump_elf32 },
#endif

#ifdef COMPAT_IBCS2
	/* SCO Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(ibcs2,probe) },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  IBCS2_ELF_AUX_ARGSIZ,
	  elf32_copyargs,
	  NULL,
	  coredump_elf32 },
#endif

#ifdef EXEC_ELF_CATCHALL
	/* Generic Elf32 -- run at NetBSD Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_LAST,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  elf32_copyargs,
	  NULL,
	  coredump_elf32 },
#endif
#endif /* EXEC_ELF32 */

#ifdef EXEC_ELF64
	/* Native Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { ELF64NAME2(netbsd,probe) },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), sizeof (Elf64_Addr)),
	  elf64_copyargs,
	  NULL,
	  coredump_elf64 },

#ifdef COMPAT_LINUX
	/* Linux Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { ELF64NAME2(linux,probe) },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  linux_elf64_copyargs,
	  NULL,
	  coredump_elf64 },
#endif

#ifdef COMPAT_SVR4
	/* SVR4 Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { ELF64NAME2(svr4,probe) },
	  &emul_svr4,
	  EXECSW_PRIO_ANY,
	  SVR4_AUX_ARGSIZ64,
	  svr4_copyargs64,
	  NULL,
	  coredump_elf64 },
#endif

#ifdef EXEC_ELF_CATCHALL
	/* Generic Elf64 -- run at NetBSD Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), sizeof (Elf64_Addr)),
	  elf64_copyargs,
	  NULL,
	  coredump_elf64 },
#endif
#endif /* EXEC_ELF64 */

#if defined(EXEC_MACHO)
#ifdef COMPAT_MACH
	/* Mach MACH-O (native word size) */
	{ sizeof (struct exec_macho_fat_header),
	  exec_macho_makecmds,
	  { .mach_probe_func = exec_mach_probe },
	  &emul_mach,
	  EXECSW_PRIO_ANY,
	  MAXPATHLEN + 1,
	  exec_mach_copyargs,
	  NULL,
	  coredump_netbsd },
#endif
#endif /* EXEC_MACHO */

#ifdef COMPAT_SUNOS
#ifdef COMPAT_NETBSD32
	/* 32-bit SunOS a.out on 64-bit */
	{ SUNOS32_AOUT_HDR_SIZE,
	  exec_sunos32_aout_makecmds,
	  { NULL },
	  &emul_sunos,
	  EXECSW_PRIO_ANY,
	  0,
	  netbsd32_copyargs,
	  NULL,
	  coredump_netbsd },
#else
	/* SunOS a.out (native word size) */
	{ SUNOS_AOUT_HDR_SIZE,
	  exec_sunos_aout_makecmds,
	  { NULL },
	  &emul_sunos,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif
#endif /* COMPAT_SUNOS */

#if defined(COMPAT_LINUX) && defined(EXEC_AOUT)
	/* Linux a.out (native word size) */
	{ LINUX_AOUT_HDR_SIZE,
	  exec_linux_aout_makecmds,
	  { NULL },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_AOUT_AUX_ARGSIZ,
	  linux_aout_copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#ifdef COMPAT_IBCS2
	/* iBCS2 COFF (native word size) */
	{ COFF_HDR_SIZE,
	  exec_ibcs2_coff_makecmds,
	  { NULL },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },

	/* iBCS2 x.out (native word size) */
	{ XOUT_HDR_SIZE,
	  exec_ibcs2_xout_makecmds,
	  { NULL },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#if defined(COMPAT_FREEBSD) && defined(EXEC_AOUT)
	/* FreeBSD a.out (native word size) */
	{ FREEBSD_AOUT_HDR_SIZE,
	  exec_freebsd_aout_makecmds,
	  { NULL },
	  &emul_freebsd,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#ifdef COMPAT_HPUX
	/* HP-UX a.out for m68k (native word size) */
	{ HPUX_EXEC_HDR_SIZE,
	  exec_hpux_makecmds,
	  { NULL },
	  &emul_hpux,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#ifdef COMPAT_M68K4K
	/* NetBSD a.out for m68k4k */
	{ sizeof(struct exec),
	  exec_m68k4k_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#ifdef COMPAT_VAX1K
	/* NetBSD vax1k a.out */
	{ sizeof(struct exec),
	  exec_vax1k_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd },
#endif

#ifdef COMPAT_PECOFF
	/* Win32/WinCE PE/COFF (native word size) */
	{ sizeof(struct exec),
	  exec_pecoff_makecmds,
	  { NULL },
	  &emul_netbsd,		/* XXX emul_pecoff once it's different */
	  EXECSW_PRIO_ANY,
	  howmany(sizeof(struct pecoff_args), sizeof(char *)),
	  pecoff_copyargs,
	  NULL,
	  coredump_netbsd },
#endif
};
int nexecs_builtin = (sizeof(execsw_builtin) / sizeof(struct execsw));
