/*	$NetBSD: altq_conf.h,v 1.2.2.2 2001/01/05 17:39:36 bouyer Exp $	*/
/*	$KAME: altq_conf.h,v 1.5 2000/12/14 08:12:45 thorpej Exp $	*/

/*
 * Copyright (C) 1998-2000
 *	Sony Computer Science Laboratories Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _ALTQ_ALTQ_CONF_H_
#define	_ALTQ_ALTQ_CONF_H_

#ifdef _KERNEL

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#if (__FreeBSD_version > 300000)
#define	ALTQ_KLD
#endif

#ifdef ALTQ_KLD
#include <sys/module.h>
#endif

#ifndef dev_decl
#ifdef __STDC__
#define	dev_decl(n,t)	d_ ## t ## _t n ## t
#else
#define	dev_decl(n,t)	d_/**/t/**/_t n/**/t
#endif
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
typedef int d_open_t __P((dev_t dev, int oflags, int devtype, struct proc *p));
typedef int d_close_t __P((dev_t dev, int fflag, int devtype, struct proc *p));
typedef int d_ioctl_t __P((dev_t dev, u_long cmd, caddr_t data,
			   int fflag, struct proc *p));

#define	noopen	(dev_type_open((*))) enodev
#define	noclose	(dev_type_close((*))) enodev
#define	noioctl	(dev_type_ioctl((*))) enodev
#endif /* __NetBSD__ || __OpenBSD__ */

#if defined(__OpenBSD__)
int altqopen __P((dev_t dev, int oflags, int devtype, struct proc *p));
int altqclose __P((dev_t dev, int fflag, int devtype, struct proc *p));
int altqioctl __P((dev_t dev, u_long cmd, caddr_t data, int fflag,
                           struct proc *p));
#endif

/*
 * altq queueing discipline switch structure
 */
struct altqsw {
        char		*d_name;
	d_open_t	*d_open;
	d_close_t	*d_close;
	d_ioctl_t	*d_ioctl;
};

#define	altqdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,ioctl)

#ifdef ALTQ_KLD

struct altq_module_data {
	int	type;		/* discipline type */
	int	ref;		/* reference count */
	struct	altqsw *altqsw; /* discipline functions */
};

#define	ALTQ_MODULE(name, type, devsw)					\
static struct altq_module_data name##_moddata = { type, 0, devsw };	\
									\
moduledata_t name##_mod = {						\
    #name,								\
    altq_module_handler,						\
    &name##_moddata							\
};									\
DECLARE_MODULE(name, name##_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE+96)

void altq_module_incref __P((int));
void altq_module_declref __P((int));
int altq_module_handler __P((module_t, int, void *));

#endif /* ALTQ_KLD */

#endif /* _KERNEL */
#endif /* _ALTQ_ALTQ_CONF_H_ */
