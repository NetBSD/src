/* $NetBSD: shell_shell.c,v 1.1 1996/01/31 23:24:04 mark Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * shell_shell.c
 *
 * Debug / Monitor shell entry and commands
 *
 * Created      : 09/10/94
 * Last updated : 28/08/94
 *
 *    $Id: shell_shell.c,v 1.1 1996/01/31 23:24:04 mark Exp $
 */

/* Include standard header files */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <vm/vm.h>

/* Local header files */

#include <machine/pmap.h>
#include <machine/katelib.h>
#include <machine/vidc.h>
#include <machine/rtc.h>

/* Special compilation symbols
 *
 * SHELL_MALLOC - indicates that we have malloc and free functions available
 */

/* Declare global variables */

caddr_t shell_ident = 0;

/* Local function prototypes */

char	*strchr __P((const char *, int));
int	strcmp __P((char *, char *));

void dumpb __P((u_char */*addr*/, int /*count*/));
void dumpw __P((u_char */*addr*/, int /*count*/));

int readstring __P((char *, int, char *, char *, caddr_t));

void debug_show_q_details __P((void));
void shell_printhistory __P((int argc, char *argv[]));
/*
 * These functions have been revoked
 *
void shell_ls __P((int argc, char *argv[]));
void shell_cp __P((int argc, char *argv[]));
void shell_mount __P((int argc, char *argv[]));
void shell_mountcd __P((int argc, char *argv[]));
void shell_umount __P((int argc, char *argv[]));
void shell_mountmsdos __P((int argc, char *argv[]));
void shell_mountufs __P((int argc, char *argv[]));
void shell_cat __P((int argc, char *argv[]));
void shell_fork __P((int argc, char *argv[]));
void shell_kill __P((int argc, char *argv[]));
void shell_od __P((int argc, char *argv[]));
void shell_write __P((int argc, char *argv[]));
*/
void shell_disassem	__P((int argc, char *argv[]));
void shell_devices	__P((int argc, char *argv[]));
void shell_vmmap	__P((int argc, char *argv[]));
void shell_flush	__P((int argc, char *argv[]));
void shell_pextract	__P((int argc, char *argv[]));
void shell_kstack	__P((int argc, char *argv[]));
void shell_vnode	__P((int argc, char *argv[]));
void debug_show_all_procs __P((int argc, char *argv[]));
void debug_show_callout	__P((int argc, char *argv[]));
void debug_show_fs	__P((int argc, char *argv[]));
void debug_show_vm_map	__P((vm_map_t map, char *text));
void debug_show_pmap	__P((pmap_t pmap));
void pmap_dump_pvs	__P((void));
void bootsync		__P((void));

/* Now for the main code */

/* readhex
 *
 * This routine interprets the input string as a sequence of hex characters
 * and returns it as an integer.
 */

int
readhex(buf)
	char *buf;
{
	int value;
	int nibble;

	if (buf == NULL) {
		return(0);
	}

/* skip any spaces */

	while (*buf == ' ')
		++buf;

/* return 0 if a zero length string is passed */

	if (*buf == 0) {
		return(0);
	}

/* Convert the characters */

	value = 0;

	while (*buf != 0 && strchr("0123456789abcdefABCDEF", *buf) != 0) {
		nibble = (*buf - '0');
		if (nibble > 9) nibble -= 7;
		if (nibble > 15) nibble -= 32;
		value = (value << 4) | nibble;
		++buf;
	}

	return(value);
}


/* pokebyte - writes a byte to memory */

void
shell_pokebyte(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;
	int data;

	if (argc < 3) {
		printf("Syntax: pokebyte <addr> <data>\n\r");
		return;
	}

/* Decode the two arguments */

	addr = (u_char *)readhex(argv[1]);
	data = readhex(argv[2]);

	*addr = data;
}


/* peekbyte - reads a byte from memory from supervisor mode */

