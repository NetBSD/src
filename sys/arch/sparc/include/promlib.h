/*	$NetBSD: promlib.h,v 1.17 2004/03/18 15:13:59 pk Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * OPENPROM functions.  These are here mainly to hide the OPENPROM interface
 * from the rest of the kernel.
 */

#ifndef _SPARC_PROMLIB_H_
#define _SPARC_PROMLIB_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#include <machine/idprom.h>
#include <machine/bsd_openprom.h>
#include <machine/openfirm.h>

/*
 * A set of methods to access the firmware.
 * We use this to support various versions of Open Boot Prom
 * or Open Firmware implementations.
 */
struct promops {
	int	po_version;		/* PROM version number */
#define PROM_OLDMON	0
#define PROM_OBP_V0	1
#define PROM_OBP_V2	2
#define PROM_OBP_V3	3
#define PROM_OPENFIRM	4
	int	po_revision;		/* revision level */
	int	po_stdin;		/* stdio handles */
	int	po_stdout;		/* */
	void	*po_bootcookie;

	/* Access to boot arguments */
	char	*(*po_bootpath)(void);
	char	*(*po_bootfile)(void);
	char	*(*po_bootargs)(void);

	/* I/O functions */
	int	(*po_getchar)(void);
	int	(*po_peekchar)(void);
	void	(*po_putchar)(int);
	void	(*po_putstr)(char *, int);
	int	(*po_open)(char *);
	void	(*po_close)(int);
	int	(*po_read)(int, void *, int);
	int	(*po_write)(int, void *, int);
	int	(*po_seek)(int, u_quad_t);

	int	(*po_instance_to_package)(int);

	/* Misc functions (common in OBP 0,2,3) */
	void	(*po_halt)(void)	__attribute__((__noreturn__));
	void	(*po_reboot)(char *)	__attribute__((__noreturn__));
	void	(*po_abort)(void);
	void	(*po_interpret)(char *);
	void	(*po_setcallback)(void (*)(void));
	int	(*po_ticks)(void);
	void	*po_tickdata;

	/* sun4/sun4c only */
	void	(*po_setcontext)(int ctxt, caddr_t va, int pmeg);

	/* MP functions (OBP v3 only) */
	int	(*po_cpustart)(int, struct openprom_addr *, int, caddr_t);
	int	(*po_cpustop)(int);
	int	(*po_cpuidle)(int);
	int	(*po_cpuresume)(int);

	/* Device node traversal (OBP v0, v2, v3; but not sun4) */
	int	(*po_firstchild)(int);
	int	(*po_nextsibling)(int);

	/* Device node properties */
	int	(*po_getproplen)(int node, char *name);
	int	(*po_getprop)(int node, char *name, void *, int);
	int	(*po_setprop)(int node, char *name, const void *, int);
	char	*(*po_nextprop)(int node, char *name);

	int	(*po_finddevice)(char *name);

};

extern struct promops	promops;

/*
 * Memory description array.
 * Same as version 2 rom meminfo property.
 */
struct memarr {
	int	zero;
	u_int	addr;
	u_int	len;
};
int	prom_makememarr(struct memarr *, int max, int which);
#define	MEMARR_AVAILPHYS	0
#define	MEMARR_TOTALPHYS	1

struct idprom	*prom_getidprom(void);
void		prom_getether(int, u_char *);

void	prom_init(void);	/* To setup promops */

/* Utility routines */
int	prom_getprop(int, char *, size_t, int *, void *);
int	prom_getpropint(int node, char *name, int deflt);
char	*prom_getpropstring(int node, char *name);
char	*prom_getpropstringA(int node, char *name, char *, size_t);
void	prom_printf(const char *, ...);

int	prom_findroot(void);
int	prom_findnode(int, const char *);
int	prom_search(int, const char *);
int	prom_opennode(char *);
int	prom_node_has_property(int, const char *);
int	prom_getoptionsnode(void);
int	prom_getoption(const char *name, char *buf, int buflen);

#define	findroot()		prom_findroot()
#define	findnode(node,name)	prom_findnode(node,name)
#define	opennode(name)		prom_opennode(name)
#define	node_has_property(node,prop)	prom_node_has_property(node,prop)

void	prom_halt(void)		__attribute__((__noreturn__));
void	prom_boot(char *)	__attribute__((__noreturn__));

#if defined(MULTIPROCESSOR)
#define callrom() do {		\
	mp_pause_cpus();	\
	prom_abort();		\
	mp_resume_cpus();	\
} while (0)
#else
#define callrom()		prom_abort()
#endif

#define prom_version()		(promops.po_version)
#define prom_revision()		(promops.po_revision)
#define prom_stdin()		(promops.po_stdin)
#define prom_stdout()		(promops.po_stdout)
#define _prom_halt()		((*promops.po_halt)(/*void*/))
#define _prom_boot(a)		((*promops.po_reboot)(a))
#define prom_abort()		((*promops.po_abort)(/*void*/))
#define prom_interpret(a)	((*promops.po_interpret)(a))
#define prom_setcallback(f)	((*promops.po_setcallback)(f))
#define prom_setcontext(c,a,p)	((*promops.po_setcontext)(c,a,p))
#define prom_getbootpath()	((*promops.po_bootpath)(/*void*/))
#define prom_getbootfile()	((*promops.po_bootfile)(/*void*/))
#define prom_getbootargs()	((*promops.po_bootargs)(/*void*/))
#define prom_ticks()		((*promops.po_ticks)(/*void*/))


#define prom_open(name)		((*promops.po_open)(name))
#define prom_close(fd)		((*promops.po_close)(fd))
#define prom_instance_to_package(fd) \
				((*promops.po_instance_to_package)(fd))
#define prom_read(fd,b,n)	((*promops.po_read)(fd,b,n))
#define prom_write(fd,b,n)	((*promops.po_write)(fd,b,n))
#define prom_seek(fd,o)		((*promops.po_seek)(fd,o))
#define prom_getchar()		((*promops.po_getchar)(/*void*/))
#define prom_peekchar()		((*promops.po_peekchar)(/*void*/))
#define prom_putchar(c)		((*promops.po_putchar)(c))
#define prom_putstr(b,n)	((*promops.po_putstr)(b,n))

/* Node traversal */
#define prom_firstchild(node)	((*promops.po_firstchild)(node))
#define prom_nextsibling(node)	((*promops.po_nextsibling)(node))
#define prom_proplen(node,name)	((*promops.po_getproplen)(node, name))
#define _prom_getprop(node, name, buf, len) \
				((*promops.po_getprop)(node, name, buf, len))
#define prom_setprop(node, name, value, len) \
				((*promops.po_setprop)(node, name, value, len))
#define prom_nextprop(node,name)	((*promops.po_nextprop)(node, name))
#define prom_finddevice(name)	((*promops.po_finddevice)(name))

#define firstchild(node)	prom_firstchild(node)
#define nextsibling(node)	prom_nextsibling(node)
#define prom_getproplen(node,name)	prom_proplen(node, name)


/* MP stuff - not currently used */
#define prom_cpustart(m,a,c,pc)	((*promops.po_cpustart)(m,a,c,pc))
#define prom_cpustop(m)		((*promops.po_cpustop)(m))
#define prom_cpuidle(m)		((*promops.po_cpuidle)(m))
#define prom_cpuresume(m)	((*promops.po_cpuresume)(m))

extern void	*romp;		/* PROM-supplied argument (see locore) */

#endif /* _SPARC_PROMLIB_H_ */
