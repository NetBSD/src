/*	$NetBSD: promlib.c,v 1.22 2003/08/27 15:59:54 mrg Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: promlib.c,v 1.22 2003/08/27 15:59:54 mrg Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sparc_arch.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>

#ifdef _STANDALONE
#include <lib/libsa/stand.h>
#define malloc(s,t,f)	alloc(s)
#else
#include <sys/systm.h>
#include <sys/malloc.h>
#endif /* _STANDALONE */

#include <machine/stdarg.h>
#include <machine/oldmon.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>

#include <lib/libkern/libkern.h>

#define obpvec ((struct promvec *)romp)

static void	notimplemented __P((void));
static void	obp_v0_fortheval __P((char *));
static void	obp_set_callback __P((void (*)__P((void))));
static int	obp_v0_read __P((int, void *, int));
static int	obp_v0_write __P((int, void *, int));
static int	obp_v2_getchar __P((void));
static int	obp_v2_peekchar __P((void));
static void	obp_v2_putchar __P((int));
static void	obp_v2_putstr __P((char *, int));
static int	obp_v2_seek __P((int, u_quad_t));
static char	*parse_bootfile __P((char *));
static char	*parse_bootargs __P((char *));
static char	*obp_v0_getbootpath __P((void));
static char	*obp_v0_getbootfile __P((void));
static char	*obp_v0_getbootargs __P((void));
static char	*obp_v2_getbootpath __P((void));
static char	*obp_v2_getbootfile __P((void));
static char	*obp_v2_getbootargs __P((void));
static int	obp_v2_finddevice __P((char *));
static int	obp_ticks __P((void));

static int	findchosen __P((void));
static char	*opf_getbootpath __P((void));
static char	*opf_getbootfile __P((void));
static char	*opf_getbootargs __P((void));
static int	opf_finddevice __P((char *));
static int	opf_instance_to_package __P((int));
static char	*opf_nextprop __P((int, char *));


/*
 * PROM entry points.
 * Note: only PROM functions we use ar represented here; add as required.
 */
struct promops promops = {
	-1,				/* version */
	-1,				/* revision */
	-1,				/* stdin handle */
	-1,				/* stdout handle */
	NULL,				/* bootargs */

	(void *)notimplemented,		/* bootpath */
	(void *)notimplemented,		/* bootargs */
	(void *)notimplemented,		/* bootfile */

	(void *)notimplemented,		/* getchar */
	(void *)notimplemented,		/* peekchar */
	(void *)notimplemented,		/* putchar */
	(void *)notimplemented,		/* putstr */
	(void *)notimplemented,		/* open */
	(void *)notimplemented,		/* close */
	(void *)notimplemented,		/* read */
	(void *)notimplemented,		/* write */
	(void *)notimplemented,		/* seek */

	(void *)notimplemented,		/* instance_to_package */

	(void *)notimplemented,		/* halt */
	(void *)notimplemented,		/* boot */
	(void *)notimplemented,		/* call */
	(void *)notimplemented,		/* interpret */
	(void *)notimplemented,		/* callback */
	(void *)notimplemented,		/* ticks */
	NULL,				/* ticker data */

	(void *)notimplemented,		/* setcontext */
	(void *)notimplemented,		/* cpustart */
	(void *)notimplemented,		/* cpustop */
	(void *)notimplemented,		/* cpuidle */
	(void *)notimplemented,		/* cpuresume */

	(void *)notimplemented,		/* firstchild */
	(void *)notimplemented,		/* nextsibling */

	(void *)notimplemented,		/* getproplen */
	(void *)notimplemented,		/* getprop */
	(void *)notimplemented,		/* setprop */
	(void *)notimplemented,		/* nextprop */
	(void *)notimplemented		/* finddevice */
};

