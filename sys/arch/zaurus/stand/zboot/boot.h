/*	$NetBSD: boot.h,v 1.1.6.2 2009/05/13 17:18:54 jym Exp $	*/

/*
 * Copyright (c) 2009 NONAKA Kimihiro
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

#ifndef	_STAND_BOOT_H_
#define	_STAND_BOOT_H_

#include <sys/param.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

extern int debug;

/* boot.c */
char *default_devname;
uint default_unit, default_partition;
const char *default_filename;
extern int default_timeout;

void boot(dev_t);
int parsebootfile(const char *fname, char **fsname, char **devname,
    uint *unit, uint *partition, const char **file);
char *sprint_bootsel(const char *filename);
void bootit(const char *filename, int howto, int tell);
void docommand(char *arg);
void bootmenu(void);

/* conf.c */
extern char devname_hd[];
extern char devname_mmcd[];

/* loadfile_zboot.c */
int loadfile_zboot(const char *fname, u_long *marks, int flags);

#endif	/* _STAND_BOOT_H_ */
