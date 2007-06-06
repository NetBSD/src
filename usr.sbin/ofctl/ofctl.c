/*	$NetBSD: ofctl.c,v 1.5 2007/06/06 23:43:56 rjs Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <err.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/queue.h>
#include <dev/ofw/openfirmio.h>

#include <prop/proplib.h>

static void oflist(int, const char *, int, void *, size_t);
static void ofprop(int);
static void ofgetprop(int, char *);
#if 0
static int isstrprint(const char *, size_t, int);
#endif

static int lflag;
static int pflag;

struct of_node {
	TAILQ_ENTRY(of_node) of_sibling;
	TAILQ_HEAD(,of_node) of_children;
	TAILQ_HEAD(,of_prop) of_properties;
	struct of_node *of_parent;
	struct of_prop *of_name;
	struct of_prop *of_device_type;
	struct of_prop *of_reg;
	int of_nodeid;
};

struct of_prop {
	TAILQ_ENTRY(of_prop) prop_sibling;
	char *prop_name;
	u_int8_t *prop_data;
	size_t prop_length;
	size_t prop_namelen;
};

struct of_node of_root;
unsigned long of_node_count;
unsigned long of_prop_count;
prop_dictionary_t of_proplib;

int OF_parent(int);
int OF_child(int);
int OF_peer(int);
int OF_finddevice(const char *);
int OF_getproplen(int, char *);
int OF_getprop(int, char *, void *, size_t);
int OF_nextprop(int, char *, void *);

struct of_prop *of_tree_getprop(int, char *);

static void
of_tree_mkprop(struct of_node *node, prop_dictionary_t propdict,
    prop_dictionary_keysym_t key)
{
	struct of_prop *prop;
	prop_data_t obj;
	const char *name;

	name = prop_dictionary_keysym_cstring_nocopy(key);
	obj = prop_dictionary_get_keysym(propdict, key);

	prop = malloc(sizeof(*prop) + strlen(name) + 1);
	if (prop == NULL)
		err(1, "malloc(%zu)", sizeof(*prop) + strlen(name) + 1);

	memset(prop, 0, sizeof(*prop));
	prop->prop_name = (char *) (prop + 1);
	prop->prop_namelen = strlen(name);
	memcpy(prop->prop_name, name, prop->prop_namelen+1);
	TAILQ_INSERT_TAIL(&node->of_properties, prop, prop_sibling);

	if (!strcmp(name, "name"))
		node->of_name = prop;
	else if (!strcmp(name, "device_type"))
		node->of_device_type = prop;
	else if (!strcmp(name, "reg"))
		node->of_reg = prop;

	of_prop_count++;

	prop->prop_length = prop_data_size(obj);
	if (prop->prop_length)
		prop->prop_data = prop_data_data(obj);
}

static struct of_node *
of_tree_mknode(struct of_node *parent)
{
	struct of_node *newnode;
	newnode = malloc(sizeof(*newnode));
	if (newnode == NULL)
		err(1, "malloc(%zu)", sizeof(*newnode));

	of_node_count++;

	memset(newnode, 0, sizeof(*newnode));
	TAILQ_INIT(&newnode->of_children);
	TAILQ_INIT(&newnode->of_properties);
	newnode->of_parent = parent;

	TAILQ_INSERT_TAIL(&parent->of_children, newnode, of_sibling);

	return newnode;
}

static void
of_tree_fill(prop_dictionary_t dict, struct of_node *node)
{
	prop_dictionary_t propdict;
	prop_array_t propkeys;
	prop_array_t children;
	unsigned int i, count;

	node->of_nodeid = prop_number_unsigned_integer_value(
	    prop_dictionary_get(dict, "node"));

	propdict = prop_dictionary_get(dict, "properties");
	propkeys = prop_dictionary_all_keys(propdict);
	count = prop_array_count(propkeys);

	for (i = 0; i < count; i++)
		of_tree_mkprop(node, propdict, prop_array_get(propkeys, i));

	children = prop_dictionary_get(dict, "children");
	if (children) {
		count = prop_array_count(children);

		for (i = 0; i < count; i++) {
			of_tree_fill(
			    prop_array_get(children, i),
			    of_tree_mknode(node));
		}
	}
}

static void
of_tree_init(prop_dictionary_t dict)
{
	/*
	 * Initialize the root node of the OFW tree.
	 */
	TAILQ_INIT(&of_root.of_children);
	TAILQ_INIT(&of_root.of_properties);

	of_tree_fill(dict, &of_root);
}

