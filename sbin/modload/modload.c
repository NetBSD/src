/*
 * modload.c
 *
 * This is the interface for the kernel module loader for statically
 * loadable kernel modules.  The interface is nearly identical to the
 * SunOS 4.1.3 not because I lack imagination but because I liked Sun's
 * approach in this particular revision of their BSD-derived OS.
 *
 * modload [-d] [-v] [-A <kernel>] [-e <entry] [-p <postinstall>]
 *         [-o <output file>] <input file>
 *
 *	-d			- debug
 *	-v			- verbose
 *	-A <kernel>		- specify symbol kernel (default="/netbsd")
 *	-e <entry>		- entry point (default="xxxinit")
 *	-p <postinstall>	- postinstall script or executable
 *	-o <output file>	- output file (default=<input file>-".o")
 *
 * Copyright (c) 1993 Terrence R. Lambert.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: modload.c,v 1.5 1994/02/10 02:06:05 cgd Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <a.out.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <paths.h>

#ifdef sun
/* these are defined in stdlib.h for everything but sun*/
extern char *optarg;
extern int optind;
#endif	/* sun*/

#ifndef DFLT_KERNEL
#define	DFLT_KERNEL	_PATH_UNIX
#endif	/* !DFLT_KERNEL*/

#ifndef DFLT_ENTRY
#define	DFLT_ENTRY	"xxxinit"
#endif	/* !DFLT_ENTRY*/

/*
 * Expected linker options:
 *
 * -A		executable to link against
 * -e		entry point
 * -o		output file
 * -T		address to link to in hex (assumes it's a page boundry)
 * <target>	object file
 *
 */
/*
#define	LINKCMD		"ld -f %s -e _%s -o %s -T %x %s"
*/
#define	LINKCMD		"ld -A %s -e _%s -o %s -T %x %s"

#define	LKM_DEV		"/dev/lkm"

#ifdef MIN
#undef MIN
#endif	/* MIN*/
#define	MIN(x,y)	((x)>(y) ? (y) : (x))


int	debug = 0;
int	verbose = 0;


linkcmd( kernel, entry, outfile, address, object)
char		*kernel;
char		*entry;
char		*outfile;
unsigned int	address;
char		*object;
{
	char	cmdbuf[ 1024];
	int	err = 0;

	sprintf( cmdbuf, LINKCMD, kernel, entry, outfile, address, object);

	if( debug)
		printf( "%s\n", cmdbuf);

	switch( system( cmdbuf)) {
	case 0:				/* SUCCESS!*/
		break;
	case 1:				/* uninformitive error*/
		/*
		 * Someone needs to fix the return values from the NetBSD
		 * ld program -- it's totally uninformative.
		 *
		 * No such file		(4 on SunOS)
		 * Can't write output	(2 on SunOS)
		 * Undefined symbol	(1 on SunOS)
		 * etc.
		 */
	case 127:			/* can't load shell*/
	case 32512:
	default:
		err = 1;
		break;
	}

	return( err);
}


usage()
{
	fprintf( stderr, "usage:\n");
	fprintf( stderr,
		 "modload [-d] [-v] [-A <kernel>] [-e <entry]\n");
	fprintf( stderr,
		 "[-p <postinstall>] [-o <output file>] <input file>\n");
	exit( 1);
}

int	filopen = 0;
#define	DEV_OPEN	0x01
#define	MOD_OPEN	0x02
#define	PART_RESRV	0x04

