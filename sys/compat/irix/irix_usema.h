/*	$NetBSD: irix_usema.h,v 1.1.2.1 2002/05/16 04:35:49 gehenna Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _IRIX_USEMA_H_
#define _IRIX_USEMA_H_

#include <sys/param.h>
#include <sys/device.h>
  
#include <compat/irix/irix_types.h>

#ifdef _KERNEL
void	irix_usemaattach __P((struct device *, struct device *, void *));
#endif

#define IRIX_USEMADEV_MINOR	1
#define IRIX_USEMACLNDEV_MINOR	0

/* From IRIX's <sys/usioctl.h> */
#define IRIX_USEMADEV		"/dev/usema"
#define IRIX_USEMACLNDEV	"/dev/usemaclone"

#define IRIX_UIOC	('u' << 16 | 's' << 8)

#define IRIX_UIOCATTACHSEMA	(IRIX_UIOC|2)
#define IRIX_UIOCBLOCK		(IRIX_UIOC|3)
#define IRIX_UIOCABLOCK		(IRIX_UIOC|4)
#define IRIX_UIOCNOIBLOCK	(IRIX_UIOC|5)
#define IRIX_UIOCUNBLOCK	(IRIX_UIOC|6)
#define IRIX_UIOCAUNBLOCK	(IRIX_UIOC|7)
#define IRIX_UIOCINIT		(IRIX_UIOC|8)
#define IRIX_UIOCGETSEMASTATE 	(IRIX_UIOC|9)
#define IRIX_UIOCABLOCKPID	(IRIX_UIOC|10)
#define IRIX_UIOCADDPID		(IRIX_UIOC|11)
#define IRIX_UIOCABLOCKQ	(IRIX_UIOC|12)
#define IRIX_UIOCAUNBLOCKQ	(IRIX_UIOC|13)
#define IRIX_UIOCIDADDR		(IRIX_UIOC|14)
#define IRIX_UIOCSETSEMASTATE 	(IRIX_UIOC|15)
#define IRIX_UIOCGETCOUNT	(IRIX_UIOC|16) 

struct irix_usattach_s {
	irix_dev_t	us_dev;
	void		*us_handle;
};
typedef struct irix_usattach_s irix_usattach_t;

struct irix_irix5_usattach_s {
	__uint32_t	us_dev;
	__uint32_t	us_handle;
};
typedef struct irix_irix5_usattach_s irix_irix5_usattach_t;

struct irix_ussemastate_s {
	int	ntid;
	int	nprepost;
	int	nfilled;
	int	nthread;
	struct irix_ussematidstate_s {
		pid_t	pid;
		int	tid;
		int	count;
	} *tidinfo;
};
typedef struct irix_ussemastate_s irix_ussemastate_t;
typedef struct irix_ussematidstate_s irix_ussematidstate_t;

#endif /* _IRIX_USEMA_H_ */
