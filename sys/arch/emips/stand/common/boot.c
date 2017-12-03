/*	$NetBSD: boot.c,v 1.1.20.2 2017/12/03 11:36:01 jdolecek Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include "common.h"
#include "bootinfo.h"
#include "start.h"

/*
 * We won't go overboard with gzip'd kernel names.  After all we can
 * still boot a gzip'd kernel called "netbsd.emips" - it doesn't need
 * the .gz suffix.
 */
char *kernelnames[] = {
	"netbsd",	"netbsd.gz",
	"netbsd.old",
	"onetbsd",
	"gennetbsd",
	"nfsnetbsd",
	NULL
};


void main (char *);
char *getboot(char *, char*);
static int devcanon(char *);

#define OPT_MAX PATH_MAX /* way overkill */

static int loadit(char *name, u_long *marks)
{
	printf("Loading: %s\n", name);
	memset(marks, 0, sizeof(*marks) * MARK_MAX);
	return (loadfile(name, marks, LOAD_ALL));
}

/*
 * The locore in start.S calls us with an 8KB stack carved after _end.
 * 
 */
void
main(char *stack_top)
{
	int autoboot = 1, win;
	char *name, **namep, *dev, *kernel;
	char bootpath[PATH_MAX], options[OPT_MAX];
	uint32_t entry;
	u_long marks[MARK_MAX];
	struct btinfo_symtab bi_syms;
	struct btinfo_bootpath bi_bpath;

	/* Init all peripherals, esp USART for printf and memory */
	init_board();

	/* On account of compression, we need a fairly large heap.
	 * To keep things simple, take one meg just below the 16 meg mark.
	 * That allows for a large kernel, and a 16MB configuration still works.
	 */
	setheap((void *)(0x81000000-(1024*1024)), (void *)0x81000000);

	/* On the BEE3 and the Giano simulator, we need a sec between the serial-line download complete
	 * and switching the serial line to PuTTY as console. Get a char to pause.
	 * This delay is also the practice on PCs so.
	 */
	Delay(200000);
	printf("Hit any char to boot..");
	(void)GetChar();

	/* print a banner */
	printf("\n");
	printf("NetBSD/emips " NETBSD_VERS " " BOOT_TYPE_NAME " Bootstrap, Revision %s\n",
	    bootprog_rev);

	/* initialise bootinfo structure early */
	bi_init(BOOTINFO_ADDR);

	/* Default is to auto-boot from the first disk */
	dev = "0/ace(0,0)/";
	kernel = kernelnames[0];
	options[0] = 0;

	win = 0;
	for (;!win;) {
	    strcpy(bootpath, dev);
	    strcat(bootpath, kernel);
	    name = getboot(bootpath,options);

	    if (name != NULL) {
	        win = (loadit(name, marks) == 0);
	    } else if (autoboot)
	        break;
	    autoboot = 0;
	}

	if (!win) {
		for (namep = kernelnames, win = 0; *namep != NULL && !win;
		    namep++) {
			kernel = *namep;
			strcpy(bootpath, dev);
			strcat(bootpath, kernel);
			win = (loadit(bootpath, marks) == 0);
			if (win) {
				name = bootpath;
			}
		}
	}
	if (!win)
		goto fail;

	strncpy(bi_bpath.bootpath, name/*kernel?*/, BTINFO_BOOTPATH_LEN);
	bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));

	entry = marks[MARK_ENTRY];
	bi_syms.nsym = marks[MARK_NSYM];
	bi_syms.ssym = marks[MARK_SYM];
	bi_syms.esym = marks[MARK_END];
	bi_add(&bi_syms, BTINFO_SYMTAB, sizeof(bi_syms));

	printf("Starting at 0x%x\n\n", entry);
	call_kernel(entry, name, options, BOOTINFO_MAGIC, bootinfo);
	(void)printf("KERNEL RETURNED!\n");

fail:
	(void)printf("Boot failed!  Halting...\n");
}