static prop_object_t
of_proplib_mkprop(int fd, int nodeid, char *name)
{
	struct ofiocdesc ofio;
	prop_object_t obj;

	ofio.of_nodeid = nodeid;
	ofio.of_name = name;
	ofio.of_namelen = strlen(name);
	ofio.of_buf = NULL;
	ofio.of_buflen = 32;

   again:
	if (ofio.of_buf != NULL)
		free(ofio.of_buf);
	ofio.of_buf = malloc(ofio.of_buflen);
	if (ofio.of_buf == NULL)
		err(1, "malloc(%d)", ofio.of_buflen);
	if (ioctl(fd, OFIOCGET, &ofio) < 0) {
		if (errno == ENOMEM) {
			ofio.of_buflen *= 2;
			goto again;
		}
		warn("OFIOCGET(%d, \"%s\")", fd, name);
		free(ofio.of_buf);
		return NULL;
	}
	obj = prop_data_create_data(ofio.of_buf, ofio.of_buflen);
	free(ofio.of_buf);
	return obj;
}

static prop_dictionary_t
of_proplib_tree_fill(int fd, int nodeid)
{
	int childid = nodeid;
	struct ofiocdesc ofio;
	char namebuf[33];
	char newnamebuf[33];
	prop_array_t children;
	prop_dictionary_t dict, propdict;
	prop_object_t obj;

	ofio.of_nodeid = nodeid;
	ofio.of_name = namebuf;
	ofio.of_namelen = 1;
	ofio.of_buf = newnamebuf;

	namebuf[0] = '\0';

	dict = prop_dictionary_create();
	prop_dictionary_set(dict, "node",
	    prop_number_create_unsigned_integer(nodeid));

	propdict = prop_dictionary_create();
	for (;;) {
		ofio.of_buflen = sizeof(newnamebuf);

		if (ioctl(fd, OFIOCNEXTPROP, &ofio) < 0) {
			if (errno == ENOENT)
				break;
			err(1, "OFIOCNEXTPROP(%d, %#x, \"%s\")", fd,
			    ofio.of_nodeid, ofio.of_name);
		}

		ofio.of_namelen = ofio.of_buflen;
		if (ofio.of_namelen == 0)
			break;
		newnamebuf[ofio.of_buflen] = '\0';
		strcpy(namebuf, newnamebuf);
		obj = of_proplib_mkprop(fd, nodeid, namebuf);
		if (obj)
			prop_dictionary_set(propdict, namebuf, obj);
	}
	prop_dictionary_set(dict, "properties", propdict);

	if (ioctl(fd, OFIOCGETCHILD, &childid) < 0)
		err(1, "OFIOCGETCHILD(%d, %#x)", fd, childid);

	children = NULL;
	while (childid != 0) {
		if (children == NULL)
			children = prop_array_create();
		prop_array_add(children, of_proplib_tree_fill(fd, childid));
		if (ioctl(fd, OFIOCGETNEXT, &childid) < 0)
			err(1, "OFIOCGETNEXT(%d, %#x)", fd, childid);
	}
	if (children != NULL) {
		prop_array_make_immutable(children);
		prop_dictionary_set(dict, "children", children);
	}

	return dict;
}

static prop_dictionary_t
of_proplib_init(const char *file)
{
	prop_dictionary_t dict;
	int rootid = 0;
	int fd;

	fd = open(file, O_RDONLY);
	if (fd < 0)
		err(1, "%s", file);

	if (ioctl(fd, OFIOCGETNEXT, &rootid) < 0)
		err(1, "OFIOCGETNEXT(%d, %#x)", fd, rootid);

	dict = of_proplib_tree_fill(fd, rootid);
	close(fd);
	return dict;
}

static struct of_node *
of_tree_walk(struct of_node *node,
	struct of_node *(*fn)(struct of_node *, const void *),
	const void *ctx)
{
	struct of_node *child, *match;

	if ((match = (*fn)(node, ctx)) != NULL)
		return match;

	TAILQ_FOREACH(child, &node->of_children, of_sibling) {
		if ((match = of_tree_walk(child, fn, ctx)) != NULL)
			return match;
	}
	return NULL;
}

static struct of_node *
of_match_by_nodeid(struct of_node *node, const void *ctx)
{
	return (node->of_nodeid == *(const int *) ctx) ? node : NULL;
}

