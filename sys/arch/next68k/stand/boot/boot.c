/*	$NetBSD: boot.c,v 1.10.122.1 2014/08/20 00:03:17 tls Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
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

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <machine/cpu.h>        /* for NEXT_RAMBASE */

#include <next68k/next68k/nextrom.h>

#define KERN_LOADADDR NEXT_RAMBASE

extern int errno;

extern char *mg;
#define	MON(type, off) (*(type *)((u_int) (mg) + off))

int devparse(const char *, int *, char *, char *, char *, char **);

/* the PROM overwrites MG_boot_arg :-( */
/* #define PROCESS_ARGS */

/*
 * Boot device is derived from PROM provided information.
 */

extern char bootprog_rev[];
extern char bootprog_name[];
extern int build;
#define KNAMEN 100
char kernel[KNAMEN];
int entry_point;		/* return value filled in by machdep_start */
int turbo;

extern void rtc_init(void);

extern int try_bootp;

volatile int qq;

int
main(char *boot_arg)
{
	int fd;
	u_long marks[MARK_MAX];
	int dev;
	char count, lun, part;
	char machine;
	char *file;
#ifdef PROCESS_ARGS
	char *kernel_args = MON(char *, MG_boot_dev);
#endif

	machine = MON(char, MG_machine_type);
	if (machine == NeXT_TURBO_MONO || machine == NeXT_TURBO_COLOR)
		turbo = 1;
	else
		turbo = 0;

	memset(marks, 0, sizeof(marks));
	printf(">> %s BOOT [%s #%d]\n", bootprog_name, bootprog_rev, build);
	printf(">> type %d, %sturbo\n", machine, turbo ? "" : "non-");
	rtc_init();

	try_bootp = 1;

#if 0
	{
		int i;
		int *p = (int *)mg;
		for (i = 0; i <= 896; ) {
			printf ("%d: %x %x %x %x %x %x %x %x\n", i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
			p = &p[8];
			i += 8*4;
		}
	}
	printf("Press return to continue.\n");
	getchar();
#endif

	strcpy(kernel, boot_arg);
	entry_point = 0;

	for (;;) {
		marks[MARK_START] = (u_long)KERN_LOADADDR;
		fd = loadfile(kernel, marks, LOAD_KERNEL);
		if (fd != -1) {
			break;
		}

		printf("load of %s: %s\n", kernel, strerror(errno));
		printf("boot: ");
		gets(kernel);
		if (kernel[0] == '\0')
			return 0;

#ifdef PROCESS_ARGS
		kernel_args = strchr(kernel, ')');
		if (kernel_args == NULL)
			kernel_args = kernel;
		kernel_args = strchr(kernel_args, ' ');
		if (kernel_args)
			*kernel_args++ = '\0';
#endif
	}

	dev = 0;
	count = lun = part = 0;
	if (devparse(kernel, &dev, &count, &lun, &part, &file) == 0) {
		char *p = (char *)marks[MARK_END];
		strcpy (p, devsw[dev].dv_name);
		MON(char *, MG_boot_dev) = p;
		p += strlen (p) + 1;
		snprintf (p, 1024, "(%d,%d,%d)", count, lun, part); /* XXX */
		MON(char *, MG_boot_info) = p;
		p += strlen (p) + 1;
		snprintf (p, 1024, "%s", file); /* XXX */
		MON(char *, MG_boot_file) = p;
#ifdef PROCESS_ARGS
		p += strlen (p) + 1;
		if (kernel_args)
			strcpy (p, kernel_args);
		else
			*p = 0;
		MON(char *, MG_boot_arg) = p;
#endif
	}

	*((u_long *)KERN_LOADADDR) = marks[MARK_END] - KERN_LOADADDR;
	printf("entry 0x%lx esym 0x%lx\n",
	       marks[MARK_ENTRY], marks[MARK_END] - KERN_LOADADDR);
	return marks[MARK_ENTRY];
}
