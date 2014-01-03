/*	$NetBSD: init_main.c,v 1.7 2014/01/03 06:37:13 tsutsui Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)init_main.c	8.2 (Berkeley) 8/15/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)init_main.c	8.2 (Berkeley) 8/15/93
 */

#include <sys/param.h>
#include <sys/boot_flag.h>
#include <machine/cpu.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/stinger.h>
#include <luna68k/stand/boot/romvec.h>
#include <luna68k/stand/boot/status.h>
#include <lib/libsa/loadfile.h>
#ifdef SUPPORT_ETHERNET
#include <lib/libsa/dev_net.h>
#endif

static int get_plane_numbers(void);
static int reorder_dipsw(int);

int cpuspeed;	/* for DELAY() macro */
int hz = 60;
int machtype;
char default_file[64];

#define	VERS_LOCAL	"Phase-31"

int nplane;

/* KIFF */

struct KernInter  KIFF;
struct KernInter *kiff = &KIFF;

/* for command parser */

#define BUFFSIZE 100
#define MAXARGS  30

char buffer[BUFFSIZE];

int   argc;
char *argv[MAXARGS];

#define BOOT_TIMEOUT 10
int boot_timeout = BOOT_TIMEOUT;

static const char prompt[] = "boot> ";

void
main(void)
{
	int i, status = 0;
	const char *machstr;
	const char *cp;
	char bootarg[64];
	bool netboot = false;
	int unit, part;

	/*
	 * Initialize the console before we print anything out.
	 */
	if (cputype == CPU_68030) {
		machtype = LUNA_I;
		machstr  = "LUNA-I";
		cpuspeed = MHZ_25;
		hz = 60;
		memcpy(bootarg, (char *)*RVPtr->vec53, sizeof(bootarg));

		/* check netboot */
		for (i = 0, cp = bootarg; i < sizeof(bootarg); i++, cp++) {
			if (*cp == '\0')
				break;
			if (*cp == 'E' && memcmp("ENADDR=", cp, 7) == 0) {
				netboot = true;
				break;
			}
		}
	} else {
		machtype = LUNA_II;
		machstr  = "LUNA-II";
		cpuspeed = MHZ_25 * 2;	/* XXX */
		hz = 100;
		memcpy(bootarg, (char *)*RVPtr->vec02, sizeof(bootarg));

		/* LUNA-II's boot monitor doesn't support netboot */
	}

	nplane   = get_plane_numbers();

	cninit();

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (based on Stinger ver 0.0 [%s])\n", VERS_LOCAL);
	printf("\n");

	kiff->maxaddr = (void *) (ROM_memsize -1);
	kiff->dipsw   = ~((dipsw2 << 8) | dipsw1) & 0xFFFF;
	kiff->plane   = nplane;

	i = (int) kiff->maxaddr + 1;
	printf("Machine model   = %s\n", machstr);
	printf("Physical Memory = 0x%x  ", i);
	i >>= 20;
	printf("(%d MB)\n", i);
	printf("\n");

	/*
	 * IO configuration
	 */

#ifdef SUPPORT_ETHERNET
	try_bootp = 1;
#endif

	find_devs();
	configure();
	printf("\n");

	unit = 0;	/* XXX should parse monitor's Boot-file constant */
	part = 0;
	snprintf(default_file, sizeof(default_file),
	    "%s(%d,%d)%s", netboot ? "le" : "sd", unit, part, "netbsd");

	howto = reorder_dipsw(dipsw2);

	if ((howto & 0xFE) == 0) {
		char c;

		printf("Press return to boot now,"
		    " any other key for boot menu\n");
		printf("booting %s - starting in ", default_file);
		c = awaitkey("%d seconds. ", boot_timeout, true);
		if (c == '\r' || c == '\n' || c == 0) {
			printf("auto-boot %s\n", default_file);
			bootnetbsd(default_file);
		}
	}

	/*
	 * Main Loop
	 */

	printf("type \"help\" for help.\n");

	do {
		memset(buffer, 0, BUFFSIZE);
		if (getline(prompt, buffer) > 0) {
			argc = getargs(buffer, argv, sizeof(argv)/sizeof(char *));

			status = parse(argc, argv);
			if (status == ST_NOTFOUND)
				printf("Command \"%s\" is not found !!\n", argv[0]);
		}
	} while(status != ST_EXIT);

	exit(0);
}

int
get_plane_numbers(void)
{
	int r = ROM_plane;
	int n = 0;

	for (; r ; r >>= 1)
		if (r & 0x1)
			n++;

	return(n);
}

int
reorder_dipsw(int dipsw)
{
	int i, sw = 0;

	for (i = 0; i < 8; i++) {
		if ((dipsw & 0x01) == 0)
			sw += 1;

		if (i == 7)
			break;

		sw <<= 1;
		dipsw >>= 1;
	}

	return(sw);
}