void
shell_peekbyte(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;
	int data;

	if (argc < 2) {
		printf("Syntax: peekb <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	data = *addr;

	printf("%08x : %02x\n\r", (int)addr, data);
}


/* pokeword - writes a byte to memory */

void
shell_pokeword(argc, argv)
	int argc;
	char *argv[];	
{
	u_int *addr;
	int data;

	if (argc < 3) {
		printf("Syntax: pokeb <addr> <data>\n\r");
		return;
	}

/* Decode the two arguments */

/*
 * Additionally we look for the address "vidc" and substitue the
 * vidc20 address.
 */

	if (strcmp(argv[1], "vidc") == 0)
		addr = (u_int *)VIDC_BASE;
	else
		addr = (u_int *)readhex(argv[1]);
	data = readhex(argv[2]);

	*addr = data;
}


/* peekword - reads a byte from memory */

void
shell_peekword(argc, argv)
	int argc;
	char *argv[];	
{
	u_int *addr;
	int data;

	if (argc < 2) {
		printf("Syntax: peekb <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_int *)readhex(argv[1]);

	data = *addr;

	printf("%08x : %08x\n\r", (int)addr, data);
}


/* dumpb - dumps memory in bytes*/

void
shell_dumpb(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;
	int count;

	if (argc < 2) {
		printf("Syntax: dumpb <addr> [<bytes>]\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	if (argc > 2)
		count = readhex(argv[2]);
	else
		count = 0x80;

	dumpb(addr, count);
}


/* dumpw - dumps memory in bytes*/

void
shell_dumpw(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;
	int count;

	if (argc < 2) {
		printf("Syntax: dumpw <addr> [<bytes>]\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	if (argc > 2)
		count = readhex(argv[2]);
	else
		count = 0x80;

	dumpw(addr, count); 
}

  
/* vmmap - dumps the vmmap */

void
shell_vmmap(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;

	if (argc < 2) {
		printf("Syntax: vmmap <map addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	debug_show_vm_map((vm_map_t)addr, argv[1]);
}


/* pmap - dumps the pmap */

void
shell_pmap(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;

	if (argc < 2) {
		printf("Syntax: pmap <pmap addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	debug_show_pmap((pmap_t)addr);
}


/*
 * void shell_devices(int argc, char *argv[])
 *
 * Display all the devices
 */

extern struct cfdata cfdata[];
 
void
shell_devices(argc, argv)
	int argc;
	char *argv[];	
{
	struct cfdata *cf;
	struct cfdriver *cd;
	struct device *dv;
	int loop;
	char *state;

	printf(" driver  unit state     name\n");
	for (cf = cfdata; cf->cf_driver; ++cf) {
		cd = cf->cf_driver;
		if (cf->cf_fstate & FSTATE_FOUND)
			state = "FOUND    ";
		else
			state = "NOT FOUND";

		printf("%08x  %2d  %s %s\n", (u_int)cd, (u_int)cf->cf_unit,
		    state, cd->cd_name);

		if (cf->cf_fstate & FSTATE_FOUND) {
			for (loop = 0; loop < cd->cd_ndevs; ++loop) {
				dv = (struct device *)cd->cd_devs[loop];
				if (dv != 0)
					printf("                        %s (%08x)\n",
					    dv->dv_xname, (u_int) dv);
			}
		}
		printf("\n");
	} 
}


void
shell_reboot(argc, argv)
	int argc;
	char *argv[];	
{
	if (shell_ident == 0) {
		IRQdisable;
		boot0();
	}

	bootsync();
              
	delay(1000000);

	splhigh();

	IRQdisable;
	boot0();
}

void
forceboot(argc, argv)
	int argc;
	char *argv[];	
{
	cmos_write(0x90, cmos_read(0x90) | 0x02);
	shell_reboot(0, NULL);
}


void
shell_flush(argc, argv)
	int argc;
	char *argv[];	
{
	idcflush();
	tlbflush();
}


void
shell_vmstat(argc, argv)
	int argc;
	char *argv[];	
{
	struct vmmeter sum;
    
	sum = cnt;
	(void)printf("%9u cpu context switches\n", sum.v_swtch);
	(void)printf("%9u device interrupts\n", sum.v_intr);
	(void)printf("%9u software interrupts\n", sum.v_soft);
	(void)printf("%9u traps\n", sum.v_trap);
	(void)printf("%9u system calls\n", sum.v_syscall);
	(void)printf("%9u total faults taken\n", sum.v_faults);
	(void)printf("%9u swap ins\n", sum.v_swpin);
	(void)printf("%9u swap outs\n", sum.v_swpout);
	(void)printf("%9u pages swapped in\n", sum.v_pswpin / CLSIZE);
	(void)printf("%9u pages swapped out\n", sum.v_pswpout / CLSIZE);
	(void)printf("%9u page ins\n", sum.v_pageins);
	(void)printf("%9u page outs\n", sum.v_pageouts);
	(void)printf("%9u pages paged in\n", sum.v_pgpgin);
	(void)printf("%9u pages paged out\n", sum.v_pgpgout);
	(void)printf("%9u pages reactivated\n", sum.v_reactivated);
	(void)printf("%9u intransit blocking page faults\n", sum.v_intrans);
	(void)printf("%9u zero fill pages created\n", sum.v_nzfod / CLSIZE);
	(void)printf("%9u zero fill page faults\n", sum.v_zfod / CLSIZE);
	(void)printf("%9u pages examined by the clock daemon\n", sum.v_scan);
	(void)printf("%9u revolutions of the clock hand\n", sum.v_rev);
	(void)printf("%9u VM object cache lookups\n", sum.v_lookups);
	(void)printf("%9u VM object hits\n", sum.v_hits);
	(void)printf("%9u total VM faults taken\n", sum.v_vm_faults);
	(void)printf("%9u copy-on-write faults\n", sum.v_cow_faults);
	(void)printf("%9u pages freed by daemon\n", sum.v_dfree);
	(void)printf("%9u pages freed by exiting processes\n", sum.v_pfree);
	(void)printf("%9u pages free\n", sum.v_free_count);
	(void)printf("%9u pages wired down\n", sum.v_wire_count);
	(void)printf("%9u pages active\n", sum.v_active_count);
	(void)printf("%9u pages inactive\n", sum.v_inactive_count);
	(void)printf("%9u bytes per page\n", sum.v_page_size);
}


void
shell_pextract(argc, argv)
	int argc;
	char *argv[];	
{
	u_char *addr;
	vm_offset_t pa;
	int pind;

	if (argc < 2) {
		printf("Syntax: pextract <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	pa = pmap_extract(kernel_pmap, (vm_offset_t)addr);
	pind = pmap_page_index(pa);

	printf("va=%08x pa=%08x pind=%d\n", (u_int)addr, (u_int)pa, pind);
}


void
shell_kstack(argc, argv)
	int argc;
	char *argv[];	
{
	struct proc *p;

	if (argc < 2) {
		printf("Syntax: kstack <proc>\n\r");
		return;
	}

/* Decode the one argument */

	p = (struct proc *)readhex(argv[1]);

#ifdef POSTMORTEM
	kstack_stuff(p);
#else
	printf("Kernel not built with postmortem support\n");
#endif
}


void
shell_vnode(argc, argv)
	int argc;
	char *argv[];	
{
	struct vnode *vp;

	if (argc < 2) {
		printf("Syntax: vnode <vp>\n\r");
		return;
	}

/* Decode the one argument */

	vp = (struct vnode *)readhex(argv[1]);

	printf("vp = %08x\n", (u_int)vp);
	printf("vp->v_type = %d\n", vp->v_type);
	printf("vp->v_flag = %ld\n", vp->v_flag);
	printf("vp->v_usecount = %d\n", vp->v_usecount);
	printf("vp->v_writecount = %d\n", vp->v_writecount);
	printf("vp->v_numoutput = %ld\n", vp->v_numoutput);

	vprint("vnode:", vp);
}

#if 0
void
shell_vndbuf(argc, argv)
	int argc;
	char *argv[];	
{
	struct vnode *vp;

	if (argc < 2) {
		printf("Syntax: vndbuf <vp>\n\r");
		return;
	}

/* Decode the one argument */

	vp = (struct vnode *)readhex(argv[1]);

	dumpvndbuf(vp);
}


void
shell_vncbuf(argc, argv)
	int argc;
	char *argv[];	
{
	struct vnode *vp;

	if (argc < 2) {
		printf("Syntax: vndbuf <vp>\n\r");
		return;
	}

/* Decode the one argument */

	vp = (struct vnode *)readhex(argv[1]);

	dumpvncbuf(vp);
}
#endif

/* shell - a crude shell */

int
shell(ident)
	caddr_t ident;
{
	int quit = 0;
	char buffer[200];
	char *ptr;
	char *ptr1;
	int args;
#ifdef SHELL_MALLOC
	char **argv;
#else
	char *argv[20];
#endif

	printf("\nRiscBSD debug/monitor shell\n");
	printf("CTRL-D, exit or reboot to terminate\n\n");

	shell_ident = ident;

	do {
/* print prompt */

		printf("kshell> ");

/* Read line from keyboard */

		if (readstring(buffer, 200, NULL, NULL, ident) == -1)
			return(0);

		ptr = buffer;

/* Slice leading spaces */

		while (*ptr == ' ')
			++ptr;

/* Loop back if zero length string */

		if (*ptr == 0)
			continue;

/* Count the number of space separated args */

		args = 0;
		ptr1 = ptr;

		while (*ptr1 != 0) {
			if (*ptr1 == ' ') {
				++args;
				while (*ptr1 == ' ')
					++ptr1;
			} else
				++ptr1;
		}

/* Allocate memory to store arg pointers */

#ifdef SHELL_MALLOC
		argv = (char **)malloc((args + 1) * sizeof(char *));
#endif


/*
 * Construct the array of pointers to the args and terminate
 * each argument with 0x00
 */

		args = 0;
		ptr1 = ptr;

		while (*ptr1 != 0) {
			argv[args] = ptr1;
			++args;
			while (*ptr1 != ' ' && *ptr1 != 0)
				++ptr1;

			while (*ptr1 == ' ') {
				*ptr1 = 0;
				++ptr1;
			}
		}

		argv[args] = NULL;

/* Interpret commands */

		if (strcmp(argv[0], "exit") == 0)
			quit = 1;
		else if (strcmp(argv[0], "peekb") == 0)
			shell_peekbyte(args, argv);
		else if (strcmp(argv[0], "pokeb") == 0)
			shell_pokebyte(args, argv);
		else if (strcmp(argv[0], "peekw") == 0)
			shell_peekword(args, argv);
		else if (strcmp(argv[0], "pokew") == 0)
			shell_pokeword(args, argv);
		else if (strcmp(argv[0], "dumpb") == 0)
			shell_dumpb(args, argv);
		else if (strcmp(argv[0], "reboot") == 0)
			shell_reboot(args, argv);
		else if (strcmp(argv[0], "dumpw") == 0)
			shell_dumpw(args, argv);
		else if (strcmp(argv[0], "dump") == 0)
			shell_dumpw(args, argv);
		else if (strcmp(argv[0], "history") == 0)
			shell_printhistory(args, argv);
		else if (strcmp(argv[0], "dis") == 0)
			shell_disassem(args, argv);
		else if (strcmp(argv[0], "qs") == 0)
			debug_show_q_details();
		else if (strcmp(argv[0], "ps") == 0)
			debug_show_all_procs(args, argv);
		else if (strcmp(argv[0], "callouts") == 0)
			debug_show_callout(args, argv);
		else if (strcmp(argv[0], "devices") == 0)
			shell_devices(args, argv);
/*		else if (strcmp(argv[0], "ls") == 0)
			shell_ls(args, argv);
		else if (strcmp(argv[0], "cp") == 0)
			shell_cp(args, argv);
		else if (strcmp(argv[0], "cat") == 0)
			shell_cat(args, argv);*/
		else if (strcmp(argv[0], "listfs") == 0)
			debug_show_fs(args, argv);
/*		else if (strcmp(argv[0], "mount") == 0)
			shell_mount(args, argv);
		else if (strcmp(argv[0], "umount") == 0)
			shell_umount(args, argv);
		else if (strcmp(argv[0], "mountmsdos") == 0)
			shell_mountmsdos(args, argv);
		else if (strcmp(argv[0], "mountufs") == 0)
			shell_mountufs(args, argv);
		else if (strcmp(argv[0], "mountcd") == 0)
			shell_mountcd(args, argv);
		else if (strcmp(argv[0], "fork") == 0)
			shell_fork(args, argv);
		else if (strcmp(argv[0], "kill") == 0)
			shell_kill(args, argv);*/
/*		else if (strcmp(argv[0], "od") == 0)
			shell_od(args, argv);*/
		else if (strcmp(argv[0], "vmmap") == 0)
			shell_vmmap(args, argv);
		else if (strcmp(argv[0], "pmap") == 0)
			shell_pmap(args, argv);
		else if (strcmp(argv[0], "flush") == 0)
			shell_flush(args, argv);
/*		else if (strcmp(argv[0], "write") == 0)
			shell_write(args, argv);*/
		else if (strcmp(argv[0], "vmstat") == 0)
			shell_vmstat(args, argv);
		else if (strcmp(argv[0], "pdstat") == 0)
			pmap_pagedir_dump();
		else if (strcmp(argv[0], "traceback") == 0)
			traceback();
		else if (strcmp(argv[0], "forceboot") == 0)
			forceboot(args, argv);
		else if (strcmp(argv[0], "dumppvs") == 0)
			pmap_dump_pvs();
		else if (strcmp(argv[0], "pextract") == 0)
			shell_pextract(args, argv);
		else if (strcmp(argv[0], "kstack") == 0)
			shell_kstack(args, argv);
		else if (strcmp(argv[0], "vnode") == 0)
			shell_vnode(args, argv);
		else if (strcmp(argv[0], "help") == 0
		    || strcmp(argv[0], "?") == 0) {
			printf("peekb <hexaddr>\r\n");
			printf("pokeb <hexaddr> <data>\r\n");
			printf("peekw <hexaddr>\r\n");
			printf("pokew <hexaddr|\x1b[1mvidc\x1b[0m> <data>\r\n");
			printf("dis <hexaddr>\r\n");
			printf("dumpb <hexaddr> [length]\r\n");
			printf("dumpw <hexaddr> [length]\r\n");
			printf("dump <hexaddr> [length]\r\n");
			printf("reboot\r\n");
			printf("history\r\n");
			printf("qs\r\n");
			printf("ps [m]\r\n");
/*			printf("ls [-l] <file|directory>\r\n");
			printf("cat [-s] <file>\r\n");
			printf("cp <file1> <file2>\r\n");
			printf("mount <mountpoint>\n");
			printf("umount <mountpoint>\n");
			printf("mountmsdos <device> <mountpoint>\n");
			printf("mountufs <device> <mountpoint>\n");
			printf("mountcd <device> <mountpoint>\n");
			printf("od <file> <offset>\n");*/
			printf("vmstat\n");
			printf("listfs\n");
/*			printf("fork\n");*/
/*			printf("debug [\x1b[1mfd\x1b[0m|\x1b[1mrd\x1b[0m]\n");*/
			printf("devices\n");
			printf("callouts\n");
			printf("prompt\r\n");
			printf("vmmap <vmmap addr>\r\n");
			printf("pmap <pmap addr>\r\n");
			printf("pdstat\r\n");
			printf("flush\r\n");
			printf("write <filename>\r\n");
			printf("exit\r\n");
			printf("forceboot\r\n");
			printf("dumppvs\r\n");
			printf("pextract <phys addr>\r\n");
			printf("kstack <proc>\r\n");
			printf("vnode <vp>\r\n");
		}

#ifdef SHELL_MALLOC
		free(argv);
#endif
	} while (!quit);

	return(0);
}

/* End of shell.c */
