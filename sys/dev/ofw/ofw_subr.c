/*	$NetBSD: ofw_subr.c,v 1.60 2022/01/22 11:49:18 thorpej Exp $	*/

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

/*
 * Copyright 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_subr.c,v 1.60 2022/01/22 11:49:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <sys/device_calls.h>

#include <dev/ofw/openfirm.h>

#define	OFW_MAX_STACK_BUF_SIZE	256
#define	OFW_PATH_BUF_SIZE	512

/*
 * OpenFirmware device handle support.
 */

static device_call_t
of_devhandle_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	__link_set_decl(of_device_calls, struct device_call_descriptor);
	struct device_call_descriptor * const *desc;

	__link_set_foreach(desc, of_device_calls) {
		if (strcmp((*desc)->name, name) == 0) {
			return (*desc)->call;
		}
	}
	return NULL;
}

static const struct devhandle_impl of_devhandle_impl = {
	.type = DEVHANDLE_TYPE_OF,
	.lookup_device_call = of_devhandle_lookup_device_call,
};

devhandle_t
devhandle_from_of(devhandle_t super_handle, int phandle)
{
	devhandle_type_t super_type = devhandle_type(super_handle);
	devhandle_t handle = { 0 };

	if (super_type == DEVHANDLE_TYPE_OF) {
		handle.impl = super_handle.impl;
	} else {
		KASSERT(super_type == DEVHANDLE_TYPE_INVALID);
		handle.impl = &of_devhandle_impl;
	}
	handle.integer = phandle;

	return handle;
}

int
devhandle_to_of(devhandle_t const handle)
{
	KASSERT(devhandle_type(handle) == DEVHANDLE_TYPE_OF);

	return handle.integer;
}

static int
of_device_enumerate_children(device_t dev, devhandle_t call_handle, void *v)
{
	struct device_enumerate_children_args *args = v;
	int phandle = devhandle_to_of(call_handle);
	int child;

	for (child = OF_child(phandle); child != 0; child = OF_peer(child)) {
		if (!args->callback(dev, devhandle_from_of(call_handle, child),
				    args->callback_arg)) {
			break;
		}
	}

	return 0;
}
OF_DEVICE_CALL_REGISTER(DEVICE_ENUMERATE_CHILDREN_STR,
			of_device_enumerate_children)

/*
 * int of_decode_int(p)
 *
 * This routine converts OFW encoded-int datums
 * into the integer format of the host machine.
 *
 * It is primarily used to convert integer properties
 * returned by the OF_getprop routine.
 *
 * Arguments:
 *	p		pointer to unsigned char array which is an
 *			OFW-encoded integer.
 *
 * Return Value:
 *	Decoded integer value of argument p.
 *
 * Side Effects:
 *	None.
 */
int
of_decode_int(const unsigned char *p)
{
	unsigned int i = *p++ << 8;
	i = (i + *p++) << 8;
	i = (i + *p++) << 8;
	return (i + *p);
}

/*
 * int of_compatible(phandle, strings)
 *
 * This routine checks an OFW node's "compatible" entry to see if
 * it matches any of the provided strings.
 *
 * of_compatible_match() is the preferred way to perform driver
 * compatibility match.  However, this routine that deals with
 * only strings is useful in some situations and is provided for
 * convenience.
 *
 * Arguments:
 *	phandle		OFW phandle of device to be checked for
 *			compatibility.
 *	strings		Array of containing expected "compatibility"
 *			property values, presence of any of which
 *			indicates compatibility.
 *
 * Return Value:
 *	0 if none of the strings are found in phandle's "compatibility"
 *	property, or the reverse index of the matching string in the
 *	phandle's "compatibility" property plus 1.
 *
 * Side Effects:
 *	None.
 */
int
of_compatible(int phandle, const char * const *strings)
{
	char *prop, propbuf[OFW_MAX_STACK_BUF_SIZE];
	const char *cp;
	int proplen, match = 0;

	proplen = OF_getproplen(phandle, "compatible");
	if (proplen <= 0) {
		return 0;
	}

	prop = kmem_tmpbuf_alloc(proplen, propbuf, sizeof(propbuf), KM_SLEEP);

	if (OF_getprop(phandle, "compatible", prop, proplen) != proplen) {
		goto out;
	}

	for (; (cp = *strings) != NULL; strings++) {
		if ((match = strlist_match(prop, proplen, cp)) != 0) {
			break;
		}
	}

 out:
	kmem_tmpbuf_free(prop, proplen, propbuf);
	return match;
}

/*
 * int of_compatible_match(phandle, compat_data)
 *
 * This routine searches an array of device_compatible_entry structures
 * for a matching "compatible" entry matching the supplied OFW node,
 * and returns a weighted match value corresponding to which string
 * from the "compatible" property was matched, which more weight given
 * to the first string than the last.
 *
 * It should be used when determining whether a driver can drive
 * a particular device.
 *
 * Arguments:
 *	phandle		OFW phandle of device to be checked for
 *			compatibility.
 *	compat_data	Array of possible compat entry strings and
 *			associated metadata. The last entry in the
 *			list should have a "compat" of NULL to terminate
 *			the list.
 *
 * Return Value:
 *	0 if none of the strings are found in phandle's "compatibility"
 *	property, or a positive number based on the reverse index of the
 *	matching string in the phandle's "compatibility" property, plus 1.
 *
 * Side Effects:
 *	None.
 */