static inline int
parse(char *cmd, char *kname, char *optarg)
{
	char *arg = cmd;
	char *ep, *p;
	int c, i;

	while ((c = *arg++)) {
	    /* skip leading blanks */
	    if (c == ' ' || c == '\t' || c == '\n')
	        continue;
	    /* find separator, or eol */
	    for (p = arg; *p && *p != '\n' && *p != ' ' && *p != '\t'; p++);
	    ep = p;
	    /* trim if separator */
	    if (*p)
	        *p++ = 0;
	    /* token is either "-opts" or "kernelname" */
	    if (c == '-') {
	        /* no overflow because whole line same length as optarg anyways */
	        while ((c = *arg++)) {
	            *optarg++ = c;
	        }
	        *optarg = 0;
	    } else {
	        arg--;
	        if ((i = ep - arg)) {
	            if ((size_t)i >= PATH_MAX)
	                return -1;
	            memcpy(kname, arg, i + 1);
	        }
	    }
	    arg = p;
	}
	return 0;
}

/* String returned is zero-terminated and at most PATH_MAX chars */
static inline void
getstr(char *cmd, int c)
{
	char *s;

	s = cmd;
	if (c == 0)
	    c = GetChar();
	for (;;) {
	    switch (c) {
	    case 0:
	        break;
	    case '\177':
	    case '\b':
	        if (s > cmd) {
	            s--;
	            printf("\b \b");
	        }
	        break;
	    case '\n':
	    case '\r':
	        *s = 0;
	        return;
	    default:
	        if ((s - cmd) < (PATH_MAX - 1))
	            *s++ = c;
	        xputchar(c);
	    }
	    c = GetChar();
	}
}

char *getboot(char *kname, char* optarg)
{
	char c = 0;
	char cmd[PATH_MAX];

	printf("\nDefault: %s%s %s\nboot: ", (*optarg) ? "-" : "", optarg, kname);
	if ((c = GetChar()) == -1)
	    return NULL;

	cmd[0] = 0;
	getstr(cmd,c);
	xputchar('\n');
	if (parse(cmd,kname,optarg))
	    xputchar('\a');
	else if (devcanon(kname) == 0)
	    return kname;
	return NULL;
}

/*
 * Make bootpath canonical, provides defaults when missing
 */
static int
devcanon(char *fname)
{
	int ctlr = 0, unit = 0, part = 0;
	int c;
	char device_name[20];
	char file_name[PATH_MAX];
	const char *cp;
	char *ncp;

	//printf("devcanon(%s)\n",fname);

	cp = fname;
	ncp = device_name;

	/* expect a string like '0/ace(0,0)/netbsd' e.g. ctrl/name(unit,part)/file
	 * Defaults: ctrl=0, name='ace', unit=0, part=0, file=<none>
	 */

	/* get controller number */
	if ((c = *cp) >= '0' && c <= '9') {
	    ctlr = c - '0';
	    c = *++cp;
	    if (c != '/')
	        return (ENXIO);
	    c = *++cp;
	}

	/* get device name */
	while ((c = *cp) != '\0') {
	    if ((c == '(') || (c == '/')) {
	        cp++;
	        break;
	    }
	    if (ncp < device_name + sizeof(device_name) - 1)
	        *ncp++ = c;
	    cp++;
	}
	/* set default if missing */
	if (ncp == device_name) {
	    strcpy(device_name,"ace");
	    ncp += 3;
	}

	/* get device number */
	if ((c = *cp) >= '0' && c <= '9') {
	    unit = c - '0';
	    c = *++cp;
	}

	if (c == ',') {
	    /* get partition number */
	    if ((c = *++cp) >= '0' && c <= '9') {
	        part = c - '0';
	        c = *++cp;
	    }
	}

	if (c == ')')
	    c = *++cp;
	if (c == '/')
	    cp++;

	*ncp = '\0';

	/* Copy kernel name before we overwrite, then do it */
	strcpy(file_name, (*cp) ? cp : kernelnames[0]);
	snprintf(fname, PATH_MAX, "%c/%s(%c,%c)/%s",
	        ctlr + '0', device_name, unit + '0', part + '0', file_name);

	//printf("devcanon -> %s\n",fname);

	return (0);
}