static void
notimplemented()
{
	char str[64];
	int n;

	n = sprintf(str, "Operation not implemented on ROM version %d\r\n",
		    promops.po_version);

	/*
	 * Use PROM vector directly, in case we're called before prom_init().
	 */
#if defined(SUN4)
	if (CPU_ISSUN4) {
		struct om_vector *sun4pvec = (struct om_vector *)PROM_BASE;
		(*sun4pvec->fbWriteStr)(str, n);
	} else
#endif
	if (obpvec->pv_magic == OBP_MAGIC) {
		if (obpvec->pv_romvec_vers < 2) {
			(*obpvec->pv_putstr)(str, n);
		} else {
			int fd = *obpvec->pv_v2bootargs.v2_fd1;
			(*obpvec->pv_v2devops.v2_write)(fd, str, n);
		}
	} else {	/* assume OFW */
		static int stdout_node;
		if (stdout_node == 0) {
			int chosen = findchosen();
			OF_getprop(chosen, "stdout", &stdout_node, sizeof(int));
		}
		OF_write(stdout_node, str, n);
	}
}


/*
 * PROM_getprop() reads the named property data from a given node.
 * A buffer for the data may be passed in `*bufp'; if NULL, a
 * buffer is allocated. The argument `size' specifies the data
 * element size of the property data. This function checks that
 * the actual property data length is an integral multiple of
 * the element size.  The number of data elements read into the
 * buffer is returned into the integer pointed at by `nitem'.
 */

int
PROM_getprop(node, name, size, nitem, bufp)
	int	node;
	char	*name;
	int	size;
	int	*nitem;
	void	*bufp;
{
	void	*buf;
	int	len;

	len = PROM_getproplen(node, name);
	if (len <= 0)
		return (ENOENT);

	if ((len % size) != 0)
		return (EINVAL);

	buf = *(void **)bufp;
	if (buf == NULL) {
		/* No storage provided, so we allocate some */
		buf = malloc(len, M_DEVBUF, M_NOWAIT);
		if (buf == NULL)
			return (ENOMEM);
	} else {
		if (size * (*nitem) < len)
			return (ENOMEM);
	}

	_prom_getprop(node, name, buf, len);
	*(void **)bufp = buf;
	*nitem = len / size;
	return (0);
}

/*
 * Return a string property.  There is a (small) limit on the length;
 * the string is fetched into a static buffer which is overwritten on
 * subsequent calls.
 */
char *
PROM_getpropstring(node, name)
	int node;
	char *name;
{
	static char stringbuf[32];

	return (PROM_getpropstringA(node, name, stringbuf, sizeof stringbuf));
}

/*
 * Alternative PROM_getpropstring(), where caller provides the buffer
 */
char *
PROM_getpropstringA(node, name, buf, bufsize)
	int node;
	char *name;
	char *buf;
	size_t bufsize;
{
	int len = bufsize - 1;

	if (PROM_getprop(node, name, 1, &len, &buf) != 0)
		len = 0;

	buf[len] = '\0';	/* usually unnecessary */
	return (buf);
}

/*
 * Fetch an integer (or pointer) property.
 * The return value is the property, or the default if there was none.
 */
int
PROM_getpropint(node, name, deflt)
	int node;
	char *name;
	int deflt;
{
	int intbuf, *ip = &intbuf;
	int len = 1;

	if (PROM_getprop(node, name, sizeof(int), &len, &ip) != 0)
		return (deflt);

	return (*ip);
}

#if 0
/*
 * prom_search() recursively searches a PROM tree for a given node
 */
int
prom_search(rootnode, name)
	int rootnode;
	const char *name;
{
	int rtnnode;
	int node = rootnode;
	char buf[32];

#define GPSA(nm)	PROM_getpropstringA(node, nm, buf, sizeof buf)
	if (node == findroot() ||
	    !strcmp("hierarchical", GPSA("device type")))
		node = firstchild(node);

	if (node == 0)
		panic("prom_search: null node");

	do {
		if (strcmp(GPSA("name"), name) == 0)
			return (node);

		if ((strcmp(GPSA("device_type"), "hierarchical") == 0 ||
		    strcmp(GPSA("name"), "iommu") == 0)
		    && (rtnnode = prom_search(node, name)) != 0)
			return (rtnnode);

	} while ((node = nextsibling(node)) != NULL);

	return (0);
}
#endif

