/*	$NetBSD: exec_conf.c,v 1.95 2007/12/31 13:39:03 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: exec_conf.c,v 1.95 2007/12/31 13:39:03 ad Exp $");

#include "opt_execfmt.h"
#include "opt_compat_freebsd.h"
#include "opt_compat_linux.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_irix.h"
#include "opt_compat_sunos.h"
#include "opt_compat_m68k4k.h"
#include "opt_compat_mach.h"
#include "opt_compat_darwin.h"
#include "opt_compat_svr4.h"
#include "opt_compat_netbsd32.h"
#include "opt_compat_linux32.h"
#include "opt_compat_aout_m68k.h"
#include "opt_compat_vax1k.h"
#include "opt_compat_pecoff.h"
#include "opt_compat_osf1.h"
#include "opt_compat_ultrix.h"
#include "opt_compat_netbsd.h"
#include "opt_coredump.h"

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
int ELF32NAME2(netbsd,probe)(struct lwp *, struct exec_package *,
    void *, char *, vaddr_t *);
#endif
#ifdef EXEC_ELF64
int ELF64NAME2(netbsd,probe)(struct lwp *, struct exec_package *,
    void *, char *, vaddr_t *);
#endif

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))
#define ELF64_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), \
    sizeof(Elf64_Addr)) + MAXPATHLEN + ALIGN(1))

/*
 * Compatibility with old ELF binaries without NetBSD note.
 * Generic ELF executable kernel support was added in NetBSD 1.1.
 * The NetBSD note was introduced in NetBSD 1.3 together with initial
 * ELF shared library support.
 */
#ifndef EXEC_ELF_NOTELESS
#  if defined(COMPAT_11) || defined(COMPAT_12)
#    define EXEC_ELF_NOTELESS		1
#  endif
#endif /* !EXEC_ELF_NOTELESS */

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

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_exec.h>
#endif

#ifdef COMPAT_FREEBSD
#include <compat/freebsd/freebsd_exec.h>
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
#ifdef COMPAT_LINUX32
#include <compat/linux32/common/linux32_exec.h>
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
#ifdef COMPAT_AOUT_M68K
extern const struct emul emul_netbsd_aoutm68k;
#endif

const struct execsw execsw_builtin[] = {
#ifdef EXEC_SCRIPT
	/* Shell scripts */
	{ SCRIPT_HDR_SIZE,
	  exec_script_makecmds,
	  { NULL },
	  NULL,
	  EXECSW_PRIO_ANY,
	  0,
	  NULL,
	  NULL,
	  NULL,
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#else /* !COMPAT_NETBSD32 */

	/* Native a.out */
	{ sizeof(struct exec),
	  exec_aout_makecmds,
	  { NULL },
#if defined(COMPAT_AOUT_M68K)
	  &emul_netbsd_aoutm68k,
#else
	  &emul_netbsd,
#endif
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif /* !COMPAT_NETBSD32 */
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },

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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
	  ELF32_AUXSIZE,
	  netbsd32_elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },		/* XXX XXX XXX */
	  /* This one should go first so it matches instead of native */

#ifdef COMPAT_LINUX32 
        /* Linux Elf32 on 64-bit */
        { sizeof (Elf32_Ehdr),
          exec_elf32_makecmds,
          { ELF32NAME2(linux32,probe) },
          &emul_linux32,
          EXECSW_PRIO_FIRST, 
	  LINUX32_ELF_AUX_ARGSIZ,
          linux32_elf32_copyargs,
          NULL,
#ifdef COREDUMP
          coredump_elf32,
#else
	  NULL,
#endif
          exec_setup_stack },           /* XXX XXX XXX */
          /* This one should go first so it matches instead of native */
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
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },	/* XXX XXX XXX */
	  /* This one should go first so it matches instead of native */
#endif

#if 0
#if EXEC_ELF_NOTELESS
	/* Generic compat Elf32 -- run as compat NetBSD Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(netbsd32,probe_noteless) },
	  &emul_netbsd32,
	  EXECSW_PRIO_FIRST,
	  ELF32_AUXSIZE,
	  netbsd32_elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },		/* XXX XXX XXX */
#endif
#endif
#else /* !COMPAT_NETBSD32 */

	/* Native Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(netbsd,probe) },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },

#ifdef COMPAT_FREEBSD
	/* FreeBSD Elf32 (probe not 64-bit safe) */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(freebsd,probe) },
	  &emul_freebsd,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#ifdef COMPAT_LINUX
	/* Linux Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(linux,probe) },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  linux_elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  linux_exec_setup_stack },
#endif

#ifdef COMPAT_IRIX
	/* IRIX Elf32 n32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(irix,probe_n32) },
	  &emul_irix,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  irix_n32_setregs,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },

	/* IRIX Elf32 o32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(irix,probe_o32) },
	  &emul_irix,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#ifdef COMPAT_SVR4
	/* SVR4 Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(svr4,probe) },
	  &emul_svr4,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#ifdef COMPAT_IBCS2
	/* SCO Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ELF32NAME2(ibcs2,probe) },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#if EXEC_ELF_NOTELESS
	/* Generic Elf32 -- run at NetBSD Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_LAST,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf32,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif
#endif /* !COMPAT_NETBSD32 */
#endif /* EXEC_ELF32 */

#ifdef EXEC_ELF64
	/* Native Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { ELF64NAME2(netbsd,probe) },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  ELF64_AUXSIZE,
	  elf64_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf64,
#else
	  NULL,
#endif
	  exec_setup_stack },

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
#ifdef COREDUMP
	  coredump_elf64,
#else
	  NULL,
#endif
	  linux_exec_setup_stack },
#endif

#ifdef COMPAT_SVR4
	/* SVR4 Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { ELF64NAME2(svr4,probe) },
	  &emul_svr4,
	  EXECSW_PRIO_ANY,
	  ELF64_AUXSIZE,
	  elf64_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf64,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#if EXEC_ELF_NOTELESS
	/* Generic Elf64 -- run at NetBSD Elf64 */
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  ELF64_AUXSIZE,
	  elf64_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_elf64,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif
#endif /* EXEC_ELF64 */

#if defined(EXEC_MACHO)
#ifdef COMPAT_DARWIN
	/* Darwin Mach-O (native word size) */
	{ sizeof (struct exec_macho_fat_header),
	  exec_macho_makecmds,
	  { .mach_probe_func = exec_darwin_probe },
	  &emul_darwin,
	  EXECSW_PRIO_ANY,
	  MAXPATHLEN + 1,
	  exec_darwin_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  darwin_exec_setup_stack },
#endif

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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  linux_exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  ibcs2_exec_setup_stack },

	/* iBCS2 x.out (native word size) */
	{ XOUT_HDR_SIZE,
	  exec_ibcs2_xout_makecmds,
	  { NULL },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  ibcs2_exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
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
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif

#ifdef COMPAT_PECOFF
	/* Win32/WinCE PE/COFF (native word size) */
	{ PECOFF_HDR_SIZE,
	  exec_pecoff_makecmds,
	  { NULL },
	  &emul_pecoff,
	  EXECSW_PRIO_ANY,
	  howmany(sizeof(struct pecoff_args), sizeof(char *)),
	  pecoff_copyargs,
	  NULL,
#ifdef COREDUMP
	  coredump_netbsd,
#else
	  NULL,
#endif
	  exec_setup_stack },
#endif
};
int nexecs_builtin = (sizeof(execsw_builtin) / sizeof(struct execsw));
