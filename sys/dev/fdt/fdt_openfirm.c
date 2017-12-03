/* $NetBSD: fdt_openfirm.c,v 1.2.18.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_openfirm.c,v 1.2.18.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

int
OF_peer(int phandle)
{
	const void *fdt_data = fdtbus_get_data();
	int off, depth;

	if (fdt_data == NULL) {
		return -1;
	}

	if (phandle == 0) {
		return fdtbus_offset2phandle(0);
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return 0;
	}

	depth = 1;
	for (off = fdt_next_node(fdt_data, off, &depth);
	     off >= 0 && depth >= 0;
	     off = fdt_next_node(fdt_data, off, &depth)) {
		if (depth == 1) {
			return fdtbus_offset2phandle(off);
		}
	}

	return 0;
}

int
OF_child(int phandle)
{
	const void *fdt_data = fdtbus_get_data();
	int off, depth;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return 0;
	}

	depth = 0;
	for (off = fdt_next_node(fdt_data, off, &depth);
	     off >= 0 && depth > 0;
	     off = fdt_next_node(fdt_data, off, &depth)) {
		if (depth == 1) {
			return fdtbus_offset2phandle(off);
		}
	}

	return 0;
}

int
OF_parent(int phandle)
{
	const void *fdt_data = fdtbus_get_data();
	int off;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return -1;
	}

	off = fdt_parent_offset(fdt_data, off);
	if (off < 0) {
		return -1;
	}

	return fdtbus_offset2phandle(off);
}

int
OF_nextprop(int phandle, const char *prop, void *nextprop)
{
	const void *fdt_data = fdtbus_get_data();
	const char *name;
	const void *val;
	int off, len;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return -1;
	}

	if (*prop == '\0') {
		name = "name";
	} else {
		off = fdt_first_property_offset(fdt_data, off);
		if (off < 0) {
			return 0;
		}
		if (strcmp(prop, "name") != 0) {
			while (off >= 0) {
				val = fdt_getprop_by_offset(fdt_data, off,
				    &name, &len);
				if (val == NULL) {
					return -1;
				}
				off = fdt_next_property_offset(fdt_data, off);
				if (off < 0) {
					return 0;
				}
				if (strcmp(name, prop) == 0)
					break;
			}
		}
		val = fdt_getprop_by_offset(fdt_data, off, &name, &len);
		if (val == NULL) {
			return -1;
		}
	}

	strlcpy(nextprop, name, 33);

	return 1;
}

int
OF_getprop(int phandle, const char *prop, void *buf, int buflen)
{
	const void *fdt_data = fdtbus_get_data();
	const char *name;
	const void *val;
	int off, len;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return -1;
	}

	if (strcmp(prop, "name") == 0) {
		val = fdt_get_name(fdt_data, off, &len);
		if (val) {
			const char *p = strchr(val, '@');
			if (p) {
				len = (uintptr_t)p - (uintptr_t)val + 1;
			} else {
				len += 1;
			}
		}
		if (val == NULL || len > buflen) {
			return -1;
		}
		char *s = buf;
		memcpy(buf, val, len - 1);
		s[len - 1] = '\0';
	} else {
		off = fdt_first_property_offset(fdt_data, off);
		if (off < 0) {
			return -1;
		}
		while (off >= 0) {
			val = fdt_getprop_by_offset(fdt_data, off, &name, &len);
			if (val == NULL) {
				return -1;
			}
			if (strcmp(name, prop) == 0) {
				break;
			}
			off = fdt_next_property_offset(fdt_data, off);
			if (off < 0) {
				return -1;
			}
		}
		if (val == NULL || len > buflen) {
			return -1;
		}
		memcpy(buf, val, len);
	}

	return len;
}

int
OF_getproplen(int phandle, const char *prop)
{
	const void *fdt_data = fdtbus_get_data();
	const char *name;
	const void *val;
	int off, len;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return -1;
	}

	if (strcmp(prop, "name") == 0) {
		val = fdt_get_name(fdt_data, off, &len);
		if (val) {
			const char *p = strchr(val, '@');
			if (p) {
				len = (uintptr_t)p - (uintptr_t)val + 1;
			} else {
				len += 1;
			}
		}
	} else {
		off = fdt_first_property_offset(fdt_data, off);
		if (off < 0) {
			return -1;
		}
		while (off >= 0) {
			val = fdt_getprop_by_offset(fdt_data, off, &name, &len);
			if (val == NULL) {
				return -1;
			}
			if (strcmp(name, prop) == 0) {
				break;
			}
			off = fdt_next_property_offset(fdt_data, off);
			if (off < 0) {
				return -1;
			}
		}
	}
	if (val == NULL) {
		return -1;
	}

	return len;
}

int
OF_setprop(int phandle, const char *prop, const void *buf, int buflen)
{
	return -1;
}

int
OF_finddevice(const char *name)
{
	const void *fdt_data = fdtbus_get_data();
	int off;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdt_path_offset(fdt_data, name);
	if (off < 0) {
		return -1;
	}

	return fdtbus_offset2phandle(off);
}

int
OF_package_to_path(int phandle, char *buf, int buflen)
{
	const void *fdt_data = fdtbus_get_data();
	int off;

	if (fdt_data == NULL) {
		return -1;
	}

	off = fdtbus_phandle2offset(phandle);
	if (off < 0) {
		return -1;
	}

	if (fdt_get_path(fdt_data, off, buf, buflen) != 0)
		return -1;

	return strlen(buf);
}
