/*	$NetBSD: loadbsd.c,v 1.2 1995/03/28 06:26:50 leo Exp $	*/

/*
 * Copyright (c) 1995 L. Weppelman
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
 *      This product includes software developed by Leo Weppelman.
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

/*
 * NetBSD loader for the Atari-TT.
 */

#include <stdio.h>
#include <a_out.h>
#include <fcntl.h>
#include <osbind.h>
#include <stdarg.h>
#include "loader.h"

char	*Progname;		/* How are we called		*/
int	t_flag = 0;		/* Just test, do not execute	*/

char version[] = "$VER: LoadBSD 1.0 (11/01/95)";

/*
 * Default name of kernel to boot, large enough to patch
 */
char		kname[80] = "n:/netbsd";

static struct {
	u_char	*kp;		/* 00: Kernel load address		*/
	long	ksize;		/* 04: Size of loaded kernel		*/
	u_long	entry;		/* 08: Kernel entry point		*/
	long	stmem_size;	/* 12: Size of st-ram			*/
	long	ttmem_size;	/* 16: Size of tt-ram			*/
	long	cputype;	/* 20: Type of cpu			*/
	long	boothowto;	/* 24: How to boot			*/
	long	ttmem_start;	/* 28: Start of tt-ram			*/
	long	esym_loc;	/* 32: End of symbol table		*/
} kparam;

void get_sys_info(void);
void error(char *fmt, ...);
void help(void);
void usage(void);
void start_kernel(void);