int
of_compatible_match(int phandle,
    const struct device_compatible_entry *compat_data)
{
	char *prop, propbuf[OFW_MAX_STACK_BUF_SIZE];
	int proplen, match = 0;

	proplen = OF_getproplen(phandle, "compatible");
	if (proplen <= 0) {
		return 0;
	}

	prop = kmem_tmpbuf_alloc(proplen, propbuf, sizeof(propbuf), KM_SLEEP);

	if (OF_getprop(phandle, "compatible", prop, proplen) != proplen) {
		goto out;
	}

	match = device_compatible_match_strlist(prop, proplen, compat_data);

 out:
	kmem_tmpbuf_free(prop, proplen, propbuf);
	return match;
}

/*
 * const struct device_compatible_entry *of_compatible_lookup(phandle,
 *							      compat_data)
 *
 * This routine searches an array of device_compatible_entry structures
 * for a "compatible" entry matching the supplied OFW node.
 *
 * Arguments:
 *	phandle		OFW phandle of device to be checked for
 *			compatibility.
 *	compat_data	Array of possible compat entry strings and
 *			associated metadata. The last entry in the
 *			list should have a "compat" of NULL to terminate
 *			the list.
 *
 * Return Value:
 *	The first matching compat_data entry in the array. If no matches
 *	are found, NULL is returned.
 *
 * Side Effects:
 *	None.
 */
const struct device_compatible_entry *
of_compatible_lookup(int phandle,
    const struct device_compatible_entry *compat_data)
{
	char *prop, propbuf[OFW_MAX_STACK_BUF_SIZE];
	const struct device_compatible_entry *match = NULL;
	int proplen;

	proplen = OF_getproplen(phandle, "compatible");
	if (proplen <= 0) {
		return 0;
	}

	prop = kmem_tmpbuf_alloc(proplen, propbuf, sizeof(propbuf), KM_SLEEP);

	if (OF_getprop(phandle, "compatible", prop, proplen) != proplen) {
		goto out;
	}

	match = device_compatible_lookup_strlist(prop, proplen, compat_data);

 out:
	kmem_tmpbuf_free(prop, proplen, propbuf);
	return match;
}

/*
 * int of_packagename(phandle, buf, bufsize)
 *
 * This routine places the last component of an OFW node's name
 * into a user-provided buffer.
 *
 * It can be used during autoconfiguration to make printing of
 * device names more informative.
 *
 * Arguments:
 *	phandle		OFW phandle of device whose name name is
 *			desired.
 *	buf		Buffer to contain device name, provided by
 *			caller.  (For now, must be at least 4
 *			bytes long.)
 *	bufsize		Length of buffer referenced by 'buf', in
 *			bytes.
 *
 * Return Value:
 *	-1 if the device path name could not be obtained or would
 *	not fit in the allocated temporary buffer, or zero otherwise
 *	(meaning that the leaf node name was successfully extracted).
 *
 * Side Effects:
 *	If the leaf node name was successfully extracted, 'buf' is
 *	filled in with at most 'bufsize' bytes of the leaf node
 *	name.  If the leaf node was not successfully extracted, a
 *	somewhat meaningful string is placed in the buffer.  In
 *	either case, the contents of 'buf' will be NUL-terminated.
 */
int
of_packagename(int phandle, char *buf, int bufsize)
{
	char *pbuf;
	const char *lastslash;
	int l, rv;

	pbuf = kmem_alloc(OFW_PATH_BUF_SIZE, KM_SLEEP);
	l = OF_package_to_path(phandle, pbuf, OFW_PATH_BUF_SIZE);

	/* check that we could get the name, and that it's not too long. */
	if (l < 0 ||
	    (l == OFW_PATH_BUF_SIZE && pbuf[OFW_PATH_BUF_SIZE - 1] != '\0')) {
		if (bufsize >= 25)
			snprintf(buf, bufsize, "??? (phandle 0x%x)", phandle);
		else if (bufsize >= 4)
			strlcpy(buf, "???", bufsize);
		else
			panic("of_packagename: bufsize = %d is silly",
			    bufsize);
		rv = -1;
	} else {
		pbuf[l] = '\0';
		lastslash = strrchr(pbuf, '/');
		strlcpy(buf, (lastslash == NULL) ? pbuf : (lastslash + 1),
		    bufsize);
		rv = 0;
	}

	kmem_free(pbuf, OFW_PATH_BUF_SIZE);
	return (rv);
}

/* 
 * Find the first child of a given node that matches name. Does not recurse.
 */
