/*
 * Copyright (c) 1992,1993,1994 Hellmuth Michaelis and Brian Dunford-Shore
 *
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
 *	This product includes software developed by
 *	Hellmuth Michaelis and Brian Dunford-Shore
 * 4. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

static char *id =
	"@(#)cursor.c, 3.00, Last Edit-Date: [Mon Jan 10 21:16:04 1994]";

#include <stdio.h>
#include <machine/pcvt_ioctl.h>

main(argc,argv)
int argc;
char *argv[];
{
	extern int optind;
	extern int opterr;
	extern char *optarg;

	struct cursorshape cursorshape;
	int c;
	int screen = -1;
	int start = -1;
	int end = -1;
	
	while( (c = getopt(argc, argv, "n:s:e:")) != EOF)
	{
		switch(c)
		{
			case 'n':
				screen = atoi(optarg);
				break;
				
			case 's':
				start = atoi(optarg);
				break;
				
			case 'e':
				end = atoi(optarg);
				break;

			case '?':
			default:
				usage();
				break;
		}
	}

	if(start == -1 || end == -1)	
		usage();

	cursorshape.start = start;
	cursorshape.end = end;
	cursorshape.screen_no = screen;

	if(ioctl(1, VGACURSOR, &cursorshape) == -1)
	{
		perror("cursor - ioctl VGACURSOR failed, error");
		exit(1);
	}
	else
		exit(0);
}

usage()
{
	fprintf(stderr,"\ncursor - set cursor shape for pcvt video driver\n");
	fprintf(stderr,"usage: cursor -n [no] -s [line] -e [line]\n");
	fprintf(stderr,"       -n <no>   screen no if specified, else current screen\n");
	fprintf(stderr,"       -s <line> start scan line (topmost scan line)\n");
	fprintf(stderr,"       -e <line> ending scan line (bottom scan line)\n\n");
	exit(1);
}

