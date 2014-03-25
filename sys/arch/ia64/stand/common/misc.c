/*	$NetBSD: misc.c,v 1.7 2014/03/25 18:35:32 christos Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/common/misc.c,v 1.8.4.1 2004/09/03 19:25:40 iedowse Exp $"); */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <bootstrap.h>


#define	min(A, B)	(((A) < (B)) ? (A) : (B))

/*
 * Concatenate the (argc) elements of (argv) into a single string, and return
 * a copy of same.
 */
char *
unargv(int argc, char *argv[])
{
    size_t	hlong;
    int		i;
    char	*cp;

    for (hlong = 0, i = 0, hlong = 0; i < argc; i++)
	hlong += strlen(argv[i]) + 2;

    if(hlong == 0)
	return(NULL);

    cp = alloc(hlong);
    cp[0] = 0;
    for (i = 0; i < argc; i++) {
	strcat(cp, argv[i]);
	if (i < (argc - 1))
	  strcat(cp, " ");
    }
	  
    return(cp);
}

/*
 * Get the length of a string in kernel space
 */
size_t
strlenout(vaddr_t src)
{
    char	c;
    size_t	len;
    
    for (len = 0; ; len++) {
	archsw.arch_copyout(src++, &c, 1);
	if (c == 0)
	    break;
    }
    return(len);
}

/*
 * Make a duplicate copy of a string in kernel space
 */
char *
strdupout(vaddr_t str)
{
    char	*result, *cp;
    
    result = alloc(strlenout(str) + 1);
    for (cp = result; ;cp++) {
	archsw.arch_copyout(str++, cp, 1);
	if (*cp == 0)
	    break;
    }
    return(result);
}

/* Zero a region in kernel space. */
void
kern_bzero(vaddr_t dest, size_t len)
{
	char buf[256];
	size_t chunk, resid;

	memset(buf, 0, sizeof(buf));
	resid = len;
	while (resid > 0) {
		chunk = min(sizeof(buf), resid);
		archsw.arch_copyin(buf, dest, chunk);
		resid -= chunk;
		dest += chunk;
	}
}

/*
 * Read the specified part of a file to kernel space.  Unlike regular
 * pread, the file pointer is advanced to the end of the read data,
 * and it just returns 0 if successful.
 */
int
kern_pread(int fd, vaddr_t dest, size_t len, off_t off)
{
	ssize_t nread;

	if (lseek(fd, off, SEEK_SET) == -1) {
		printf("\nlseek failed\n");
		return (-1);
	}
	nread = archsw.arch_readin(fd, dest, len);
	if (nread != len) {
		printf("\nreadin failed\n");
		return (-1);
	}
	return (0);
}

/*
 * Read the specified part of a file to a malloced buffer.  The file
 * pointer is advanced to the end of the read data.
 */
void *
alloc_pread(int fd, off_t off, size_t len)
{
	void *buf;
	ssize_t nread;

	buf = alloc(len);
	if (buf == NULL) {
		printf("\nalloc(%d) failed\n", (int)len);
		return (NULL);
	}
	if (lseek(fd, off, SEEK_SET) == -1) {
		printf("\nlseek failed\n");
		free(buf);
		return (NULL);
	}
	nread = read(fd, buf, len);
	if (nread != len) {
		printf("\nread failed\n");
		free(buf);
		return (NULL);
	}
	return (buf);
}

/*
 * Display a region in traditional hexdump format.
 */
void
hexdump(void *region, size_t len)
{
    void *	line;
    int		x, c;
    char	lbuf[80];
#define emit(fmt, args...)	{snprintf(lbuf, sizeof(lbuf), fmt , ## args); pager_output(lbuf);}

    pager_open();
    for (line = region; line < (region + len); line += 16) {
	emit("%08lx  ", (long) line);
	
	for (x = 0; x < 16; x++) {
	    if ((line + x) < (region + len)) {
		emit("%02x ", *(u_int8_t *)(line + x));
	    } else {
		emit("-- ");
	    }
	    if (x == 7)
		emit(" ");
	}
	emit(" |");
	for (x = 0; x < 16; x++) {
	    if ((line + x) < (region + len)) {
		c = *(u_int8_t *)(line + x);
		if ((c < ' ') || (c > '~'))	/* !isprint(c) */
		    c = '.';
		emit("%c", c);
	    } else {
		emit(" ");
	    }
	}
	emit("|\n");
    }
    pager_close();
}

void
dev_cleanup(void)
{

}
