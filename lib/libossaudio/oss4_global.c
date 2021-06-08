/*	$NetBSD: oss4_global.c,v 1.1 2021/06/08 18:43:54 nia Exp $	*/

/*-
 * Copyright (c) 2020-2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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
#include <errno.h>
#include "internal.h"

oss_private int
_oss4_global_ioctl(int fd, unsigned long com, void *argp)
{
	int retval = 0;

	switch (com) {
	/*
	 * These ioctls were added in OSSv4 with the idea that
	 * applications could apply strings to audio devices to
	 * display what they are using them for (e.g. with song
	 * names) in mixer applications. In practice, the popular
	 * implementations of the API in FreeBSD and Solaris treat
	 * these as a no-op and return EINVAL, and no software in the
	 * wild seems to use them.
	 */
	case SNDCTL_SETSONG:
	case SNDCTL_GETSONG:
	case SNDCTL_SETNAME:
	case SNDCTL_SETLABEL:
	case SNDCTL_GETLABEL:
		errno = EINVAL;
		retval = -1;
		break;
	default:
		errno = EINVAL;
		retval = -1;
		break;
	}
	return retval;
}