/*
 * Find the named device in the PROM device tree.
 * XXX - currently we discard any qualifiers attached to device component names
 */
int
obp_v2_finddevice(name)
	char *name;
{
	int node;
	char component[64];
	char c, *startp, *endp, *cp;
#define IS_SEP(c)	((c) == '/' || (c) == '@' || (c) == ':')

	if (name == NULL)
		return (-1);

	node = prom_findroot();

	for (startp = name; *startp != '\0'; ) {
		node = prom_firstchild(node);

		/*
		 * Identify next component in pathname
		 */
		while (*startp == '/')
			startp++;

		endp = startp;
		while ((c = *endp) != '\0' && !IS_SEP(c))
			endp++;

		/* Copy component */
		for (cp = component; startp != endp;)
			*cp++ = *startp++;

		/* Zero terminate this component */
		*cp = '\0';

		/* Advance `startp' over any non-slash separators */
		while ((c = *startp) != '\0' && c != '/')
			startp++;

		node = prom_findnode(node, component);
		if (node == 0)
			return (-1);
	}

	return (node);
}


/*
 * Translate device path to node
 */
int
prom_opennode(path)
	char *path;
{
	int fd;

	if (prom_version() < 2) {
		printf("WARNING: opennode not valid on PROM version %d\n",
			promops.po_version);
		return (0);
	}
	fd = prom_open(path);
	if (fd == 0)
		return (0);

	return (prom_instance_to_package(fd));
}

int
prom_findroot()
{
static	int rootnode;
	int node;

	if ((node = rootnode) == 0 && (node = prom_nextsibling(0)) == 0)
		panic("no PROM root device");
	rootnode = node;
	return (node);
}

/*
 * Given a `first child' node number, locate the node with the given name.
 * Return the node number, or 0 if not found.
 */
int
prom_findnode(first, name)
	int first;
	const char *name;
{
	int node;
	char buf[32];

	for (node = first; node != 0; node = prom_nextsibling(node)) {
		if (strcmp(PROM_getpropstringA(node, "name", buf, sizeof(buf)),
			   name) == 0)
			return (node);
	}
	return (0);
}

/*
 * Determine whether a node has the given property.
 */
int
prom_node_has_property(node, prop)
	int node;
	const char *prop;
{

	return (PROM_getproplen(node, (caddr_t)prop) != -1);
}


void
prom_halt()
{

	prom_setcallback(NULL);
	_prom_halt();
	panic("PROM exit failed");
}

void
prom_boot(str)
	char *str;
{

	prom_setcallback(NULL);
	_prom_boot(str);
	panic("PROM boot failed");
}


/*
 * print debug info to prom.
 * This is not safe, but then what do you expect?
 */
void
#ifdef __STDC__
prom_printf(const char *fmt, ...)
#else
prom_printf(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
static	char buf[256];
	int i, len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

#if _obp_not_cooked_
	(*promops.po_write)(promops.po_stdout, buf, len);
#endif

	for (i = 0; i < len; i++) {
		int c = buf[i];
		if (c == '\n')
			(*promops.po_putchar)('\r');
		(*promops.po_putchar)(c);
	}
}


/*
 * Pass a string to the FORTH PROM to be interpreted.
 * (Note: may fail silently)
 */
static void
obp_v0_fortheval(s)
	char *s;
{

	obpvec->pv_fortheval.v0_eval(strlen(s), s);
}

int
obp_v0_read(fd, buf, len)
	int fd;
	void *buf;
	int len;
{
	if (fd != prom_stdin())
		prom_printf("obp_v0_read: unimplemented read from %d\n", fd);
	return (-1);
}

int
obp_v0_write(fd, buf, len)
	int fd;
	void *buf;
	int len;
{
	if (fd != prom_stdout())
		prom_printf("obp_v0_write: unimplemented write on %d\n", fd);
	(*obpvec->pv_putstr)(buf, len);
	return (-1);
}

