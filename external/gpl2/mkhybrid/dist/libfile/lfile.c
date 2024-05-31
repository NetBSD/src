/* @(#)file.c	1.10 09/08/04 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)file.c	1.10 09/08/04 joerg";
#endif
/*
**	find file types by using a modified "magic" file
**
**	based on file v3.22 by Ian F. Darwin (see below)
**
**	For each entry in the magic file, the message MUST start with
**	two 4 character strings which are the CREATOR and TYPE for the
**	Mac file. Any continuation lines are ignored. e.g magic entry
**	for a GIF file:
**	
**	0       string          GIF8            8BIM GIFf
**	>4      string          7a              \b, version 8%s,
**	>4      string          9a              \b, version 8%s,
**	>6      leshort         >0              %hd x
**	>8      leshort         >0              %hd,
**	#>10    byte            &0x80           color mapped,
**	#>10    byte&0x07       =0x00           2 colors
**	#>10    byte&0x07       =0x01           4 colors
**	#>10    byte&0x07       =0x02           8 colors
**	#>10    byte&0x07       =0x03           16 colors
**	#>10    byte&0x07       =0x04           32 colors
**	#>10    byte&0x07       =0x05           64 colors
**	#>10    byte&0x07       =0x06           128 colors
**	#>10    byte&0x07       =0x07           256 colors
**
**	Just the "8BIM" "GIFf" will be used whatever the type GIF file
**	it is.
**
**	Modified for mkhybrid James Pearson 19/5/98
*/

/*
 * file - find type of a file or files - main program.
 *
 * Copyright (c) Ian F. Darwin, 1987.
 * Written by Ian F. Darwin.
 *
 * This software is not subject to any export provision of the United States
 * Department of Commerce, and may be exported to any country or planet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef	lint
static UConst char moduleid[] = 
	"@(#)Id: file.c,v 1.38 1997/01/15 19:28:35 christos Exp";
#endif	/* lint */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* for read() */
#include <schily/stat.h>
#include <schily/fcntl.h>	/* for open() */

#ifdef RESTORE_TIME
#include <schily/utime.h>
#ifdef HAVE_UTIMES
#define	USE_UTIMES
#endif
#endif

#if 0
#include <schily/in.h>		/* for byte swapping */
#endif

#include "patchlevel.h"
#include "file.h"

#ifdef MAIN
 			/* Global command-line options 		*/
#ifdef DEBUG
int	debug = 1; 	/* debugging 				*/
#else
int	debug = 0; 	/* debugging 				*/
#endif /* DEBUG */
int	lflag = 0;	/* follow Symlinks (BSD only) 		*/
int	zflag = 0;	/* follow (uncompress) compressed files */

			/* Misc globals				*/
char *magicfile;	/* where magic be found 		*/

char *progname;		/* used throughout 			*/
#endif

char *	get_magic_match	__PR((const char *inname));
void	clean_magic	__PR((void));

#if 0
static int	byteconv4	__P((int, int, int));
static short	byteconv2	__P((int, int, int));
#endif

#if 0
/*
 * byteconv4
 * Input:
 *	from		4 byte quantity to convert
 *	same		whether to perform byte swapping
 *	big_endian	whether we are a big endian host
 */
static int
byteconv4(from, same, big_endian)
    int from;
    int same;
    int big_endian;
{
  if (same)
    return from;
  else if (big_endian)		/* lsb -> msb conversion on msb */
  {
    union {
      int i;
      char c[4];
    } retval, tmpval;

    tmpval.i = from;
    retval.c[0] = tmpval.c[3];
    retval.c[1] = tmpval.c[2];
    retval.c[2] = tmpval.c[1];
    retval.c[3] = tmpval.c[0];

    return retval.i;
  }
  else
    return ntohl(from);		/* msb -> lsb conversion on lsb */
}

/*
 * byteconv2
 * Same as byteconv4, but for shorts
 */
static short
byteconv2(from, same, big_endian)
	int from;
	int same;
	int big_endian;
{
  if (same)
    return from;
  else if (big_endian)		/* lsb -> msb conversion on msb */
  {
    union {
      short s;
      char c[2];
    } retval, tmpval;

    tmpval.s = (short) from;
    retval.c[0] = tmpval.c[1];
    retval.c[1] = tmpval.c[0];

    return retval.s;
  }
  else
    return ntohs(from);		/* msb -> lsb conversion on lsb */
}
#endif

/*
 * get_magic_match - get the CREATOR/TYPE string
 * based on the original process()
 */
char *
get_magic_match(inname)
const char	*inname;
{
	int	fd = 0;
	unsigned char	buf[HOWMANY+1];	/* one extra for terminating '\0' */
	struct stat	sb;
	int nbytes = 0;	/* number of bytes read from a datafile */
	char *match;

	/* check the file is regular and non-zero length */
	if (stat(inname, &sb) != 0)
		return 0;

	if (sb.st_size == 0 || ! S_ISREG(sb.st_mode))
		return 0;

	if ((fd = open(inname, O_RDONLY)) < 0)
		    return 0;

	/*
	 * try looking at the first HOWMANY bytes
	 */
	if ((nbytes = read(fd, (char *)buf, HOWMANY)) == -1)
		return 0;

	if (nbytes == 0)
		return 0;
	else {
		buf[nbytes++] = '\0';	/* null-terminate it */
		match = softmagic(buf, nbytes);
	}

#ifdef RESTORE_TIME
	/* really no point as we going to access the file later anyway */
	{
		/*
		 * Try to restore access, modification times if read it.
		 */
# ifdef USE_UTIMES
		struct timeval  utsbuf[2];
		utsbuf[0].tv_sec = sb.st_atime;
		utsbuf[1].tv_sec = sb.st_mtime;

		(void) utimes(inname, utsbuf); /* don't care if loses */
# else
		struct utimbuf  utbuf;

		utbuf.actime = sb.st_atime;
		utbuf.modtime = sb.st_mtime;
		(void) utime(inname, &utbuf); /* don't care if loses */
# endif
	}
#endif
	(void) close(fd);

	return(match);
}

/*
 * clean_magic - deallocate memory used
 */
void
clean_magic()
{
	if (__f_magic)
		free(__f_magic);
}
	

#ifdef MAIN
main(argc, argv)
int	argc;
char	**argv;
{
	char	*ret;
	char	creator[5];
	char	type[5];

	if (argc < 3)
		exit(1);

	init_magic(argv[1]);

	ret = get_magic_match(argv[2]);

	if (!ret)
		ret = "unixTEXT";

	sscanf(ret, "%4s%4s", creator, type);

	creator[4] = type[4] = '\0';

	printf("%s %s\n", creator, type);


	exit(0);
}
#endif /* MAIN */

