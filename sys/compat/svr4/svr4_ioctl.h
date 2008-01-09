/*	$NetBSD: svr4_ioctl.h,v 1.9.16.1 2008/01/09 01:51:52 matt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef	_SVR4_IOCTL_H_
#define	_SVR4_IOCTL_H_

#define	SVR4_IOC_VOID	0x20000000
#define	SVR4_IOC_OUT	0x40000000
#define	SVR4_IOC_IN	0x80000000
#define	SVR4_IOC_INOUT	(SVR4_IOC_IN|SVR4_IOC_OUT)

#define	SVR4_IOC(inout,group,num,len) \
	(inout | ((len & 0xff) << 16) | ((group) << 8) | (num))

#define SVR4_XIOC	('X' << 8)

#define	SVR4_IO(g,n)		SVR4_IOC(SVR4_IOC_VOID,	(g), (n), 0)
#define	SVR4_IOR(g,n,t)		SVR4_IOC(SVR4_IOC_OUT,	(g), (n), sizeof(t))
#define	SVR4_IOW(g,n,t)		SVR4_IOC(SVR4_IOC_IN,	(g), (n), sizeof(t))
#define	SVR4_IOWR(g,n,t)	SVR4_IOC(SVR4_IOC_INOUT,(g), (n), sizeof(t))

int	svr4_stream_ti_ioctl(struct file *, struct lwp *, register_t *,
			          int, u_long, void *);
int	svr4_stream_ioctl(struct file *, struct lwp *, register_t *,
				  int, u_long, void *);
int	svr4_term_ioctl(struct file *, struct lwp *, register_t *,
				  int, u_long, void *);
int	svr4_ttold_ioctl(struct file *, struct lwp *, register_t *,
				  int, u_long, void *);
int	svr4_fil_ioctl(struct file *, struct lwp *, register_t *,
				  int, u_long, void *);
int	svr4_sock_ioctl(struct file *, struct lwp *, register_t *,
				  int, u_long, void *);

#endif /* !_SVR4_IOCTL_H_ */
