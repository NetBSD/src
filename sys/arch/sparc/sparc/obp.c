/*	$NetBSD: obp.c,v 1.2 1998/09/26 19:08:09 pk Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bsd_openprom.h>

/*
 * getprop() reads the named property data from a given node.
 * A buffer for the data may be passed in `*bufp'; if NULL, a
 * buffer is allocated. The argument `size' specifies the data
 * element size of the property data. This function checks that
 * the actual property data length is an integral multiple of
 * the element size.  The number of data elements read into the
 * buffer is returned into the integer pointed at by `nitem'.
 */
int
getprop(node, name, size, nitem, bufp)
	int	node;
	char	*name;
	int	size;
	int	*nitem;
	void	**bufp;
{
	struct	nodeops *no;
	void	*buf;
	int	len;

	no = promvec->pv_nodeops;
	len = no->no_proplen(node, name);
	if (len <= 0)
		return (ENOENT);

	if ((len % size) != 0)
		return (EINVAL);

	buf = *bufp;
	if (buf == NULL) {
		/* No storage provided, so we allocate some */
		buf = malloc(len, M_DEVBUF, M_NOWAIT);
		if (buf == NULL)
			return (ENOMEM);
	}

	no->no_getprop(node, name, buf);
	*bufp = buf;
	*nitem = len / size;
	return (0);
}

/*
 * Internal form of proplen().  Returns the property length.
 */
int
getproplen(node, name)
	int node;
	char *name;
{
	struct nodeops *no = promvec->pv_nodeops;

	return (no->no_proplen(node, name));
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
	static char stringbuf[32];

	return (getpropstringA(node, name, stringbuf));
}

/*
 * Alternative getpropstring(), where caller provides the buffer
 */
char *
getpropstringA(node, name, buffer)
	int node;
	char *name;
	char *buffer;
{
	int blen;

	if (getprop(node, name, 1, &blen, (void **)&buffer) != 0)
		blen = 0;

	buffer[blen] = '\0';	/* usually unnecessary */
	return (buffer);
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
	int intbuf, *ip = &intbuf;
	int len;

	if (getprop(node, name, sizeof(int), &len, (void **)&ip) != 0)
		return (deflt);

	return (*ip);
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

/*
 * search_prom() recursively searches a PROM tree for a given node
 */
int
search_prom(rootnode, name)
        int rootnode;
        char *name;
{
	int rtnnode;
	int node = rootnode;
	char buf[32];

	if (node == findroot() ||
	    !strcmp("hierarchical", getpropstringA(node, "device_type", buf)))
		node = firstchild(node);

	if (node == 0)
		panic("search_prom: null node");

	do {
		if (strcmp(getpropstringA(node, "name", buf), name) == 0)
			return (node);

		if (node_has_property(node,"device_type") &&
		    (strcmp(getpropstringA(node, "device_type", buf),
			     "hierarchical") == 0
		     || strcmp(getpropstringA(node, "name", buf), "iommu") == 0)
		    && (rtnnode = search_prom(node, name)) != 0)
			return (rtnnode);

	} while ((node = nextsibling(node)));

	return (0);
}


/*
 * Translate device path to node
 */
int
opennode(path)
	char *path;
{
	int fd;

	if (promvec->pv_romvec_vers < 2) {
		printf("WARNING: opennode not valid on sun4! %s\n", path);
		return (0);
	}
	fd = promvec->pv_v2devops.v2_open(path);
	if (fd == 0)
		return (0);

	return (promvec->pv_v2devops.v2_fd_phandle(fd));
}

/*
 * Determine whether a node has the given property.
 */
int
node_has_property(node, prop)
	int node;
	const char *prop;
{

	return ((*promvec->pv_nodeops->no_proplen)(node, (caddr_t)prop) != -1);
}

/*
 * Pass a string to the FORTH PROM to be interpreted.
 * (Note: may fail silently)
 */
void
rominterpret(s)
	char *s;
{

	if (promvec->pv_romvec_vers < 2)
		promvec->pv_fortheval.v0_eval(strlen(s), s);
	else
		promvec->pv_fortheval.v2_eval(s);
}

void
romhalt()
{

	*promvec->pv_synchook = NULL;

#if defined(SUN4M)
	if (0 && CPU_ISSUN4M) {
		extern void srmmu_restore_prom_ctx __P((void));
		srmmu_restore_prom_ctx();
	}
#endif

	promvec->pv_halt();
	panic("PROM exit failed");
}

void
romboot(str)
	char *str;
{

	*promvec->pv_synchook = NULL;
	promvec->pv_reboot(str);
	panic("PROM boot failed");
}

void
callrom()
{

	promvec->pv_abort();
}

/*
 * Start a cpu.
 */
void
rom_cpustart(node, ctxtab, ctx, pc)
	int node, ctx;
	struct openprom_addr *ctxtab;
	caddr_t pc;
{

	(*promvec->pv_v3cpustart)(node, ctxtab, ctx, pc);
}

void
rom_cpustop(node)
	int node;
{

	(*promvec->pv_v3cpustop)(node);
}

void
rom_cpuidle(node)
	int node;
{

	(*promvec->pv_v3cpuidle)(node);
}

void
rom_cpuresume(node)
	int node;
{

	(*promvec->pv_v3cpuresume)(node);
}