static struct of_node *
of_match_by_parentid(struct of_node *node, const void *ctx)
{
	if (node->of_parent == NULL)
		return NULL;
	return (node->of_parent->of_nodeid == *(const int *) ctx) ? node : NULL;
}

int
OF_parent(int childid)
{
	struct of_node *child;

	if (childid == 0)
		return 0;

	child = of_tree_walk(&of_root, of_match_by_nodeid, &childid);
	if (child == NULL || child->of_parent == NULL)
		return 0;
	return child->of_parent->of_nodeid;
}

int
OF_child(int parentid)
{
	struct of_node *child;

	child = of_tree_walk(&of_root, of_match_by_parentid, &parentid);
	if (child == NULL)
		return 0;
	return child->of_nodeid;
}

int
OF_peer(int peerid)
{
	struct of_node *node, *match;

	if (peerid == 0)
		return of_root.of_nodeid;

	node = of_tree_walk(&of_root, of_match_by_nodeid, &peerid);
	if (node == NULL || node->of_parent == NULL)
		return 0;

	/*
	 * The peer should be our next sibling (if one exists).
	 */
	match = TAILQ_NEXT(node, of_sibling);
	return (match != NULL) ? match->of_nodeid : 0;
}

int
OF_finddevice(const char *name)
{
#if 0
	struct ofiocdesc ofio;
	
	ofio.of_nodeid = 0;
	ofio.of_name = argv[optind++];
	ofio.of_namelen = strlen(ofio.of_name);
	ofio.of_buf = NULL;
	ofio.of_buflen = 0;
	if (ioctl(of_fd, OFIOCFINDDEVICE, &ofio) < 0)
		err(1, "OFIOCFINDDEVICE(%d, \"%s\")", of_fd, ofio.of_name);
#endif
	return 0;
}

struct of_prop *
of_tree_getprop(int nodeid, char *name)
{
	struct of_node *node;
	struct of_prop *prop;

	if (nodeid == 0)
		return 0;

	node = of_tree_walk(&of_root, of_match_by_nodeid, &nodeid);
	if (node == NULL)
		return NULL;

	if (name[0] == '\0')
		return TAILQ_FIRST(&node->of_properties);

	if (!strcmp(name, "name"))
		return node->of_name;
	if (!strcmp(name, "device_type"))
		return node->of_device_type;
	if (!strcmp(name, "reg"))
		return node->of_reg;

	TAILQ_FOREACH(prop, &node->of_properties, prop_sibling) {
		if (!strcmp(name, prop->prop_name))
			break;
	}
	return prop;
}

int
OF_getproplen(int nodeid, char *name)
{
	struct of_prop *prop = of_tree_getprop(nodeid, name);
	return (prop != NULL) ? prop->prop_length : -1;
}

int
OF_getprop(int nodeid, char *name, void *buf, size_t len)
{
	struct of_prop *prop = of_tree_getprop(nodeid, name);
	if (prop == NULL)
		return -1;
	if (len > prop->prop_length)
		len = prop->prop_length;
	memcpy(buf, prop->prop_data, len);
	return len;
}

int
OF_nextprop(int nodeid, char *name, void *nextname)
{
	struct of_prop *prop = of_tree_getprop(nodeid, name);
	if (prop == NULL)
		return -1;
	if (name[0] != '\0') {
		prop = TAILQ_NEXT(prop, prop_sibling);
		if (prop == NULL)
			return -1;
	}
	strcpy(nextname, prop->prop_name);
	return strlen(prop->prop_name);
}

