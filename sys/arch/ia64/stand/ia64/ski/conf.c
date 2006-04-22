/*	$NetBSD: conf.c,v 1.1.6.2 2006/04/22 11:37:39 simonb Exp $	*/

/*-
 * Copyright (c) 1997
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 *	$NetBSD: conf.c,v 1.1.6.2 2006/04/22 11:37:39 simonb Exp $	 
 */

#include <sys/cdefs.h>

#include <lib/libsa/stand.h>

#include <bootstrap.h>

#include "libski.h"


/* skifs related prototypes. Should move into a header file if this bloats. */

extern int skifs_dev_strategy(void *, int, daddr_t, size_t, void *, size_t *);
extern int skifs_dev_open(struct open_file *, ...);
extern int skifs_dev_close(struct open_file *);

extern int skifs_open(const char *, struct open_file *);
extern int skifs_close(struct open_file *);
extern int skifs_read(struct open_file *, void *, size_t, size_t *);

#define skifs_write null_write

extern off_t skifs_seek(struct open_file *, off_t, int);
extern int skifs_stat(struct open_file *, struct stat *);

extern struct devsw devsw[];


/*
 * We could use linker sets for some or all of these, but
 * then we would have to control what ended up linked into
 * the bootstrap.  So it's easier to conditionalise things
 * here.
 *
 * XXX rename these arrays to be consistent and less namespace-hostile
 */

struct devsw devsw[] = {
	{"skidisk", skifs_dev_strategy, skifs_dev_open, skifs_dev_close, noioctl},
};

int ndevs = sizeof(devsw) / sizeof(struct devsw);

struct fs_ops file_system[] = {
	FS_OPS(skifs),
};

int nfsys = sizeof(file_system) / sizeof(struct fs_ops);

/* Exported for ia64 only */
/* 
 * Sort formats so that those that can detect based on arguments
 * rather than reading the file go first.
 */
extern struct file_format ia64_elf;

struct file_format *file_formats[] = {
	&ia64_elf,
	NULL
};

/* 
 * Consoles 
 *
 * We don't prototype these in libalpha.h because they require
 * data structures from bootstrap.h as well.
 */
extern struct console ski_console;

struct console *consoles[] = {
	&ski_console,
	NULL
};