__inline__ void
obp_v2_putchar(c)
	int c;
{
	char c0;

	c0 = (c & 0x7f);
	(*promops.po_write)(promops.po_stdout, &c0, 1);
}

#if 0
void
obp_v2_putchar_cooked(c)
	int c;
{

	if (c == '\n')
		obp_v2_putchar('\r');
	obp_v2_putchar(c);
}
#endif

int
obp_v2_getchar()
{
	char c;
	int n;

	while ((n = (*promops.po_read)(promops.po_stdin, &c, 1)) != 1)
		/*void*/;
	if (c == '\r')
		c = '\n';
	return (c);
}

int
obp_v2_peekchar()
{
	char c;
	int n;

	n = (*promops.po_read)(promops.po_stdin, &c, 1);
	if (n < 0)
		return (-1);

	if (c == '\r')
		c = '\n';
	return (c);
}

int
obp_v2_seek(handle, offset)
	int handle;
	u_quad_t offset;
{
	u_int32_t hi, lo;

	lo = offset & ((u_int32_t)-1);
	hi = (offset >> 32) & ((u_int32_t)-1);
	(*obpvec->pv_v2devops.v2_seek)(handle, hi, lo);
	return (0);
}

/*
 * On SS1s (and also IPCs, SLCs), `promvec->pv_v0bootargs->ba_argv[1]'
 * contains the flags that were given after the boot command.  On SS2s
 * (and ELCs, IPXs, etc. and any sun4m class machine), `pv_v0bootargs'
 * is NULL but `*promvec->pv_v2bootargs.v2_bootargs' points to
 * "netbsd -s" or whatever.
 */
char *
obp_v0_getbootpath()
{
	struct v0bootargs *ba = promops.po_bootcookie;
	return (ba->ba_argv[0]);
}

char *
obp_v0_getbootargs()
{
	struct v0bootargs *ba = promops.po_bootcookie;
	return (ba->ba_argv[1]);
}

char *
obp_v0_getbootfile()
{
	struct v0bootargs *ba = promops.po_bootcookie;
	return (ba->ba_kernel);
}

char *
parse_bootargs(args)
	char *args;
{
	char *cp;

	for (cp = args; *cp != '\0'; cp++) {
		if (*cp == '-') {
			int c;
			/*
			 * Looks like options start here, but check this
			 * `-' is not part of the kernel name.
			 */
			if (cp == args)
				break;
			if ((c = *(cp-1)) == ' ' || c == '\t')
				break;
		}
	}
	return (cp);
}

char *
obp_v2_getbootpath()
{
	struct v2bootargs *ba = promops.po_bootcookie;
	return (*ba->v2_bootpath);
}

char *
obp_v2_getbootargs()
{
	struct v2bootargs *ba = promops.po_bootcookie;

	return (parse_bootargs(*ba->v2_bootargs));
}

char *
parse_bootfile(args)
	char *args;
{
static	char storage[128];
	char *cp, *dp;

	cp = args;
	dp = storage;
	while (*cp != 0 && *cp != ' ' && *cp != '\t') {
		if (dp >= storage + sizeof(storage) - 1) {
			prom_printf("v2_bootargs too long\n");
			return (NULL);
		}
		if (*cp == '-') {
			int c;
			/*
			 * If this `-' is most likely the start of boot
			 * options, we're done.
			 */
			if (cp == args)
				break;
			if ((c = *(cp-1)) == ' ' || c == '\t')
				break;
		}
		*dp++ = *cp++;
	}
	*dp = '\0';
	return (storage);
}

char *
obp_v2_getbootfile()
{
	struct v2bootargs *ba = promops.po_bootcookie;

	return (parse_bootfile(*ba->v2_bootargs));
}

void
obp_v2_putstr(str, len)
	char *str;
	int len;
{
	prom_write(prom_stdout(), str, len);
}

