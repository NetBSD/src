/*	$NetBSD: exec_conf.c,v 1.56 2001/05/15 02:00:12 lukem Exp $	*/

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

#include "opt_execfmt.h"
#include "opt_compat_freebsd.h"
#include "opt_compat_linux.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_sunos.h"
#include "opt_compat_hpux.h"
#include "opt_compat_m68k4k.h"
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
	{ MAXINTERP, exec_script_makecmds, { NULL },
	  NULL, EXECSW_PRIO_ANY, }, /* shell scripts */
#endif
#ifdef EXEC_AOUT
#ifdef COMPAT_NETBSD32
	{ sizeof(struct netbsd32_exec), exec_netbsd32_makecmds, { NULL },
	  &emul_netbsd32, EXECSW_PRIO_FIRST,
	  0, netbsd32_copyargs, netbsd32_setregs }, /* sparc 32 bit */
#endif
	{ sizeof(struct exec), exec_aout_makecmds, { NULL },
#ifdef COMPAT_AOUT
	  &emul_netbsd_aout,
#elif defined(COMPAT_AOUT_M68K)
	  &emul_netbsd_aoutm68k,
#else
	  &emul_netbsd,
#endif /* COMPAT_AOUT */
	  EXECSW_PRIO_ANY,
	  0, copyargs, setregs },	/* a.out binaries */
#endif
#ifdef EXEC_COFF
	{ COFF_HDR_SIZE, exec_coff_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  0, copyargs, setregs },	/* coff binaries */
#endif
#ifdef EXEC_ECOFF
#ifdef COMPAT_OSF1
	{ ECOFF_HDR_SIZE, exec_ecoff_makecmds,
	  { .ecoff_probe_func = osf1_exec_ecoff_probe },
	  &emul_osf1, EXECSW_PRIO_ANY,
  	  howmany(OSF1_MAX_AUX_ENTRIES * sizeof (struct osf1_auxv) +
	    2 * (MAXPATHLEN + 1), sizeof (char *)), /* exec & loader names */
	  osf1_copyargs, cpu_exec_ecoff_setregs }, /* OSF1 ecoff binaries */
#endif /* COMPAT_OSF1 */
	{ ECOFF_HDR_SIZE, exec_ecoff_makecmds,
	  { .ecoff_probe_func = cpu_exec_ecoff_probe },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  0, copyargs, cpu_exec_ecoff_setregs },	/* ecoff binaries */
#ifdef COMPAT_ULTRIX
	{ ECOFF_HDR_SIZE, exec_ecoff_makecmds,
	  { .ecoff_probe_func = ultrix_exec_ecoff_probe },
	  &emul_ultrix, EXECSW_PRIO_LAST, /* XXX probe func alw. succeeds */
  	  0, copyargs, cpu_exec_ecoff_setregs }, /* Ultrix ecoff binaries */
#endif /* COMPAT_ULTRIX */
#endif
#ifdef EXEC_ELF32
	/* 32bit ELF bins */
#ifdef COMPAT_NETBSD32
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(netbsd32,probe) },
	  &emul_netbsd32, EXECSW_PRIO_FIRST,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  netbsd32_elf32_copyargs, netbsd32_setregs }, /* NetBSD32 32bit ELF bins */
	  /* This one should go first so it matches instead of netbsd */
#endif
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(netbsd,probe) },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  elf32_copyargs, setregs },	/* NetBSD 32bit ELF bins */
#ifdef COMPAT_FREEBSD
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(freebsd,probe) },
	  &emul_freebsd, EXECSW_PRIO_ANY,
	  FREEBSD_ELF_AUX_ARGSIZ,
	  elf32_copyargs, freebsd_setregs },	/* FreeBSD 32bit ELF bins (not 64bit safe )*/
#endif
#ifdef COMPAT_LINUX
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(linux,probe) },
	  &emul_linux, EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  LINUX_COPYARGS_FUNCTION, setregs },	/* Linux 32bit ELF bins */
#endif
#ifdef COMPAT_SVR4_32
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(svr4_32,probe) },
	  &emul_svr4_32, EXECSW_PRIO_ANY,
	  SVR4_32_AUX_ARGSIZ,
	  svr4_32_copyargs, svr4_32_setregs },	/* SVR4 32bit ELF bins (not 64bit safe) */
#endif
#ifdef COMPAT_SVR4
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(svr4,probe) },
	  &emul_svr4, EXECSW_PRIO_ANY,
	  SVR4_AUX_ARGSIZ,
	  svr4_copyargs, svr4_setregs },	/* SVR4 32bit ELF bins (not 64bit safe) */
#endif
#ifdef COMPAT_IBCS2
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds,
	  { ELF32NAME2(ibcs2,probe) },
	  &emul_ibcs2, EXECSW_PRIO_ANY,
	  IBCS2_ELF_AUX_ARGSIZ, elf32_copyargs, setregs },
				/* SCO 32bit ELF bins (not 64bit safe) */
