/*	$NetBSD: conf.c,v 1.3 2004/03/24 16:54:18 drochner Exp $	*/

/*
 * Copyright (c) 1996
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
 */


#include <sys/types.h>
#include <netinet/in.h>

#include <lib/libsa/stand.h>

#include "dev_net.h"

#include <nfs.h>
#include <tftp.h>

struct devsw devsw[] = {
  { "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = sizeof(devsw)/sizeof(struct devsw);

struct fs_ops file_system[] = {
#ifdef SUPPORT_NFS
  { nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
#endif
#ifdef SUPPORT_TFTP
 { tftp_open, tftp_close, tftp_read, tftp_write, tftp_seek, tftp_stat },
#endif
};
int nfsys = sizeof(file_system)/sizeof(struct fs_ops);
