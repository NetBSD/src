/*	$NetBSD: msconfig.c,v 1.3 1999/01/11 22:21:30 kleink Exp $	*/

/*
 * Copyright (c) 1996 Thomas Gerner.
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
 *      This product includes software developed by Thomas Gerner.
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <machine/msioctl.h>

void usage(char *name)
{
    fprintf(stderr,"usage: %s [ -f dev ] [ on | off ]\n", name);
    exit(1);
}

int
main(int argc, char **argv)
{
    char *mousedev = "/dev/mouse";
    int set_emul = 0;
    int emul3b = 0;
    int i = 1;
    int fd;

    if ((i < argc) && !strcmp(argv[i], "-f")) {
	i++;
	if(i >= argc)
	    usage(argv[0]);
	mousedev = argv[i];
	i++;
    }
    if ((i < argc) && !strcmp(argv[i], "on")) {
	i++;
	set_emul = 1;
	emul3b = 1;
    } else if ((i < argc) && !strcmp(argv[i], "off")) {
	i++;
	set_emul = 1;
    }
    if (i < argc)
	usage(argv[0]);

    if ((fd = open(mousedev,O_RDONLY | O_NONBLOCK)) < 0) {
	fprintf(stderr, "%s: open %s: %s\n", argv[0], mousedev,
		strerror(errno));
	exit(1);
    }
    if(set_emul) {
	if(ioctl(fd, MIOCS3B_EMUL, (char *)&emul3b) < 0) {;
	    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
	    close(fd);
	    exit(1);
	}
    }
    if(ioctl(fd, MIOCG3B_EMUL, (char *)&emul3b) < 0) {;
	fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
	close(fd);
	exit(1);
    }
    if(emul3b)
	printf("State of 3 button emulation: ON\n");
    else
	printf("State of 3 button emulation: OFF\n");
    
    close(fd);
    exit(0);
}