#endif
	{ sizeof (Elf32_Ehdr), exec_elf32_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_LAST,	/* catch all entry */
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  elf32_copyargs, setregs },	/* NetBSD 32bit ELF bins */
#endif /* EXEC_ELF32 */
#ifdef EXEC_ELF64
	/* 64bit ELF bins */
	{ sizeof (Elf64_Ehdr), exec_elf64_makecmds,
	  { ELF64NAME2(netbsd,probe) },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), sizeof (Elf64_Addr)),
	  elf64_copyargs, setregs }, /* NetBSD 64bit ELF bins */
#ifdef COMPAT_LINUX
	{ sizeof (Elf64_Ehdr), exec_elf64_makecmds,
	  { ELF64NAME2(linux,probe) },
	  &emul_linux, EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  linux_elf64_copyargs, setregs }, /* Linux 64bit ELF bins */
#endif
#ifdef COMPAT_SVR4
	{ sizeof (Elf64_Ehdr), exec_elf64_makecmds,
	  { ELF64NAME2(svr4,probe) },
	  &emul_svr4, EXECSW_PRIO_ANY,
	  SVR4_AUX_ARGSIZ64,
	  svr4_copyargs64, svr4_setregs },	/* SVR4 64bit ELF bins (not 64bit safe) */
#endif
	{ sizeof (Elf64_Ehdr), exec_elf64_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), sizeof (Elf64_Addr)),
	  elf64_copyargs, setregs }, /* NetBSD 64bit ELF bins */
#endif /* EXEC_ELF64 */
#ifdef COMPAT_SUNOS
#ifdef COMPAT_NETBSD32
	{ SUNOS32_AOUT_HDR_SIZE, exec_sunos32_aout_makecmds, { NULL },
	  &emul_sunos, EXECSW_PRIO_ANY,
	  0, netbsd32_copyargs, netbsd32_setregs }, /* SunOS a.out, 64-bit kernel */
#else
	{ SUNOS_AOUT_HDR_SIZE, exec_sunos_aout_makecmds, { NULL },
	  &emul_sunos, EXECSW_PRIO_ANY,
	  0, copyargs, setregs }, /* SunOS a.out */
#endif
#endif
#if defined(COMPAT_LINUX) && defined(EXEC_AOUT)
	{ LINUX_AOUT_HDR_SIZE, exec_linux_aout_makecmds, { NULL },
	  &emul_linux, EXECSW_PRIO_ANY,
	  LINUX_AOUT_AUX_ARGSIZ, linux_aout_copyargs, linux_setregs }, /* linux a.out */
#endif
#ifdef COMPAT_IBCS2
	{ COFF_HDR_SIZE, exec_ibcs2_coff_makecmds, { NULL },
	  &emul_ibcs2, EXECSW_PRIO_ANY,
	  0, copyargs, ibcs2_setregs },	/* coff binaries */
	{ XOUT_HDR_SIZE, exec_ibcs2_xout_makecmds, { NULL },
	  &emul_ibcs2, EXECSW_PRIO_ANY,
	  0, copyargs, ibcs2_setregs },	/* x.out binaries */
#endif
#if defined(COMPAT_FREEBSD) && defined(EXEC_AOUT)
	{ FREEBSD_AOUT_HDR_SIZE, exec_freebsd_aout_makecmds, { NULL },
	  &emul_freebsd, EXECSW_PRIO_ANY,
	  0, copyargs, freebsd_setregs },	/* a.out */
#endif
#ifdef COMPAT_HPUX
	{ HPUX_EXEC_HDR_SIZE, exec_hpux_makecmds, { NULL },
	  &emul_hpux, EXECSW_PRIO_ANY,
	  0, copyargs, hpux_setregs },	/* HP-UX a.out */
#endif
#ifdef COMPAT_M68K4K
	{ sizeof(struct exec), exec_m68k4k_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  0, copyargs, setregs },	/* m68k4k a.out */
#endif
#ifdef COMPAT_VAX1K
	{ sizeof(struct exec), exec_vax1k_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_ANY,
	  0, copyargs, setregs },	/* vax1k a.out */
#endif
#ifdef COMPAT_PECOFF
	{ sizeof(struct exec), exec_pecoff_makecmds, { NULL },
	  &emul_netbsd, EXECSW_PRIO_ANY, /* XXX emul_pecoff once it's different */
	  howmany(sizeof(struct pecoff_args), sizeof(char *)),
	  pecoff_copyargs, setregs },	/* Win32/CE PE/COFF */
#endif
};
int nexecs_builtin = (sizeof(execsw_builtin) / sizeof(struct execsw));
