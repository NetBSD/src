/*	$NetBSD: loadbsd.c,v 1.17 2001/10/10 14:24:48 leo Exp $	*/

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

#include "exec_elf.h"
#include <a_out.h>
#include <fcntl.h>
#include <stdio.h>
#include <osbind.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libtos.h"
#include "tosdefs.h"

#ifdef COMPRESSED_READ
#define	open	copen
#define	read	cread
#define	lseek	clseek
#define	close	cclose
#endif /* COMPRESSED_READ */

char	*Progname;		/* How are we called		*/
int	d_flag  = 0;		/* Output debugging output?	*/
int	h_flag  = 0;		/* show help			*/
int	N_flag  = 0;		/* No symbols?			*/
int	s_flag  = 0;		/* St-ram only			*/
int	t_flag  = 0;		/* Just test, do not execute	*/
int	v_flag  = 0;		/* show version			*/

const char version[] = "$Revision: 1.17 $";

/*
 * Default name of kernel to boot, large enough to patch
 */
char	kname[80] = "n:/netbsd";

static osdsc_t	kernelparms;

void help  PROTO((void));
void usage PROTO((void));
void get_sys_info PROTO((osdsc_t *));
void start_kernel PROTO((osdsc_t *));

#define ELFMAGIC	((ELFMAG0 << 24) | (ELFMAG1 << 16) | \
				(ELFMAG2 << 8) | ELFMAG3)
int
main(argc, argv)
int	argc;
char	**argv;
{
	/*
	 * Option parsing
	 */
	extern	int	optind;
	extern	char	*optarg;
	int		ch, err;
	char		*errmsg;
	int		fd;
	osdsc_t		*od;
	
	init_toslib(argv[0]);
	Progname = argv[0];

	od = &kernelparms;
	od->boothowto = RB_SINGLE;

	while ((ch = getopt(argc, argv, "abdDhNstVwo:S:T:")) != -1) {
		switch (ch) {
		case 'a':
			od->boothowto &= ~(RB_SINGLE);
			od->boothowto |= RB_AUTOBOOT;
			break;
		case 'b':
			od->boothowto |= RB_ASKNAME;
			break;
		case 'd':
			od->boothowto |= RB_KDB;
			break;
		case 'D':
			d_flag = 1;
			break;
		case 'h':
			h_flag = 1;
			break;
		case 'N':
			N_flag = 1;
			break;
		case 'o':
			redirect_output(optarg);
			break;
		case 's':
			s_flag = 1;
			break;
		case 'S':
			od->stmem_size = atoi(optarg);
			break;
		case 't':
			t_flag = 1;
			break;
		case 'T':
			od->ttmem_size = atoi(optarg);
			break;
		case 'V':
			v_flag = 1;
			break;
		case 'w':
			set_wait_for_key();
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 1)
		strcpy(kname, argv[0]);

	if (h_flag)
		help();
	if (v_flag)
		eprintf("%s\r\n", version);

	/*
	 * Get system info to pass to NetBSD
	 */
	get_sys_info(od);
	if (d_flag) {
	    eprintf("Machine info:\r\n");
	    eprintf("ST-RAM size\t: %10d bytes\r\n",od->stmem_size);
	    eprintf("TT-RAM size\t: %10d bytes\r\n",od->ttmem_size);
	    eprintf("TT-RAM start\t: 0x%08x\r\n", od->ttmem_start);
	    eprintf("Cpu-type\t: 0x%08x\r\n", od->cputype);
	}

	/*
	 * Find the kernel to boot and read it's exec-header
	 */
	if ((fd = open(kname, O_RDONLY)) < 0)
		fatal(-1, "Cannot open kernel '%s'", kname);
	if ((err = elf_load(fd, od, &errmsg, !N_flag)) == -1) {
		/*
		 * Not ELF, try a.out
		 */
		if (err = aout_load(fd, od, &errmsg, !N_flag)) {
			if (err == -1) 
				errmsg = "Not an ELF or NMAGIC file '%s'";
			fatal(-1, errmsg, kname);
		}
	}
	else {
		if (err)
			fatal(-1, errmsg);
	}

	close(fd);

	if (d_flag) {
	    eprintf("\r\nKernel info:\r\n");
	    eprintf("Kernel loadaddr\t: 0x%08x\r\n", od->kstart);
	    eprintf("Kernel size\t: %10d bytes\r\n", od->ksize);
	    eprintf("Kernel entry\t: 0x%08x\r\n", od->kentry);
	    eprintf("Kernel esym\t: 0x%08x\r\n", od->k_esym);
	}

	if (!t_flag)
		start_kernel(od);
		/* NOT REACHED */

	eprintf("Kernel '%s' was loaded OK\r\n", kname);
	xexit(0);
	return 0;
}

void
get_sys_info(od)
osdsc_t	*od;
{
	long	stck;

	stck = Super(0);

	sys_info(od);

	if (!(od->cputype & ATARI_ANYCPU))
		fatal(-1, "Cannot determine CPU-type");

	(void)Super(stck);
	if (s_flag)
 		od->ttmem_size = od->ttmem_start = 0;
}

void
help()
{
	eprintf("\r
NetBSD loader for the Atari-TT\r
\r
Usage: %s [-abdhstVD] [-S <stram-size>] [-T <ttram-size>] [kernel]\r
\r
Description of options:\r
\r
\t-a  Boot up to multi-user mode.\r
\t-b  Ask for root device to use.\r
\t-d  Enter kernel debugger.\r
\t-D  printout debug information while loading\r
\t-h  What you're getting right now.\r
`t-N  No symbols must be loaded.\r
\t-o  Write output to both <output file> and stdout.\r
\t-s  Use only ST-compatible RAM\r
\t-S  Set amount of ST-compatible RAM\r
\t-T  Set amount of TT-compatible RAM\r
\t-t  Test the loader. It will do everything except executing the\r
\t    loaded kernel.\r
\t-V  Print loader version.\r
\t-w  Wait for a keypress before exiting.\r
", Progname);
	xexit(0);
}

void
usage()
{
	eprintf("Usage: %s [-abdhstVD] [-S <stram-size>] "
		"[-T <ttram-size>] [kernel]\r\n", Progname);
	xexit(1);
}

void
start_kernel(od)
osdsc_t	*od;
{
	long	stck;

	stck = Super(0);
	bsd_startup(&(od->kp));
	/* NOT REACHED */

	(void)Super(stck);
}