void
obp_set_callback(f)
	void (*f)__P((void));
{
	*obpvec->pv_synchook = f;
}

int
obp_ticks()
{

	return (*((int *)promops.po_tickdata));
}

static int
findchosen()
{
static	int chosennode;
	int node;

	if ((node = chosennode) == 0 && (node = OF_finddevice("/chosen")) == -1)
		panic("no CHOSEN node");

	chosennode = node;
	return (node);
}

static int
opf_finddevice(name)
	char *name;
{
	int phandle = OF_finddevice(name);
	if (phandle == -1)
		return (0);
	else
		return (phandle);
}

static int
opf_instance_to_package(ihandle)
	int ihandle;
{
	int phandle = OF_instance_to_package(ihandle);
	if (phandle == -1)
		return (0);
	else
		return (phandle);
}


static char *
opf_getbootpath()
{
	int node = findchosen();
	char *buf = NULL;
	int blen = 0;

	if (PROM_getprop(node, "bootpath", 1, &blen, &buf) != 0)
		return ("");

	return (buf);
}

static char *
opf_getbootargs()
{
	int node = findchosen();
	char *buf = NULL;
	int blen = 0;

	if (PROM_getprop(node, "bootargs", 1, &blen, &buf) != 0)
		return ("");

	return (parse_bootargs(buf));
}

static char *
opf_getbootfile()
{
	int node = findchosen();
	char *buf = NULL;
	int blen = 0;

	if (PROM_getprop(node, "bootargs", 1, &blen, &buf) != 0)
		return ("");

	return (parse_bootfile(buf));
}

static char *
opf_nextprop(node, prop)
	int node;
	char *prop;
{
#define OF_NEXTPROP_BUF_SIZE 32	/* specified by the standard */
	static char buf[OF_NEXTPROP_BUF_SIZE];
	OF_nextprop(node, prop, buf);
	return (buf);
}

/*
 * Retrieve physical memory information from the PROM.
 * If ap is NULL, return the required length of the array.
 */
int
prom_makememarr(ap, max, which)
	struct memarr *ap;
	int max, which;
{
	struct v0mlist *mp;
	int node, n;
	char *prop;

	if (which != MEMARR_AVAILPHYS && which != MEMARR_TOTALPHYS)
		panic("makememarr");

	/*
	 * `struct memarr' is in V2 memory property format.
	 * On previous ROM versions we must convert.
	 */
	switch (prom_version()) {
		struct promvec *promvec;
		struct om_vector *oldpvec;
	case PROM_OLDMON:
		oldpvec = (struct om_vector *)PROM_BASE;
		n = 1;
		if (ap != NULL) {
			ap[0].zero = 0;
			ap[0].addr = 0;
			ap[0].len = (which == MEMARR_AVAILPHYS)
				? *oldpvec->memoryAvail
				: *oldpvec->memorySize;
		}
		break;

	case PROM_OBP_V0:
		/*
		 * Version 0 PROMs use a linked list to describe these
		 * guys.
		 */
		promvec = romp;
		mp = (which == MEMARR_AVAILPHYS)
			? *promvec->pv_v0mem.v0_physavail
			: *promvec->pv_v0mem.v0_phystot;
		for (n = 0; mp != NULL; mp = mp->next, n++) {
			if (ap == NULL)
				continue;
			if (n >= max) {
				printf("makememarr: WARNING: lost some memory\n");
				break;
			}
			ap->zero = 0;
			ap->addr = (u_int)mp->addr;
			ap->len = mp->nbytes;
			ap++;
		}
		break;

	default:
		printf("makememarr: hope version %d PROM is like version 2\n",
			prom_version());
		/* FALLTHROUGH */

        case PROM_OBP_V3:
	case PROM_OBP_V2:
		/*
		 * Version 2 PROMs use a property array to describe them.
		 */

		/* Consider emulating `OF_finddevice' */
		node = findnode(firstchild(findroot()), "memory");
		goto case_common;

	case PROM_OPENFIRM:
		node = OF_finddevice("/memory");
		if (node == -1)
			node = 0;

	case_common:
		if (node == 0)
			panic("makememarr: cannot find \"memory\" node");

		prop = (which == MEMARR_AVAILPHYS) ? "available" : "reg";
		if (ap == NULL) {
			n = PROM_getproplen(node, prop);
		} else {
			n = max;
			if (PROM_getprop(node, prop, sizeof(struct memarr),
					&n, &ap) != 0)
				panic("makememarr: cannot get property");
		}
		break;
	}

	if (n <= 0)
		panic("makememarr: no memory found");
	/*
	 * Success!  (Hooray)
	 */
	return (n);
}


