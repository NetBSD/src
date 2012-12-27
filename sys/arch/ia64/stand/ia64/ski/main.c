/*	$NetBSD: main.c,v 1.4 2012/12/27 20:21:51 martin Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * Copyright (c) 1998,2000 Doug Rabson <dfr@freebsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include "bootstrap.h"
#include "libski.h"

static int command_quit(int argc, char *argv[]);

extern char bootprog_name[];
extern char bootprog_rev[];
int skifs_dev_init(void);

struct bootblk_command commands[] = {
	COMMON_COMMANDS,
	{ "quit",	"exit the loader",	command_quit },
	{ NULL,		NULL,			NULL	     },
};

struct ski_devdesc	currdev;	/* our current device */
struct arch_switch	archsw;		/* MI/MD interface boundary */

static int
ski_autoload(void)
{

	return (0);
}

void
ski_main(void)
{
	static char malloc[512*1024];
	int i;

	/* 
	 * initialise the heap as early as possible.  Once this is done,
	 * alloc() is usable. The stack is buried inside us, so this is
	 * safe.
	 */
	setheap((void *)malloc, (void *)(malloc + 512*1024));

	/* 
	 * XXX Chicken-and-egg problem; we want to have console output
	 * early, but some console attributes may depend on reading from
	 * eg. the boot device, which we can't do yet.  We can use
	 * printf() etc. once this is done.
	 */
	cons_probe();

	/*
	 * Initialise the block cache XXX: Fixme: do we need the bcache ?
	 */
	/*	bcache_init(32, 512);	*/	/* 16k XXX tune this */

	/* Initialise skifs.c */

	skifs_dev_init();

	printf("\n");
	printf("%s, Revision %s\n", bootprog_name, bootprog_rev);
#if 0
	printf("Memory: %ld k\n", memsize() / 1024);
#endif
    
	/* XXX presumes that biosdisk is first in devsw */
	currdev.d_dev = &devsw[0];
	currdev.d_type = DEVT_DISK;
	currdev.d_kind.skidisk.unit = 0;
	/* XXX should be able to detect this, default to autoprobe */
	currdev.d_kind.skidisk.slice = -1;
	/* default to 'a' */
	currdev.d_kind.skidisk.partition = 0;

#if 0
	/* Create arc-specific variables */
	bootfile = GetEnvironmentVariable(ARCENV_BOOTFILE);
	if (bootfile)
		setenv("bootfile", bootfile, 1);
#endif

	env_setenv("currdev", EV_VOLATILE, ski_fmtdev(&currdev),
	    (ev_sethook_t *) ski_setcurrdev, env_nounset);
	env_setenv("loaddev", EV_VOLATILE, ski_fmtdev(&currdev), env_noset,
	    env_nounset);

	setenv("LINES", "24", 1);	/* optional */
    
	archsw.arch_autoload = ski_autoload;
	archsw.arch_getdev = ski_getdev;
	archsw.arch_copyin = ski_copyin;
	archsw.arch_copyout = ski_copyout;
	archsw.arch_readin = ski_readin;

	interact();			/* doesn't return */

	exit(0);
}

static int
command_quit(int argc, char *argv[])
{
	exit(0);
	return (CMD_OK);
}
