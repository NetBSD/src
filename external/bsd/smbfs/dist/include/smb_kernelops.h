/*      $NetBSD: smb_kernelops.h,v 1.1.8.2 2014/08/19 23:52:13 tls Exp $        */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SMBFS_KERNEL_OPS_H_
#define _SMBFS_KERNEL_OPS_H_

#include <sys/types.h>
#include <sys/socket.h>

#include <fcntl.h>

struct smb_kernelops {
	int (*ko_open)(const char *, int, ...);
	int (*ko_ioctl)(int, unsigned long, ...);
	int (*ko_close)(int);

	int (*ko_socket)(int, int, int);
	int (*ko_setsockopt)(int, int, int, const void *, socklen_t);
	int (*ko_bind)(int, const struct sockaddr *, socklen_t);
	ssize_t (*ko_sendto)(int, const void *, size_t, int,
			     const struct sockaddr *, socklen_t);
	ssize_t (*ko_recvfrom)(int, void *, size_t, int,
			      struct sockaddr *, socklen_t *);
};
extern const struct smb_kernelops smb_kops;

#endif /* _SMBFS_KERNEL_OPS_H_ */