static struct idprom idprom;
#ifdef _STANDALONE
long hostid;
#endif

struct idprom *
prom_getidprom(void)
{
	int node, len;
	u_long h;
	u_char *src, *dst;

	if (idprom.id_format != 0)
		/* Already got it */
		return (&idprom);

	switch (prom_version()) {
	case PROM_OLDMON:
		len = sizeof(struct idprom);
		src = (char *)AC_IDPROM;
		dst = (char *)&idprom;
		do {
			*dst++ = lduba(src++, ASI_CONTROL);
		} while (--len > 0);
		break;

	/*
	 * Fetch the `idprom' property at the root node.
	 */
	case PROM_OBP_V0:
	case PROM_OBP_V2:
	case PROM_OPENFIRM:
	case PROM_OBP_V3:
		dst = (char *)&idprom;
		len = sizeof(struct idprom);
		node = prom_findroot();
		if (PROM_getprop(node, "idprom", 1, &len, &dst) != 0) {
			printf("`idprom' property cannot be read: "
				"cannot get ethernet address");
		}
		break;
	}

	/* Establish hostid */
	h =  idprom.id_machine << 24;
	h |= idprom.id_hostid[0] << 16;
	h |= idprom.id_hostid[1] << 8;
	h |= idprom.id_hostid[2];
	hostid = h;

	return (&idprom);
}

__strong_alias(myetheraddr,prom_getether);
void prom_getether(cp)
	u_char *cp;
{
	struct idprom *idp = prom_getidprom();

	cp[0] = idp->id_ether[0];
	cp[1] = idp->id_ether[1];
	cp[2] = idp->id_ether[2];
	cp[3] = idp->id_ether[3];
	cp[4] = idp->id_ether[4];
	cp[5] = idp->id_ether[5];
}

static void prom_init_oldmon __P((void));
static void prom_init_obp __P((void));
static void prom_init_opf __P((void));

static __inline__ void
prom_init_oldmon()
{
	struct om_vector *oldpvec = (struct om_vector *)PROM_BASE;
	extern void sparc_noop __P((void));

	promops.po_version = PROM_OLDMON;
	promops.po_revision = oldpvec->monId[0];	/*XXX*/

	promops.po_stdin = *oldpvec->inSource;
	promops.po_stdout = *oldpvec->outSink;

	promops.po_bootcookie = *oldpvec->bootParam; /* deref 1 lvl */
	promops.po_bootpath = obp_v0_getbootpath;
	promops.po_bootfile = obp_v0_getbootfile;
	promops.po_bootargs = obp_v0_getbootargs;

	promops.po_putchar = oldpvec->putChar;
	promops.po_getchar = oldpvec->getChar;
	promops.po_peekchar = oldpvec->mayGet;
	promops.po_putstr = oldpvec->fbWriteStr;
	promops.po_reboot = oldpvec->reBoot;
	promops.po_abort = oldpvec->abortEntry;
	promops.po_halt = oldpvec->exitToMon;
	promops.po_ticks = obp_ticks;
	promops.po_tickdata = oldpvec->nmiClock;
	promops.po_setcallback = (void *)sparc_noop;
	promops.po_setcontext = oldpvec->setcxsegmap;

#ifdef SUN4
#ifndef _STANDALONE
	if (oldpvec->romvecVersion >= 2) {
		extern void oldmon_w_cmd __P((u_long, char *));
		*oldpvec->vector_cmd = oldmon_w_cmd;
	}
#endif
#endif
}

