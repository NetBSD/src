/*	$NetBSD: conf.c,v 1.2.74.1 2009/08/19 18:46:25 yamt Exp $	 */

/*
 * Copyright (c) 2004
 *	The NetBSD Foundation.  All rights reserved.
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


#include <sys/cdefs.h>

#include <sys/types.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <bootstrap.h>

#include <efi.h>
#include <efilib.h>

#include <efiboot.h>

#include "efifsdev.h"
#include "dev_net.h"

struct devsw devsw[] = { 
	{"disk", efifs_dev_strategy, efifs_dev_open, efifs_dev_close, noioctl},
}; 

int ndevs = sizeof(devsw) / sizeof(struct devsw);

struct fs_ops file_system[] = {
        FS_OPS(efifs),
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
 * We don't prototype these in efiboot.h because they require
 * data structures from bootstrap.h as well.
 */
extern struct console efi_console;

struct console *consoles[] = {
	&efi_console,
	NULL
};
