/*	$NetBSD: promdev.c,v 1.9 1995/02/22 08:18:20 mycroft Exp $ */

/*
 * Copyright (c) 1993 Paul Kranenburg
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>

#include "defs.h"

struct promvec	*promvec;

int	promopen __P((struct open_file *, ...));
int	promclose __P((struct open_file *));
int	promioctl __P((struct open_file *, u_long, void *));
int	promstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));
char	*dev_type __P((int, char *));
int	getprop __P((int, char *, void *, int));
char	*getpropstring __P((int, char *));
int	getpropint __P((int, char *, int));
int	findroot __P((void));
int	findnode __P((int, char *));
int	firstchild __P((int));
int	nextsibling __P((int));

struct devsw devsw[] = {
	{ "prom", promstrategy, promopen, promclose, promioctl },
};

int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));

struct promdata {
	int	fd;
	int	devtype;
#define BLOCK	1
#define NET	2
#define BYTE	3
};

int
devopen(f, fname, file)
	struct open_file *f;
	char *fname;
	char **file;
{
	int	error, fd;
	char	*cp, *path, *dev;
	struct	promdata *pdp;

	if (promvec->pv_romvec_vers >= 2) {
		path = *promvec->pv_v2bootargs.v2_bootpath;
		fd = (*promvec->pv_v2devops.v2_open)(path);
		cp = path + strlen(path);
		while (cp >= path)
			if (*--cp == '/') {
				++cp;
				break;
			}
		dev = cp;
	} else {
		static char pathbuf[100];

		path = pathbuf;
		cp = (*promvec->pv_v0bootargs)->ba_argv[0];
		while (*cp) {
			*path++ = *cp;
			if (*cp++ == ')')
				break;
		}
		*path = '\0';
		dev = path = pathbuf;
		fd = (*promvec->pv_v0devops.v0_open)(path);
	}

	if (fd == 0) {
		printf("Can't open device `%s'\n", dev);
		return ENXIO;
	}

	pdp = (struct promdata *)alloc(sizeof *pdp);
	pdp->fd = fd;

	cp = dev_type(findroot(), dev);
	if (cp == NULL)
		printf("%s: bummer\n", dev);
	else if (strcmp(cp, "block") == 0)
		pdp->devtype = BLOCK;
	else if (strcmp(cp, "network") == 0)
		pdp->devtype = NET;
	else if (strcmp(cp, "byte") == 0)
		pdp->devtype = BYTE;
#ifdef DEBUG
	if (cp) printf("Boot device type: %s\n", cp);
#endif
	f->f_dev = devsw;
	f->f_devdata = (void *)pdp;
	*file = fname;
	return 0;
}

int
promstrategy(devdata, flag, dblk, size, buf, rsize)
	void	*devdata;
	int	flag;
	daddr_t	dblk;
	u_int	size;
	char	*buf;
	u_int	*rsize;
{
	int	error = 0;
	struct	promdata *pdp = (struct promdata *)devdata;
	int	fd = pdp->fd;

#ifdef DEBUG
	printf("promstrategy: size=%d dblk=%d\n", size, dblk);
#endif

	if (promvec->pv_romvec_vers >= 2) {
		if (pdp->devtype == BLOCK)
			(*promvec->pv_v2devops.v2_seek)(fd, 0, dbtob(dblk));

		*rsize = (*((flag == F_READ)
				? (u_int (*)())promvec->pv_v2devops.v2_read
				: (u_int (*)())promvec->pv_v2devops.v2_write
			 ))(fd, buf, size);
	} else {
		int n = (*((flag == F_READ)
				? (u_int (*)())promvec->pv_v0devops.v0_rbdev
				: (u_int (*)())promvec->pv_v0devops.v0_wbdev
			))(fd, btodb(size), dblk, buf);
		*rsize = dbtob(n);
	}

#ifdef DEBUG
	printf("rsize = %x\n", *rsize);
#endif
	return error;
}

int
promopen(f)
	struct open_file *f;
{
#ifdef DEBUG
	printf("promopen:\n");
#endif
	return 0;
}

int
promclose(f)
	struct open_file *f;
{
	return EIO;
}

int
promioctl(f, cmd, data)
	struct open_file *f;
	u_long cmd;
	void *data;
{
	return EIO;
}

getchar()
{
	char c;
	int n;
 
	if (promvec->pv_romvec_vers > 2)
		while ((n = (*promvec->pv_v2devops.v2_read)
			(*promvec->pv_v2bootargs.v2_fd0, (caddr_t)&c, 1)) != 1);
	else
		c = (*promvec->pv_getchar)();

	if (c == '\r')
		c = '\n';
	return (c);
}
 
peekchar()
{
	register int c;
 
	c = (*promvec->pv_nbgetchar)();
	if (c == '\r')
		c = '\n';
	return (c);
}

static void
pv_putchar(c)
	int c;
{
	char c0 = c;
	if (promvec->pv_romvec_vers > 2)
		(*promvec->pv_v2devops.v2_write)
			(*promvec->pv_v2bootargs.v2_fd1, &c0, 1);
	else
		(*promvec->pv_putchar)(c);
}

void
putchar(c)
	int c;
{
 
	if (c == '\n')
		pv_putchar('\r');
	pv_putchar(c);

#if 0
	if (c == '\n')
		(*promvec->pv_putchar)('\r');
	(*promvec->pv_putchar)(c);
#endif
}

char *
dev_type(node, dev)
	int	node;
	char	*dev;
{
	char	*name, *type;
	register int child;

	for (; node; node = nextsibling(node)) {
		name = getpropstring(node, "name");
		if (strncmp(dev, name, strlen(name)) == 0)
			return getpropstring(node, "device_type");

		child = firstchild(node);
		if (child)
			if (type = dev_type(child, dev))
				return type;
	}
	return NULL;
}

/*
 * PROM nodes & property routines (from <sparc/autoconf.c>).
 */