static __inline__ void
prom_init_obp()
{
	struct nodeops *no;

	/*
	 * OBP v0, v2 & v3
	 */
	switch (obpvec->pv_romvec_vers) {
	case 0:
		promops.po_version = PROM_OBP_V0;
		break;
	case 2:
		promops.po_version = PROM_OBP_V2;
		break;
	case 3:
		promops.po_version = PROM_OBP_V3;
		break;
	default:
		obpvec->pv_halt();	/* What else? */
	}

	promops.po_revision = obpvec->pv_printrev;

	promops.po_halt = obpvec->pv_halt;
	promops.po_reboot = obpvec->pv_reboot;
	promops.po_abort = obpvec->pv_abort;
	promops.po_setcontext = obpvec->pv_setctxt;
	promops.po_setcallback = obp_set_callback;
	promops.po_ticks = obp_ticks;
	promops.po_tickdata = obpvec->pv_ticks;

	/*
	 * Remove indirection through `pv_nodeops' while we're here.
	 * Hopefully, the PROM has no need to change this pointer on the fly..
	 */
	no = obpvec->pv_nodeops;
	promops.po_firstchild = no->no_child;
	promops.po_nextsibling = no->no_nextnode;
	promops.po_getproplen = no->no_proplen;
	/* XXX - silently discard getprop's `len' argument */
	promops.po_getprop = (void *)no->no_getprop;
	promops.po_setprop = no->no_setprop;
	promops.po_nextprop = no->no_nextprop;

	/*
	 * Next, deal with prom vector differences between versions.
	 */
	switch (promops.po_version) {
	case PROM_OBP_V0:
		promops.po_stdin = *obpvec->pv_stdin;
		promops.po_stdout = *obpvec->pv_stdout;
		promops.po_bootcookie = *obpvec->pv_v0bootargs; /* deref 1 lvl */
		promops.po_bootpath = obp_v0_getbootpath;
		promops.po_bootfile = obp_v0_getbootfile;
		promops.po_bootargs = obp_v0_getbootargs;
		promops.po_putchar = obpvec->pv_putchar;
		promops.po_getchar = obpvec->pv_getchar;
		promops.po_peekchar = obpvec->pv_nbgetchar;
		promops.po_putstr = obpvec->pv_putstr;
		promops.po_open = obpvec->pv_v0devops.v0_open;
		promops.po_close = (void *)obpvec->pv_v0devops.v0_close;
		promops.po_read = obp_v0_read;
		promops.po_write = obp_v0_write;
		promops.po_interpret = obp_v0_fortheval;
		break;
	case PROM_OBP_V3:
		promops.po_cpustart = obpvec->pv_v3cpustart;
		promops.po_cpustop = obpvec->pv_v3cpustop;
		promops.po_cpuidle = obpvec->pv_v3cpuidle;
		promops.po_cpuresume = obpvec->pv_v3cpuresume;
		/*FALLTHROUGH*/
	case PROM_OBP_V2:
		/* Deref stdio handles one level */
		promops.po_stdin = *obpvec->pv_v2bootargs.v2_fd0;
		promops.po_stdout = *obpvec->pv_v2bootargs.v2_fd1;

		promops.po_bootcookie = &obpvec->pv_v2bootargs;
		promops.po_bootpath = obp_v2_getbootpath;
		promops.po_bootfile = obp_v2_getbootfile;
		promops.po_bootargs = obp_v2_getbootargs;

		promops.po_interpret = obpvec->pv_fortheval.v2_eval;

		promops.po_putchar = obp_v2_putchar;
		promops.po_getchar = obp_v2_getchar;
		promops.po_peekchar = obp_v2_peekchar;
		promops.po_putstr = obp_v2_putstr;
		promops.po_open = obpvec->pv_v2devops.v2_open;
		promops.po_close = (void *)obpvec->pv_v2devops.v2_close;
		promops.po_read = obpvec->pv_v2devops.v2_read;
		promops.po_write = obpvec->pv_v2devops.v2_write;
		promops.po_seek = obp_v2_seek;
		promops.po_instance_to_package = obpvec->pv_v2devops.v2_fd_phandle;
		promops.po_finddevice = obp_v2_finddevice;

#ifndef _STANDALONE
		prom_printf("OBP version %d, revision %d.%d (plugin rev %x)\n",
			obpvec->pv_romvec_vers,
			obpvec->pv_printrev >> 16, obpvec->pv_printrev & 0xffff,
			obpvec->pv_plugin_vers);
#endif
		break;
	}
}

