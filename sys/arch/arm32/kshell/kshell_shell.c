/* $NetBSD: kshell_shell.c,v 1.8 1996/10/11 00:07:10 christos Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
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

/* Any asc devices configured ? */

#include "asc.h"

/* Declare global variables */

/* Local function prototypes */

char	*strchr __P((const char *, int));

void dumpb __P((u_char */*addr*/, int /*count*/));
void dumpw __P((u_char */*addr*/, int /*count*/));

int readstring __P((char *, int, char *, char *));

void debug_show_q_details __P((void));
void shell_disassem	__P((int argc, char *argv[]));
void shell_devices	__P((int argc, char *argv[]));
void shell_vmmap	__P((int argc, char *argv[]));
void shell_flush	__P((int argc, char *argv[]));
void shell_pextract	__P((int argc, char *argv[]));
void shell_vnode	__P((int argc, char *argv[]));
void debug_show_all_procs __P((int argc, char *argv[]));
void debug_show_callout	__P((int argc, char *argv[]));
void debug_show_fs	__P((int argc, char *argv[]));
void debug_show_vm_map	__P((vm_map_t map, char *text));
void debug_show_pmap	__P((pmap_t pmap));
void pmap_dump_pvs	__P((void));
void asc_dump		__P((void));
void pmap_pagedir_dump	__P((void));

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

	if (buf == NULL)
		return(0);

/* skip any spaces */

	while (*buf == ' ')
		++buf;

/* return 0 if a zero length string is passed */

	if (*buf == 0)
		return(0);

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


/* poke - writes a byte/word to memory */

void
shell_poke(argc, argv)
	int argc;
	char *argv[];	
{
	u_int addr;
	u_int data;

	if (argc < 3) {
		kprintf("Syntax: poke[bw] <addr> <data>\n\r");
		return;
	}

/* Decode the two arguments */

	addr = readhex(argv[1]);
	data = readhex(argv[2]);

	if (argv[0][4] == 'b')
		WriteByte(addr, data);
	if (argv[0][4] == 'w')
		WriteWord(addr, data);
}


/* peek - reads a byte/word from memory*/