int
of_find_firstchild_byname(int node, const char *name)
{
	char namex[32]; 
	int nn;
 
	for (nn = OF_child(node); nn; nn = OF_peer(nn)) {
		memset(namex, 0, sizeof(namex));
		if (OF_getprop(nn, "name", namex, sizeof(namex)) == -1)
			continue;
		if (strcmp(name, namex) == 0)
			return nn;
	}
	return -1;
}

/*
 * Find a child node that is compatible with str. Recurses, starting at node.
 */
int
of_find_bycompat(int node, const char *str)
{
	const char * compatible[] = { str, NULL };
	int child, ret;

	for (child = OF_child(node); child; child = OF_peer(child)) {
		if (of_compatible(child, compatible))
			return child;
		ret = of_find_bycompat(child, str);
		if (ret != -1)
			return ret;
	}

	return -1;
}

/*
 * Find a give node by name.  Recurses, and seems to walk upwards too.
 */

int
of_getnode_byname(int start, const char *target)
{
	int node, next;
	char name[64];

	if (start == 0)
		start = OF_peer(0);

	for (node = start; node; node = next) {
		memset(name, 0, sizeof name);
		OF_getprop(node, "name", name, sizeof name - 1);
		if (strcmp(name, target) == 0)
			break;

		if ((next = OF_child(node)) != 0)
			continue;

		while (node) {
			if ((next = OF_peer(node)) != 0)
				break;
			node = OF_parent(node);
		}
	}

	/* XXX is this correct? */
	return node;
}

/*
 * Create a uint32_t integer property from an OFW node property.
 */

bool
of_to_uint32_prop(prop_dictionary_t dict, int node, const char *ofname,
    const char *propname)
{
	uint32_t prop;

	if (OF_getprop(node, ofname, &prop, sizeof(prop)) != sizeof(prop))
		return FALSE;

	return(prop_dictionary_set_uint32(dict, propname, prop));
}

/*
 * Create a data property from an OFW node property.  Max size of 256bytes.
 */

bool
of_to_dataprop(prop_dictionary_t dict, int node, const char *ofname,
    const char *propname)
{
	int len;
	uint8_t prop[256];

	len = OF_getprop(node, ofname, prop, 256);
	if (len < 1)
		return FALSE;

	return prop_dictionary_set_data(dict, propname, prop, len);
}

/*
 * look at output-device, see if there's a Sun-typical video mode specifier as
 * in screen:r1024x768x60 attached. If found copy it into *buffer, otherwise
 * return NULL
 */

char *
of_get_mode_string(char *buffer, int len)
{
	int options;
	char *pos, output_device[256];

	/*
	 * finally, let's see if there's a video mode specified in
	 * output-device and pass it on so there's at least some way
	 * to program video modes
	 */
	options = OF_finddevice("/options");
	if ((options == 0) || (options == -1))
		return NULL;
	if (OF_getprop(options, "output-device", output_device, 256) == 0)
		return NULL;

	/* find the mode string if there is one */
	pos = strstr(output_device, ":r");
	if (pos == NULL)
		return NULL;
	strncpy(buffer, pos + 2, len);
	return buffer;
}

/*
 * of_device_from_phandle --
 *
 *	Return a device_t associated with the specified phandle.
 *
 *	This is expected to be used rarely, so we don't care if
 *	it's fast.  Also, it can only find devices that have
 *	gone through of_device_register() (obviously).
 */
device_t
of_device_from_phandle(int phandle)
{
	devhandle_t devhandle;
	deviter_t di;
	device_t dev;

	for (dev = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     dev != NULL;
	     dev = deviter_next(&di)) {
		devhandle = device_handle(dev);
		if (devhandle_type(devhandle) == DEVHANDLE_TYPE_OF) {
			if (devhandle_to_of(devhandle) == phandle) {
				/* Found it! */
				break;
			}
		}
	}
	deviter_release(&di);
	return dev;
}

/*
 * Returns true if the specified property is present.
 */
bool
of_hasprop(int node, const char *prop)
{
	return OF_getproplen(node, prop) >= 0;
}

/*
 * Get the value of a uint32 property, compensating for host byte order.
 * Returns 0 on success, non-zero on failure.
 */
int
of_getprop_uint32(int node, const char *prop, uint32_t *val)
{
	uint32_t v;
	int len;

	len = OF_getprop(node, prop, &v, sizeof(v));
	if (len != sizeof(v))
		return -1;

	*val = be32toh(v);
	return 0;
}

int
of_getprop_uint32_array(int node, const char *prop, uint32_t *array, int n)
{
	uint32_t *v = array;
	int len;

	len = OF_getprop(node, prop, array, n * sizeof(*v));
	if (len < (int)(n * sizeof(*v)))
		return -1;

	for (; n > 0; n--) {
		BE32TOH(*v);
		v++;
	}

	return 0;
}
/*
 * Get the value of a uint64 property, compensating for host byte order.
 * Returns 0 on success, non-zero on failure.
 */
int
of_getprop_uint64(int node, const char *prop, uint64_t *val)
{
	uint64_t v;
	int len;

	len = OF_getprop(node, prop, &v, sizeof(v));
	if (len != sizeof(v))
		return -1;

	*val = be64toh(v);
	return 0;
}
