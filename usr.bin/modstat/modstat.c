/*
 * modstat.c
 *
 * This is the loadable kernel module status display program.  The
 * interface is nearly identical to the SunOS 4.1.3 not because I
 * lack imagination but because I liked Sun's approach in this
 * particular revision of their BSD-derived OS.
 *
 * modstat [-i <module id>] [-n <module name>]
 *
 * Default behaviour is to report status for all modules.
 *
 *	-i <module id>		- status for module by id
 *	-n <module name>	- status for module by name
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
 *	$Id: modstat.c,v 1.2 1993/12/03 10:39:31 deraadt Exp $
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

extern int	errno;	/* should be in errno.h*/


#ifdef sun
/* these are defined in stdlib.h for everything but sun*/
extern char *optarg;
extern int optind;
#endif	/* sun*/

#define	LKM_DEV		"/dev/lkm"


extern int dostat();


usage()
{
	fprintf( stderr, "usage:\n");
	fprintf( stderr,
		 "modstat [-i <module id>] [-n <module name>]\n");
	exit( 1);
}


main( ac, av)
int	ac;
char	*av[];
{
	int	devfd;
	int	i;
	int	ch;
	int	err = 0;
	int	modnum = -1;
	char	*modname = NULL;

	while( ( ch = getopt( ac, av, "i:n:")) != EOF) {
		switch(ch) {
		case 'i':	modnum = atoi( optarg);	break;	/* number*/
		case 'n':	modname = optarg;	break;	/* name*/
		case '?':	usage();
		default:	printf( "default!\n");
		}
	}
	ac -= optind;
	av += optind;

	if( ac != 0)
		usage();


	/*
	 * Open the virtual device device driver for exclusive use (needed
	 * to ioctl() to retrive the loaded module(s) status).
	 */
	if( ( devfd = open( LKM_DEV, O_RDONLY, 0)) == -1) {
		perror( LKM_DEV);
		exit( 2);
	}

	printf( "Type    Id  Off Loadaddr Size Info     Rev Module Name\n");

	/*
	 * Oneshot?
	 */
	if( modnum != -1 || modname != NULL) {
		if( dostat( devfd, modnum, modname))
			err = 3;
		goto done;
	}

	/*
	 * Start at 0 and work up until "EEXIST"
	 */
 	for( modnum = 0; dostat( devfd, modnum, NULL) < 2; modnum++)
 		continue;

done:
	close( devfd);
	exit( err);
}


static char	*type_names[] = {
	"SYSCALL",
	"VFS",
	"DEV",
	"STRMOD",
	"EXEC",
	"MISC"
};

int
dostat( devfd, modnum, modname)
int	devfd;
int	modnum;
char	*modname;
{
	struct lmc_stat	sbuf;

	if( modname != NULL)
		strcpy( sbuf.name, modname);

	sbuf.id = modnum;

	if( ioctl( devfd, LMSTAT, &sbuf) == -1) {
		switch( errno) {
		case EINVAL:		/* out of range*/
			return( 2);
		case ENOENT:		/* no such entry*/
			return( 1);
		default:		/* other error (EFAULT, etc)*/
			perror( "LMSTAT");
			return( 4);
		}
	}

	/*
	 * Decode this stat buffer...
	 */
	printf( "%-7s %3d %3d %08x %04x %8x %3d %s\n",
		type_names[ sbuf.type],
		sbuf.id,		/* module id*/
		sbuf.offset,		/* offset into modtype struct*/
		sbuf.area,		/* address module loaded at*/
		sbuf.size,		/* size in pages(K)*/
		sbuf.private,		/* kernel address of private area*/
		sbuf.ver,		/* Version; always 1 for now*/
		sbuf.name		/* name from private area*/
	);

	/*
	 * Done (success).
	 */
	return( 0);
}


/*
 * EOF -- This file has not been truncated.
 */
