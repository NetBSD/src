/*
 * modunload.c
 *
 * This is the loadable kernel module unload program.  The interface
 * is nearly identical to the SunOS 4.1.3 not because I lack
 * imagination but because I liked Sun's approach in this particular
 * revision of their BSD-derived OS.
 *
 * modunload [-i <module id>] [-n <module name>]
 *
 * Default behaviour is to provide a usage message.
 *
 *	-i <module id>		- unload module by id
 *	-n <module name>	- unload module by name
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
 *	$Id: modunload.c,v 1.3 1993/12/03 10:38:08 deraadt Exp $
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
		 "modunload [-i <module id>] [-n <module name>]\n");
	exit( 1);
}


main( ac, av)
int	ac;
char	*av[];
{
	int			devfd;
	int			ch;
	int			err = 0;
	int			modnum = -1;
	char			*modname = NULL;
	struct lmc_unload	ulbuf;

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
	if( ( devfd = open( LKM_DEV, O_RDWR, 0)) == -1) {
		perror( LKM_DEV);
		exit( 2);
	}

	/*
	 * Not specified?
	 */
	if( modnum == -1 && modname == NULL)
		usage();

	/*
	 * Unload the requested module.
	 */

/*
	if( modname != NULL)
		strcpy( ulbuf.name, modname);
*/
ulbuf.name = modname;

	ulbuf.id = modnum;

	if( ioctl( devfd, LMUNLOAD, &ulbuf) == -1) {
		switch( errno) {
		case EINVAL:		/* out of range*/
			fprintf( stderr, "modunload: id out of range\n");
			err = 3;
			break;
		case ENOENT:		/* no such entry*/
			fprintf( stderr, "modunload: no such module\n");
			err = 4;
			break;
		default:		/* other error (EFAULT, etc)*/
			perror( "LMUNLOAD");
			err = 5;
			break;
		}
	}

done:
	close( devfd);
	exit( err);
}


/*
 * EOF -- This file has not been truncated.
 */
