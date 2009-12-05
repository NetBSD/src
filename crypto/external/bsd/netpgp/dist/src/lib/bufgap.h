/* $Header: /cvsroot/src/crypto/external/bsd/netpgp/dist/src/lib/bufgap.h,v 1.1 2009/12/05 07:08:18 agc Exp $ */

/*
 * Copyright © 1996-1997 Alistair G. Crooks.  All rights reserved.
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
 *	This product includes software developed by Alistair G. Crooks.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef BUFGAP_H_
#define BUFGAP_H_ 20091023

#include <sys/types.h>

#include <inttypes.h>
#include <stdio.h>

#ifndef BUFGAP_VERSION_STRING
#define BUFGAP_VERSION_STRING	"20091022"
#endif

#ifndef BUFGAP_AUTHOR_STRING
#define BUFGAP_AUTHOR_STRING	"Alistair Crooks (agc@netbsd.org)"
#endif

/* Constants for Buffer Gap routines */
enum {
	BGByte,
	BGChar,
	BGLine,

	BGFromBOF,
	BGFromHere,
	BGFromEOF
};

/* this struct describes a file in memory */
typedef struct bufgap_t {
	uint64_t	 size;		/* size of file */
	uint64_t	 abc;		/* # of bytes after the gap */
	uint64_t	 bbc;		/* # of bytes before the gap */
	uint64_t	 acc;		/* # of utf chars after the gap */
	uint64_t	 bcc;		/* # of utf chars before the gap */
	uint64_t	 alc;		/* # of records after the gap */
	uint64_t	 blc;		/* # of records before the gap */
	char		*name;		/* file name - perhaps null */
	char		*buf;		/* buffer-gap buffer */
	char		 modified;	/* file has been modified */
} bufgap_t;

int bufgap_open(bufgap_t *, const char *);
void bufgap_close(bufgap_t *);
int bufgap_forwards(bufgap_t *, uint64_t, int);
int bufgap_backwards(bufgap_t *, uint64_t, int);
int bufgap_seek(bufgap_t *, int64_t, int, int);
char *bufgap_getstr(bufgap_t *);
int bufgap_getbin(bufgap_t *, void *, size_t);
int64_t bufgap_tell(bufgap_t *, int, int);
int64_t bufgap_size(bufgap_t *, int);
int bufgap_insert(bufgap_t *, const char *, int);
int bufgap_delete(bufgap_t *, uint64_t);
int bufgap_peek(bufgap_t *, int64_t);
char *bufgap_gettext(bufgap_t *, int64_t, int64_t);
int bufgap_write(bufgap_t *, FILE *);
int bufgap_dirty(bufgap_t *);

#endif /* !BUFGAP_H_ */