main( ac, av)
int	ac;
char	*av[];
{
	int	ch;
	char	*kname = DFLT_KERNEL;
	char	*entry = DFLT_ENTRY;
	char	*post = NULL;
	char	*out = NULL;
	char	*modobj;
	int	devfd;
	int	modfd;
	char	modout[ 80];
	char	*p;
	struct exec	info_buf;
	int	err = 0;
	unsigned long	modsize;
	unsigned long	modentry;

	struct lmc_resrv	resrv;
	struct lmc_loadbuf	ldbuf;
	int			i;
	int			sz;
	char			buf[ MODIOBUF];
	int			bytesleft;


	while( ( ch = getopt( ac, av, "dvA:e:p:o:")) != EOF) {
		switch(ch) {
		case 'd':	debug = 1;	break;	/* debug*/
		case 'v':	verbose = 1;	break;	/* verbose*/
		case 'A':	kname = optarg;	break;	/* kernel*/
		case 'e':	entry = optarg;	break;	/* entry point*/
		case 'p':	post = optarg;	break;	/* postinstall*/
		case 'o':	out = optarg;	break;	/* output file*/
		case '?':	usage();
		default:	printf( "default!\n");
		}
	}
	ac -= optind;
	av += optind;

	if( ac != 1)
		usage();

	modobj = av[ 0];

	/*
	 * Open the virtual device device driver for exclusive use (needed
	 * to write the new module to it as our means of getting it in the
	 * kernel).
	 */
	if( ( devfd = open( LKM_DEV, O_RDWR, 0)) == -1) {
		perror( LKM_DEV);
		err = 3;
		goto done;
	}
	filopen |= DEV_OPEN;

	strcpy( modout, modobj);

	for( p = modout; *p && *p != '.'; p++)
		continue;

	if( !*p || strcmp( p, ".o")) {
		fprintf( stderr, "Module object must end in .o\n");
		err = 2;
		goto done;
	}

	if( out == NULL) {
		out = modout;
		*p == 0;
	}

	/*
	 * Prelink to get file size
	 */
	if( linkcmd( kname, entry, out, 0, modobj)) {
		fprintf( stderr,
			 "Can't prelink %s creating %s!\n", modobj, out);
		err = 1;
		goto done;
	}

	/*
	 * Pre-open the 0-linked module to get the size information
	 */
	if( ( modfd = open( out, O_RDONLY, 0)) == -1) {
		perror( out);
		err = 4;
		goto done;
	}
	filopen |= MOD_OPEN;

	/*
	 * Get the load module post load size... do this by reading the
	 * header and doing page counts.
	 */
	if( read( modfd, &info_buf, sizeof(struct exec)) == -1) {
		perror( "read");
		err = 3;
		goto done;
	}

	/*
	 * Close the dummy module -- we have our sizing information.
	 */
	close( modfd);
	filopen &= ~MOD_OPEN;


	/*
	 * Magic number...
	 */
	if( N_BADMAG( info_buf)) {
		fprintf( stderr, "Not an a.out format file\n");
		err = 4;
		goto done;
	}


	/*
	 * Calculate the size of the module
	 */
 	modsize = info_buf.a_text + info_buf.a_data;	/* amount to load*/


	/*
	 * Reserve the required amount of kernel memory -- this may fail
	 * to be successful.
	 */
	resrv.size = modsize;				/* size in bytes*/
	resrv.name = modout;				/* objname w/o ".o"*/
	resrv.slot = -1;				/* returned*/
	resrv.addr = 0;					/* returned*/
	if( ioctl( devfd, LMRESERV, &resrv) == -1) {
		perror( "LMRESERV");
		fprintf( stderr, "Can't reserve memory\n");
		err = 9;
		goto done;
	}
	filopen |= PART_RESRV;

	/*
	 * Relink at kernel load address
	 */
	if( linkcmd( kname, entry, out, resrv.addr, modobj)) {
		fprintf( stderr,
			 "Can't link %s creating %s bound to 0x%08x!\n",
			 modobj, out, resrv.addr);
		err = 1;
		goto done;
	}

	/*
	 * Open the relinked module to load it...
	 */
	if( ( modfd = open( out, O_RDONLY, 0)) == -1) {
		perror( out);
		err = 4;
		goto done;
	}
	filopen |= MOD_OPEN;


	/*
	 * Reread the header to get the actual entry point *after* the
	 * relink.
	 */
	if( read( modfd, &info_buf, sizeof(struct exec)) == -1) {
		perror( "read");
		err = 3;
		goto done;
	}

	/*
	 * Get the entry point (for initialization)
	 */
	modentry = info_buf.a_entry;			/* place to call*/

	/*
	 * Seek to the text offset to start loading...
	 */
	if( lseek( modfd, N_TXTOFF(info_buf), 0) == -1) {
		perror( "lseek");
		err = 12;
		goto done;
	}

	/*
	 * Transfer the relinked module to kernel memory in chunks of
	 * MODIOBUF size at a time.
	 */
 	bytesleft = modsize;
	for( i = 0; i < (modsize + MODIOBUF-1)/MODIOBUF; i++) {
		sz = MIN(bytesleft,MODIOBUF);
		read( modfd, buf, sz);
		ldbuf.cnt = sz;
		ldbuf.data = buf;
		if( ioctl( devfd, LMLOADBUF, &ldbuf) == -1) {
			perror( "LMLOADBUF");
			/* error*/
			fprintf( stderr, "Error transferring buffer\n");
			err = 11;
			goto done;
		}
		bytesleft -= MODIOBUF;
		if( bytesleft < 1)
			break;
	}

	/*
	 * Save ourselves before disaster (potentitally) strikes...
	 */
	sync();

	/*
	 * Trigger the module as loaded by calling the entry procedure;
	 * this will do all necessary table fixup to ensure that state
	 * is maintained on success, or blow everything back to ground
	 * zero on failure.
	 */
	if( ioctl( devfd, LMREADY, &modentry) == -1) {
		perror( "LMREADY");
		err = 14;
		goto done;
	}

	/*
	 * Success!
	 */
	filopen &= ~PART_RESRV;		/* loaded*/
	printf( "Module loaded as ID %d\n", resrv.slot);

done:
	if( filopen & PART_RESRV) {
		/*
		 * Free up kernel memory
		 */
		if( ioctl( devfd, LMUNRESRV, 0) == -1) {
			fprintf( stderr, "Can't release slot 0x%08x memory\n",
					 resrv.slot);
		}
	}

	if( filopen & DEV_OPEN)
		close( devfd);

	if( filopen & MOD_OPEN)
		close( modfd);

	exit( err);
}
