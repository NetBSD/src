/*	$NetBSD: exec_elf32.c,v 1.137 2008/11/19 18:36:06 ad Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
__KERNEL_RCSID(0, "$NetBSD: exec_elf32.c,v 1.137 2008/11/19 18:36:06 ad Exp $");

#define	ELFSIZE	32

#include "exec_elf.c"

#include <sys/module.h>

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))

#ifdef COREDUMP
#define	DEP	"coredump"
#else
#define	DEP	NULL
#endif

MODULE(MODULE_CLASS_MISC, exec_elf32, DEP);

static struct execsw exec_elf32_execsw[] = {
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { netbsd_elf32_probe },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },
#if EXEC_ELF_NOTELESS
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { NULL },
	  &emul_netbsd,
	  EXECSW_PRIO_LAST,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },
#endif
};

static int
exec_elf32_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(exec_elf32_execsw,
		    __arraycount(exec_elf32_execsw));

	case MODULE_CMD_FINI:
		return exec_remove(exec_elf32_execsw,
		    __arraycount(exec_elf32_execsw));

	default:
		return ENOTTY;
        }
}