static u_int32_t
of_decode_int(const u_int8_t *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/*
 * Now we start the real program
 */

int
main(int argc, char **argv)
{
	u_long of_buf[256];
	char device_type[33];
	int phandle;
	int errflag = 0;
	int c;
	int len;
#if defined(__sparc__) || defined(__sparc64__)
	const char *file = "/dev/openprom";
#else
	const char *file = "/dev/openfirm";
#endif
	const char *propfilein = NULL;
	const char *propfileout = NULL;

	while ((c = getopt(argc, argv, "f:lpr:w:")) != EOF) {
		switch (c) {
		case 'l': lflag++; break;
		case 'p': pflag++; break;
		case 'f': file = optarg; break;
		case 'r': propfilein = optarg; break;
		case 'w': propfileout = optarg; break;
		default: errflag++; break;
		}
	}
	if (errflag)
		errx(1, "usage: ofctl [-pl] [-f file] [-r propfile] [-w propfile] [node...]\n");

	if (propfilein != NULL) {
		of_proplib = prop_dictionary_internalize_from_file(propfilein);
	} else {
		of_proplib = of_proplib_init(file);
	}

	if (propfileout)
		prop_dictionary_externalize_to_file(of_proplib, propfileout);

	of_tree_init(of_proplib);
	printf("[Caching %lu nodes and %lu properties]\n",
		of_node_count, of_prop_count);

	if (argc == optind) {
		phandle = OF_peer(0);
		device_type[0] = '\0';
		len = OF_getprop(phandle, "device_type", device_type,
		    sizeof(device_type));
		if (len <= 0)
			len = OF_getprop(phandle, "name", device_type,
			    sizeof(device_type));
		if (len >= 0)
			device_type[len] = '\0';
		oflist(phandle, device_type, 0, of_buf, sizeof(of_buf));
	} else {
#if 0
		pandle = OF_finddevice(argv[optind++]);

		if (argc == optind) {
			if (lflag)
				oflist(phandle, 0, of_buf, sizeof(of_buf));
			else
				ofprop(phandle);
		} else {
			for (; optind < argc; optind++) {
				ofgetprop(phandle, argv[optind]);
			}
		}
#else
		printf("%s: OF_finddevice not yet implemented\n", argv[optind]);
#endif
	}
	exit(0);
}

static size_t
ofname(int node, char *buf, size_t buflen)
{
	u_int8_t address_cells_buf[4];
	u_int8_t reg_buf[4096];
	char name[33];
	char device_type[33];
	size_t off = 0;
	int parent = OF_parent(node);
	int reglen;
	int reg[sizeof(reg_buf)/sizeof(int)];
	int address_cells;
	int len;

	len = OF_getprop(node, "name", name, sizeof(name));
	assert(len > 0);
	off += snprintf(buf + off, buflen - off, "/%s", name);

	reglen = OF_getprop(node, "reg", reg_buf, sizeof(reg_buf));
	if (reglen <= 0)
		return off;

	len = OF_getprop(parent, "device_type",
	    device_type, sizeof(device_type));
	if (len <= 0)
		len = OF_getprop(parent, "name",
		    device_type, sizeof(device_type));
	device_type[len] = '\0';

	for (;;) {
		len = OF_getprop(parent, "#address-cells",
		    address_cells_buf, sizeof(address_cells_buf));
		if (len >= 0) {
			assert(len == 4);
			break;
		}
		parent = OF_parent(parent);
		if (parent == 0)
			break;
	}

	if (parent == 0) {

		parent = OF_parent(node);

		for (;;) {
			len = OF_getprop(parent, "#size-cells",
			    address_cells_buf, sizeof(address_cells_buf));
			if (len >= 0) {
				assert(len == 4);
				break;
			}
			parent = OF_parent(parent);
			if (parent == 0)
				break;
		}
		/* no #size-cells */
		len = 0;
	}

	if (len == 0) {
		/* looks like we're on an OBP2 system */
		if (reglen > 12)
			return off;
		off += snprintf(buf + off, buflen - off, "@");
		memcpy(reg, reg_buf, 8);
		off += snprintf(buf + off, buflen - off, "%x,%x", reg[0],
		    reg[1]);
		return off;
	}

	off += snprintf(buf + off, buflen - off, "@");
	address_cells = of_decode_int(address_cells_buf);
	for (len = 0; len < address_cells; len ++)
		reg[len] = of_decode_int(&reg_buf[len * 4]);
	
	if (!strcmp(device_type,"pci")) {
		off += snprintf(buf + off, buflen - off,
		    "%x", (reg[0] >> 11) & 31);
		if (reg[0] & 0x700)
			off += snprintf(buf + off, buflen - off,
			    ",%x", (reg[0] >> 8) & 7);
	} else if (!strcmp(device_type,"upa")) {
		off += snprintf(buf + off, buflen - off,
		    "%x", (reg[0] >> 4) & 63);
		for (len = 1; len < address_cells; len++)
			off += snprintf(buf + off, buflen - off,
			    ",%x", reg[len]);
#if !defined(__sparc__) && !defined(__sparc64__)
	} else if (!strcmp(device_type,"isa")) {
#endif
	} else {
		off += snprintf(buf + off, buflen - off, "%x", reg[0]);
		for (len = 1; len < address_cells; len++)
			off += snprintf(buf + off, buflen - off,
			    ",%x", reg[len]);
	}
	return off;
}

static size_t
offullname2(int node, char *buf, size_t len)
{
	size_t off;
	int parent = OF_parent(node);
	if (parent == 0)
		return 0;

	off = offullname2(parent, buf, len);
	off += ofname(node, buf + off, len - off);
	return off;
}

static size_t
offullname(int node, char *buf, size_t len)
{
	if (node == OF_peer(0)) {
		size_t off = snprintf(buf, len, "/");
		off += OF_getprop(node, "name", buf + off, len - off);
		return off;
	}
	return offullname2(node, buf, len);
}

static void
oflist(int node, const char *parent_device_type, int depth,
	void *of_buf, size_t of_buflen)
{
	int len;
	while (node != 0) {
		int child;
		if (pflag == 0) {
			len = ofname(node, of_buf, of_buflen-1);
			printf("%08x: %*s%s", node, depth * 2, "",
			    (char *) of_buf);
		} else {
			len = offullname(node, of_buf, of_buflen-1);
			printf("%08x: %s", node, (char *) of_buf);
		}
		putchar('\n');
		if (pflag) {
			putchar('\n');
			ofprop(node);
			puts("\n----------------------------------------"
			    "----------------------------------------\n\n");
		}
		child = OF_child(node);
		if (child != -1 && child != 0) {
			char device_type[33];
			len = OF_getprop(node, "device_type",
			    device_type, sizeof(device_type));
			if (len <= 0)
				len = OF_getprop(node, "name",
				    device_type, sizeof(device_type));
			if (len >= 0)
				device_type[len] = '\0';
			depth++;
			oflist(child, device_type, depth, of_buf, of_buflen);
			depth--;
		}
		if (depth == 0)
			break;
		node = OF_peer(node);
	}
}

static void
print_line(const u_int8_t *buf, size_t off, size_t len)
{
	if (len - off > 16)
		len = off + 16;

	for (; off < ((len + 15) & ~15); off++) {
		if (off > 0) {
			if ((off & 15) == 0)
				printf("%12s%04lx:%7s", "",
				    (unsigned long int) off, "");
			else if ((off & 3) == 0)
				putchar(' ');
		}
		if (off < len)
			printf("%02x", buf[off]);
#if 0
		else if (off >= ((len + 3) & ~3))
			printf("  ");
#endif
		else
			printf("..");
	}
}

static void
default_format(int node, const u_int8_t *buf, size_t len)
{
	size_t off = 0;
	while (off < len) {
		size_t end;
		print_line(buf, off, len);
		printf("   ");	/* 24 + 32 + 3 = 59, so +3 makes 62 */
		end = len;
		if (end > off + 16)
			end = off + 16;
		for (; off < end; off++) {
			char ch = buf[off];
			if (isascii(ch) &&
			    (isalnum((int)ch) || ispunct((int)ch) || ch == ' '))
				putchar(ch);
			else
				putchar('.');
		}
		putchar('\n');
	}
}

static void
reg_format(int node, const u_int8_t *buf, size_t len)
{
	/* parent = OF_parent(node); */
	default_format(node, buf, len);
}

static void
frequency_format(int node, const u_int8_t *buf, size_t len)
{
	if (len == 4) {
		u_int32_t freq = of_decode_int(buf);
		u_int32_t divisor, whole, frac;
		const char *units = "";
		print_line(buf, 0, len);
		for (divisor = 1000000000; divisor > 1; divisor /= 1000) {
			if (freq >= divisor)
				break;
		}
		whole = freq / divisor;
		if (divisor == 1)
			frac = 0;
		else
			frac = (freq / (divisor / 1000)) % 1000;
			
		switch (divisor) {
		case 1000000000: units = "GHz"; break;
		case    1000000: units = "MHz"; break;
		case       1000: units = "KHz"; break;
		case          1: units =  "Hz"; break;
		}
		if (frac > 0)
			printf("   %u.%03u%s\n", whole, frac, units);
		else
			printf("   %u%s\n", whole, units);
	} else
		default_format(node, buf, len);
}

static void
size_format(int node, const u_int8_t *buf, size_t len)
{
	if (len == 4) {
		u_int32_t freq = of_decode_int(buf);
		u_int32_t divisor, whole, frac;
		const char *units = "";
		print_line(buf, 0, len);
		for (divisor = 0x40000000; divisor > 1; divisor >>= 10) {
			if (freq >= divisor)
				break;
		}
		whole = freq / divisor;
		if (divisor == 1)
			frac = 0;
		else
			frac = (freq / (divisor >> 10)) & 1023;
			
		switch (divisor) {
		case 0x40000000: units = "G"; break;
		case   0x100000: units = "M"; break;
		case      0x400: units = "K"; break;
		case          1: units =  ""; break;
		}
		if (frac > 0)
			printf("   %3u.%03u%s\n", whole, frac, units);
		else
			printf("   %3u%s\n", whole, units);
	} else
		default_format(node, buf, len);
}

static void
string_format(int node, const u_int8_t *buf, size_t len)
{
	size_t off = 0;
	int first_line = 1;
	while (off < len) {
		size_t string_len = 0;
		int leading = 1;
		for (; off + string_len < len; string_len++) {
			if (buf[off+string_len] == '\0') {
				string_len++;
				break;
			}
		}
		while (string_len > 0) {
			size_t line_len = string_len;
			if (line_len > 16)
				line_len = 16;
			if (!first_line)
				printf("%12s%04lx:%7s", "",
				    (unsigned long int) off, "");
			print_line(buf + off, 0, line_len);
			printf("   ");
			if (leading)
				putchar('"');
			first_line = 0;
			leading = 0;
			string_len -= line_len;
			for (; line_len > 0; line_len--, off++) {
				if (buf[off] != '\0')
					putchar(buf[off]);
			}
			if (string_len == 0)
				putchar('"');
			putchar('\n');
		}
	}
}

static const struct {
	const char *prop_name;
	void (*prop_format)(int, const u_int8_t *, size_t);
} formatters[] = {
	{ "reg", reg_format },
#if 0
	{ "assigned-addresses", assigned_addresses_format },
	{ "ranges", ranges_format },
	{ "interrupt-map", interrup_map_format },
	{ "interrupt", interrupt_format },
#endif
	{ "model", string_format },
	{ "name", string_format },
	{ "device_type", string_format },
	{ "compatible", string_format },
	{ "*frequency", frequency_format },
	{ "*-size", size_format },
	{ "*-cells", size_format },
	{ "*-entries", size_format },
	{ "*-associativity", size_format },
	{ NULL, default_format }
};

static void
ofgetprop(int node, char *name)
{
	u_int8_t of_buf[4097];
	int len;
	int i;

	len = OF_getprop(node, name, of_buf, sizeof(of_buf) - 1);
	if (len < 0)
		return;
	of_buf[len] = '\0';
	printf("%-24s", name);
	if (len == 0) {
		putchar('\n');
		return;
	}
	if (strlen(name) >= 24)
		printf("\n%24s", "");

	for (i = 0; formatters[i].prop_name != NULL; i++) {
		if (formatters[i].prop_name[0] == '*') {
			if (strstr(name, &formatters[i].prop_name[1]) != NULL) {
				(*formatters[i].prop_format)(node, of_buf, len);
				return;
			}
			continue;
		}
		if (strcmp(name, formatters[i].prop_name) == 0) {
			(*formatters[i].prop_format)(node, of_buf, len);
			return;
		}
	}
	(*formatters[i].prop_format)(node, of_buf, len);
}

static void
ofprop(int node)
{
	char namebuf[33];
	char newnamebuf[33];
	int len;

	namebuf[0] = '\0';

	for (;;) {
		len = OF_nextprop(node, namebuf, newnamebuf);
		if (len <= 0)
			break;

		newnamebuf[len] = '\0';
		strcpy(namebuf, newnamebuf);
		ofgetprop(node, newnamebuf);
	}
}
#if 0
static int
isstrprint(const char *str, size_t len, int ignorenulls)
{
	if (*str == '\0')
		return 0;
	for (; len-- > 0; str++) {
		if (*str == '\0' && len > 0 && str[1] == '\0')
			return 0;
		if (len == 0 && *str == '\0')
			return 1;
		if (ignorenulls) {
			if (*str == '\0')
				continue;
			if (isalnum(*str) || ispunct(*str) || *str == ' ')
				continue;
			return 0;
		}
		if (!isprint(*str))
			return 0;
	}
	return 1;
}
#endif
