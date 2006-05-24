/*	$NetBSD: efifsdev.h,v 1.1.10.2 2006/05/24 15:48:08 tron Exp $	*/

/*
 * Copyright (c) 2005
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

int efifs_dev_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int efifs_dev_open(struct open_file *, ...);
int efifs_dev_close(struct open_file *);
int efifs_dev_init(void);

int efifs_open(const char *, struct open_file *);
int efifs_close(struct open_file *);
int efifs_read(struct open_file *, void *, size_t, size_t *);
int efifs_write(struct open_file *, void *, size_t, size_t *);
off_t efifs_seek(struct open_file *, off_t, int );
int efifs_stat(struct open_file *, struct stat *);