static int rootnode;

int
findroot()
{
	register int node;

	if ((node = rootnode) == 0 && (node = nextsibling(0)) == 0)
		printf("no PROM root device");
	rootnode = node;
	return (node);
}

#if 0
int
shownodes(first)
{
	register int node, child;
	char name[32], type[32];
static	int indent;

	for (node = first; node; node = nextsibling(node)) {
		int i = indent;
		strcpy(name, getpropstring(node, "name"));
		strcpy(type, getpropstring(node, "device_type"));
		while (i--) putchar(' ');
		printf("node(%x) name `%s', type `%s'\n", node, name, type);
		child = firstchild(node);
		if (child) {
			indent += 4;
			shownodes(child);
			indent -= 4;
		}
	}
}
#endif

/*
 * Given a `first child' node number, locate the node with the given name.
 * Return the node number, or 0 if not found.
 */
int
findnode(first, name)
	int first;
	register char *name;
{
	register int node;

	for (node = first; node; node = nextsibling(node))
		if (strcmp(getpropstring(node, "name"), name) == 0)
			return (node);
	return (0);
}

/*
 * Internal form of getprop().  Returns the actual length.
 */
int
getprop(node, name, buf, bufsiz)
	int node;
	char *name;
	void *buf;
	register int bufsiz;
{
	register struct nodeops *no;
	register int len;

	no = promvec->pv_nodeops;
	len = no->no_proplen(node, name);
	if (len > bufsiz) {
		printf("node %x property %s length %d > %d\n",
		    node, name, len, bufsiz);
		return (0);
	}
	no->no_getprop(node, name, buf);
	return (len);
}

/*
 * Return a string property.  There is a (small) limit on the length;
 * the string is fetched into a static buffer which is overwritten on
 * subsequent calls.
 */
char *
getpropstring(node, name)
	int node;
	char *name;
{
	register int len;
	static char stringbuf[32];

	len = getprop(node, name, (void *)stringbuf, sizeof stringbuf - 1);
	stringbuf[len] = '\0';	/* usually unnecessary */
	return (stringbuf);
}

/*
 * Fetch an integer (or pointer) property.
 * The return value is the property, or the default if there was none.
 */
int
getpropint(node, name, deflt)
	int node;
	char *name;
	int deflt;
{
	register int len;
	char intbuf[16];

	len = getprop(node, name, (void *)intbuf, sizeof intbuf);
	if (len != 4)
		return (deflt);
	return (*(int *)intbuf);
}

int
firstchild(node)
	int node;
{

	return (promvec->pv_nodeops->no_child(node));
}

int
nextsibling(node)
	int node;
{

	return (promvec->pv_nodeops->no_nextnode(node));
}