int main(argc, argv)
int	argc;
char	**argv;
{
	/*
	 * Option parsing
	 */
	extern	int	optind;
	extern	char	*optarg;
	int		ch;
	int		fd;
	long		textsz, stringsz;
	struct exec	ehdr;
	
	Progname = argv[0];

	kparam.boothowto = RB_SINGLE;

	while ((ch = getopt(argc, argv, "abdhtv")) != EOF) {
		switch(ch) {
		case 'a':
			kparam.boothowto &= ~(RB_SINGLE);
			kparam.boothowto |= RB_AUTOBOOT;
			break;
		case 'b':
			kparam.boothowto |= RB_ASKNAME;
			break;
		case 'd':
			kparam.boothowto |= RB_KDB;
			break;
		case 't':
			t_flag = 1;
			break;
		case 'v':
			fprintf(stderr,"%s\n", version);
			break;
		case 'h':
			help();
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if(argc == 1)
		strcpy(kname, argv[0]);

	/*
	 * Get system info to pass to NetBSD
	 */
	get_sys_info();

	/*
	 * Find the kernel to boot and read it's exec-header
	 */
	if((fd = open(kname, O_RDONLY)) < 0)
		error("Cannot open kernel '%s'", kname);
	if(read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
		error("Cannot read exec-header of '%s'", kname);
	if((ehdr.a_magic & 0xffff) != NMAGIC) /* XXX */
		error("Not an NMAGIC file '%s'", kname);

	/*
	 * Extract various sizes from the kernel executable
	 */
	textsz          = (ehdr.a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1);
	kparam.esym_loc = 0;
	kparam.ksize    = textsz + ehdr.a_data + ehdr.a_bss;
	kparam.entry    = ehdr.a_entry;

	if(ehdr.a_syms) {
	  if(lseek(fd,ehdr.a_text+ehdr.a_data+ehdr.a_syms+sizeof(ehdr), 0) <= 0)
		error("Cannot seek to string table in '%s'", kname);
	  if(read(fd, &stringsz, sizeof(long)) != sizeof(long))
		error("Cannot read string-table size");
	  if(lseek(fd, sizeof(ehdr), 0) <= 0)
		error("Cannot seek back to text start");
	  kparam.ksize += ehdr.a_syms + sizeof(long) + stringsz;
	}

	if((kparam.kp = (u_char *)malloc(kparam.ksize)) == NULL)
		error("Cannot malloc kernel image space");

	/*
	 * Read text & data, clear bss
	 */
	if((read(fd, kparam.kp, ehdr.a_text) != ehdr.a_text)
	    || (read(fd, kparam.kp + textsz, ehdr.a_data) != ehdr.a_data))
		error("Unable to read kernel image\n");
	memset(kparam.kp + textsz + ehdr.a_data, 0, ehdr.a_bss);

	/*
	 * Read symbol and string table
	 */
	if(ehdr.a_syms) {
		long	*p;

		p = (long *)(kparam.kp + textsz + ehdr.a_data + ehdr.a_bss);
		*p++ = ehdr.a_syms;
		if(read(fd, (char *)p, ehdr.a_syms) != ehdr.a_syms)
			error("Cannot read symbol table\n");
		p = (long *)((char *)p + ehdr.a_syms);
		if(read(fd, (char *)p, stringsz) != stringsz)
			error("Cannot read string table\n");
		kparam.esym_loc = (long)((char *)p-(char *)kparam.kp +stringsz);
	}

	if(!t_flag)
		start_kernel();
		/* NOT REACHED */

	fprintf(stderr, "Kernel '%s' was loaded OK\n", kname);
	exit(0);
}

/*
 * Extract memory and cpu/fpu info from system.
 */
void get_sys_info()
{
	long	stck;
	long	*jar;

	kparam.cputype = 0;

	stck = Super(0);

	kparam.ttmem_start = TTRAM_BASE;
	kparam.ttmem_size  = *ADDR_RAMTOP - TTRAM_BASE;
	kparam.stmem_size  = *ADDR_PHYSTOP;

	/*
	 * Scan cookiejar for cpu/fpu types
	 */
	jar = *ADDR_P_COOKIE;
	if(jar != NULL) {
		do {
			if(jar[0] == 0x5f435055) { /* _CPU	*/
				switch(jar[1]) {
					case 0:
						kparam.cputype |= ATARI_68000;
						break;
					case 10:
						kparam.cputype |= ATARI_68010;
						break;
					case 20:
						kparam.cputype |= ATARI_68020;
						break;
					case 30:
						kparam.cputype |= ATARI_68030;
						break;
					case 40:
						kparam.cputype |= ATARI_68040;
						break;
					default:
						error("Unknown CPU-type");
				}
			}
			if(jar[0] == 0x5f465055) { /* _FPU	*/
				switch(jar[1]) {
					case 0x10000:
					case 0x20000:
						kparam.cputype |= ATARI_68881;
						break;
					case 0x30000:
						kparam.cputype |= ATARI_68882;
						break;
					case 0x40000:
						kparam.cputype |= ATARI_FPU40;
						break;
					default:
						error("Unknown FPU-type");
				}
			}
			jar = &jar[2];
		} while(jar[-2]);
	}
	if(!(kparam.cputype & ATARI_ANYCPU))
		error("Cannot determine CPU-type");
	if(!(kparam.cputype & ATARI_ANYFPU))
		error("Cannot determine FPU-type");

	Super(stck);
}

void error(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);

	fprintf(stderr, "%s: ", Progname);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
	/*NOTREACHED*/
}

void help()
{
	fprintf(stderr, "
NetBSD loader for the Atari-TT

Usage: %s [-abdhtv] [kernel]

Description of options:

\t-a  Boot up to multi-user mode.
\t-b  Ask for root device to use.
\t-d  Enter kernel debugger.
\t-h  What your getting right now.
\t-t  Test the loader. It will do everything except executing the
\t    loaded kernel.
\t-v  Print loader version.
", Progname);
	exit(1);
}

void usage()
{
	fprintf(stderr, "Usage: %s [-abdhtv] [kernel]", Progname);
	exit(1);
}

void start_kernel()
{
	long	stck;

	stck = Super(0);
	startit();
	/* NOT REACHED */

	Super(stck);
}

asm("
	.text
	.globl	_startit

_startit:
	move.w	#0x2700,sr

	| the BSD kernel wants values into the following registers:
	| d0:  ttmem-size
	| d1:  stmem-size
	| d2:  cputype
	| d3:  boothowto
	| d4:  length of loaded kernel
	| a0:  start of loaded kernel
	| a1:  end of symbols (esym)
	| All other registers zeroed for possible future requirements.

	lea	_kparam, a3		| a3 points to parameter block
	lea	_startit,sp		| make sure we have a good stack ***
	move.l	(a3),a0			| loaded kernel
	move.l	8(a3),-(sp)		| push entry point		***
	move.l	a0,d0			| offset of loaded kernel
	add.l	d0,(sp)			| add offset
	move.l	12(a3),d1		| stmem-size
	move.l	16(a3),d0		| ttmem-size
	move.l	20(a3),d2		| cputype
	move.l	24(a3),d3		| boothowto
	move.l	4(a3),d4		| length of loaded kernel
	move.l	28(a3),d5		| start of fastram
	move.l	32(a3),a1		| end of symbols
	sub.l	a5,a5			| target, load to 0
	btst	#4, d2			| Is this an 68040?
	beq	not040

	| Turn off 68040 MMU
	.word 0x4e7b,0xd003		| movec a5,tc
	.word 0x4e7b,0xd806		| movec a5,urp
	.word 0x4e7b,0xd807		| movec a5,srp
	.word 0x4e7b,0xd004		| movec a5,itt0
	.word 0x4e7b,0xd005		| movec a5,itt1
	.word 0x4e7b,0xd006		| movec a5,dtt0
	.word 0x4e7b,0xd007		| movec a5,dtt1
	bra	nott

not040:
	lea	zero,a3
	pmove	(a3),tcr		| Turn off MMU
	lea	nullrp,a3
	pmove	(a3),crp		| Turn off MMU some more
	pmove	(a3),srp		| Really, really, turn off MMU

	| Turn off 68030 TT registers
	btst	#3, d2			| Is this an 68030?
	beq.b	nott
	lea	zero,a3
	pmove	(a3),tt0
	pmove	(a3),tt1

nott:
	moveq.l	#0,d6			|  would have known contents)
	moveq.l	#0,d7
	movea.l	d6,a2
	movea.l	d6,a3
	movea.l	d6,a4
	movea.l	d6,a5
	movea.l	d6,a6
	rts				| enter kernel at address on stack ***


| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x80000202
zero:	.long	0
svsp:	.long   0


");