static __inline__ void
prom_init_opf()
{
	int node;

	promops.po_version = PROM_OPENFIRM;

	/*
	 * OpenFirmware ops are mostly straightforward.
	 */
	promops.po_halt = OF_exit;
	promops.po_reboot = OF_boot;
	promops.po_abort = OF_enter;
	promops.po_interpret = OF_interpret;
	promops.po_setcallback = (void *)OF_set_callback;
	promops.po_ticks = OF_milliseconds;

	promops.po_bootpath = opf_getbootpath;
	promops.po_bootfile = opf_getbootfile;
	promops.po_bootargs = opf_getbootargs;

	promops.po_firstchild = OF_child;
	promops.po_nextsibling = OF_peer;
	promops.po_getproplen = OF_getproplen;
	promops.po_getprop = OF_getprop;
	promops.po_nextprop = opf_nextprop;
	promops.po_setprop = OF_setprop;

	/* We can re-use OBP v2 emulation */
	promops.po_putchar = obp_v2_putchar;
	promops.po_getchar = obp_v2_getchar;
	promops.po_peekchar = obp_v2_peekchar;
	promops.po_putstr = obp_v2_putstr;

	promops.po_open = OF_open;
	promops.po_close = OF_close;
	promops.po_read = OF_read;
	promops.po_write = OF_write;
	promops.po_seek = OF_seek;
	promops.po_instance_to_package = opf_instance_to_package;
	promops.po_finddevice = opf_finddevice;

	/* Retrieve and cache stdio handles */
	node = findchosen();
	OF_getprop(node, "stdin", &promops.po_stdin, sizeof(int));
	OF_getprop(node, "stdout", &promops.po_stdout, sizeof(int));
}

/*
 * Initialize our PROM operations vector.
 */
void
prom_init()
{
#ifdef _STANDALONE
	int node;
	char *cp;
#endif

	if (CPU_ISSUN4) {
		prom_init_oldmon();
	} else if (obpvec->pv_magic == OBP_MAGIC) {
		prom_init_obp();
	} else {
		/*
		 * Assume this is an Openfirm machine.
		 */
		prom_init_opf();
	}

#ifdef _STANDALONE
	/*
	 * Find out what type of machine we're running on.
	 *
	 * This process is actually started in srt0.S, which has discovered
	 * the minimal set of machine specific parameters for the 1st-level
	 * boot program (bootxx) to run. The page size has already been set
	 * and the CPU type is either CPU_SUN4, CPU_SUN4C or CPU_SUN4M.
	 */

	if (cputyp == CPU_SUN4 || cputyp == CPU_SUN4M)
		return;

	/*
	 * We have SUN4C, SUN4M or SUN4D.
	 * Use the PROM `compatible' property to determine which.
	 * Absence of the `compatible' property means `sun4c'.
	 */

	node = prom_findroot();
	cp = PROM_getpropstring(node, "compatible");
	if (*cp == '\0' || strcmp(cp, "sun4c") == 0)
		cputyp = CPU_SUN4C;
	else if (strcmp(cp, "sun4m") == 0)
		cputyp = CPU_SUN4M;
	else if (strcmp(cp, "sun4d") == 0)
		cputyp = CPU_SUN4D;
	else
		printf("Unknown CPU type (compatible=`%s')\n", cp);
#endif /* _STANDALONE */
}