void
shell_peek(argc, argv)
	int argc;
	char *argv[];	
{
	u_int addr;
	u_int data;

	if (argc < 2) {
		kprintf("Syntax: peek[bw] <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = readhex(argv[1]);

	if (argv[0][4] == 'b')
		data = ReadByte(addr);
	else
		data = ReadWord(addr);

	kprintf("%08x : %08x\n\r", addr, data);
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
		kprintf("Syntax: dumpb <addr> [<bytes>]\n\r");
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
		kprintf("Syntax: dumpw <addr> [<bytes>]\n\r");
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
		kprintf("Syntax: vmmap <map addr>\n\r");
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
		kprintf("Syntax: pmap <pmap addr>\n\r");
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

	kprintf(" driver  unit state     name\n");
	for (cf = cfdata; cf->cf_driver; ++cf) {
		cd = cf->cf_driver;
		if (cf->cf_fstate & FSTATE_FOUND)
			state = "FOUND    ";
		else
			state = "NOT FOUND";

		kprintf("%08x  %2d  %s %s\n", (u_int)cd, (u_int)cf->cf_unit,
		    state, cd->cd_name);

		if (cf->cf_fstate & FSTATE_FOUND) {
			for (loop = 0; loop < cd->cd_ndevs; ++loop) {
				dv = (struct device *)cd->cd_devs[loop];
				if (dv != 0)
					kprintf("                        %s (%08x)\n",
					    dv->dv_xname, (u_int) dv);
			}
		}
		kprintf("\n");
	} 
}


void
shell_reboot(argc, argv)
	int argc;
	char *argv[];	
{
	kprintf("Running shutdown hooks ...\n");
	doshutdownhooks();

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
	(void)kprintf("%9u cpu context switches\n", sum.v_swtch);
	(void)kprintf("%9u device interrupts\n", sum.v_intr);
	(void)kprintf("%9u software interrupts\n", sum.v_soft);
	(void)kprintf("%9u traps\n", sum.v_trap);
	(void)kprintf("%9u system calls\n", sum.v_syscall);
	(void)kprintf("%9u total faults taken\n", sum.v_faults);
	(void)kprintf("%9u swap ins\n", sum.v_swpin);
	(void)kprintf("%9u swap outs\n", sum.v_swpout);
	(void)kprintf("%9u pages swapped in\n", sum.v_pswpin / CLSIZE);
	(void)kprintf("%9u pages swapped out\n", sum.v_pswpout / CLSIZE);
	(void)kprintf("%9u page ins\n", sum.v_pageins);
	(void)kprintf("%9u page outs\n", sum.v_pageouts);
	(void)kprintf("%9u pages paged in\n", sum.v_pgpgin);
	(void)kprintf("%9u pages paged out\n", sum.v_pgpgout);
	(void)kprintf("%9u pages reactivated\n", sum.v_reactivated);
	(void)kprintf("%9u intransit blocking page faults\n", sum.v_intrans);
	(void)kprintf("%9u zero fill pages created\n", sum.v_nzfod / CLSIZE);
	(void)kprintf("%9u zero fill page faults\n", sum.v_zfod / CLSIZE);
	(void)kprintf("%9u pages examined by the clock daemon\n", sum.v_scan);
	(void)kprintf("%9u revolutions of the clock hand\n", sum.v_rev);
	(void)kprintf("%9u VM object cache lookups\n", sum.v_lookups);
	(void)kprintf("%9u VM object hits\n", sum.v_hits);
	(void)kprintf("%9u total VM faults taken\n", sum.v_vm_faults);
	(void)kprintf("%9u copy-on-write faults\n", sum.v_cow_faults);
	(void)kprintf("%9u pages freed by daemon\n", sum.v_dfree);
	(void)kprintf("%9u pages freed by exiting processes\n", sum.v_pfree);
	(void)kprintf("%9u pages free\n", sum.v_free_count);
	(void)kprintf("%9u pages wired down\n", sum.v_wire_count);
	(void)kprintf("%9u pages active\n", sum.v_active_count);
	(void)kprintf("%9u pages inactive\n", sum.v_inactive_count);
	(void)kprintf("%9u bytes per page\n", sum.v_page_size);
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
		kprintf("Syntax: pextract <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	pa = pmap_extract(kernel_pmap, (vm_offset_t)addr);
	pind = pmap_page_index(pa);

	kprintf("va=%08x pa=%08x pind=%d\n", (u_int)addr, (u_int)pa, pind);
}


void
shell_vnode(argc, argv)
	int argc;
	char *argv[];	
{
	struct vnode *vp;

	if (argc < 2) {
		kprintf("Syntax: vnode <vp>\n\r");
		return;
	}

/* Decode the one argument */

	vp = (struct vnode *)readhex(argv[1]);

	kprintf("vp = %08x\n", (u_int)vp);
	kprintf("vp->v_type = %d\n", vp->v_type);
	kprintf("vp->v_flag = %ld\n", vp->v_flag);
	kprintf("vp->v_usecount = %d\n", vp->v_usecount);
	kprintf("vp->v_writecount = %d\n", vp->v_writecount);
	kprintf("vp->v_numoutput = %ld\n", vp->v_numoutput);

	vprint("vnode:", vp);
}


void
shell_buf(argc, argv)
	int argc;
	char *argv[];	
{
	struct buf *bp;

	if (argc < 2) {
		kprintf("Syntax: buf <bp>\n\r");
		return;
	}

/* Decode the one argument */

	bp = (struct buf *)readhex(argv[1]);

	kprintf("buf pointer=%08x\n", (u_int)bp);
	if (bp->b_proc)
		kprintf("proc=%08x pid=%d\n", (u_int)bp->b_proc, bp->b_proc->p_pid);
	
	kprintf("flags=%08x\n", (u_int)bp->b_proc);
	kprintf("b_error=%d\n", bp->b_error);
	kprintf("b_bufsize=%08x\n", (u_int)bp->b_bufsize);
	kprintf("b_bcount=%08x\n", (u_int)bp->b_bcount);
	kprintf("b_resid=%d\n", (int)bp->b_resid);
	kprintf("b_bdev=%04x\n", bp->b_dev);
	kprintf("b_baddr=%08x\n", (u_int)bp->b_un.b_addr);
	kprintf("b_lblkno=%08x\n", (u_int)bp->b_lblkno);
	kprintf("b_blkno=%08x\n", bp->b_blkno);
	kprintf("b_vp=%08x\n", (u_int)bp->b_vp);
}



#ifdef XXX1
void
shell_bufstats(argc, argv)
	int argc;
	char *argv[];	
{
	vfs_bufstats();
}


void
shell_vndbuf(argc, argv)
	int argc;
	char *argv[];	
{
	struct vnode *vp;

	if (argc < 2) {
		kprintf("Syntax: vndbuf <vp>\n\r");
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
		kprintf("Syntax: vndbuf <vp>\n\r");
		return;
	}

/* Decode the one argument */

	vp = (struct vnode *)readhex(argv[1]);

	dumpvncbuf(vp);
}
#endif

/* shell - a crude shell */

int
shell()
{
	int quit = 0;
	char buffer[200];
	char *ptr;
	char *ptr1;
	int args;
	char *argv[20];

	kprintf("\nRiscBSD debug/monitor shell\n");
	kprintf("CTRL-D, exit or reboot to terminate\n\n");

	do {
/* print prompt */

		kprintf("kshell> ");

/* Read line from keyboard */

		if (readstring(buffer, 200, NULL, NULL) == -1)
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
#ifdef DDB
		else if (strcmp(argv[0], "deb") == 0)
			Debugger();
#endif
		else if (strcmp(argv[0], "peekb") == 0)
			shell_peek(args, argv);
		else if (strcmp(argv[0], "pokeb") == 0)
			shell_poke(args, argv);
		else if (strcmp(argv[0], "peekw") == 0)
			shell_peek(args, argv);
		else if (strcmp(argv[0], "pokew") == 0)
			shell_poke(args, argv);
		else if (strcmp(argv[0], "dumpb") == 0)
			shell_dumpb(args, argv);
		else if (strcmp(argv[0], "reboot") == 0)
			shell_reboot(args, argv);
		else if (strcmp(argv[0], "dumpw") == 0)
			shell_dumpw(args, argv);
		else if (strcmp(argv[0], "dump") == 0)
			shell_dumpw(args, argv);
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
		else if (strcmp(argv[0], "listfs") == 0)
			debug_show_fs(args, argv);
		else if (strcmp(argv[0], "vmmap") == 0)
			shell_vmmap(args, argv);
		else if (strcmp(argv[0], "pmap") == 0)
			shell_pmap(args, argv);
		else if (strcmp(argv[0], "flush") == 0)
			shell_flush(args, argv);
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
		else if (strcmp(argv[0], "vnode") == 0)
			shell_vnode(args, argv);
#if NASC > 0
		else if (strcmp(argv[0], "ascdump") == 0)
			asc_dump();
#endif
		else if (strcmp(argv[0], "buf") == 0)
			shell_buf(args, argv);
#ifdef XXX1
		else if (strcmp(argv[0], "vndbuf") == 0)
			shell_vndbuf(args, argv);
		else if (strcmp(argv[0], "vncbuf") == 0)
			shell_vncbuf(args, argv);
		else if (strcmp(argv[0], "bufstats") == 0)
			shell_bufstats(args, argv);
#endif
		else if (strcmp(argv[0], "help") == 0
		    || strcmp(argv[0], "?") == 0) {
			kprintf("peekb <hexaddr>\r\n");
			kprintf("pokeb <hexaddr> <data>\r\n");
			kprintf("peekw <hexaddr>\r\n");
			kprintf("pokew <hexaddr <data>\r\n");
			kprintf("dis <hexaddr>\r\n");
			kprintf("dumpb <hexaddr> [length]\r\n");
			kprintf("dumpw <hexaddr> [length]\r\n");
			kprintf("dump <hexaddr> [length]\r\n");
			kprintf("reboot\r\n");
			kprintf("qs\r\n");
			kprintf("ps [m]\r\n");
			kprintf("vmstat\n");
			kprintf("listfs\n");
			kprintf("devices\n");
			kprintf("callouts\n");
			kprintf("prompt\r\n");
			kprintf("vmmap <vmmap addr>\r\n");
			kprintf("pmap <pmap addr>\r\n");
			kprintf("pdstat\r\n");
			kprintf("flush\r\n");
			kprintf("exit\r\n");
			kprintf("forceboot\r\n");
			kprintf("dumppvs\r\n");
			kprintf("pextract <phys addr>\r\n");
			kprintf("vnode <vp>\r\n");
			kprintf("buf <bp>\r\n");
#if NASC > 0
			kprintf("ascdump\r\n");
#endif
#ifdef XXX1
			kprintf("vndbuf\r\n");
			kprintf("vncbuf\r\n");
			kprintf("bufstats\r\n");
#endif
		}
	} while (!quit);

	return(0);
}

/* End of shell_shell.c */
