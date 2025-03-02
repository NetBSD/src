/* $NetBSD: efiio.c,v 1.2 2025/03/02 00:03:41 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: efiio.c,v 1.2 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <sys/efiio.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "defs.h"
#include "efiio.h"
#include "utils.h"

//******************************************************
// Variable Attributes
//******************************************************
#if 0	/* see sys/efiio.h */
struct efi_var_ioc {
	uint16_t *	name;		/* vendor's variable name */
	size_t		namesize;	/* size in bytes of the name buffer */
	struct uuid	vendor;		/* unique identifier for vendor */
	uint32_t	attrib;		/* variable attribute bitmask */
	void *		data;		/* buffer containing variable data */
	size_t		datasize;	/* size in bytes of the data buffer */
};
#endif

static int
xioctl(int fd, unsigned long request, void *buf)
{
	int rv;

	rv = ioctl(fd, request, buf);
	if (rv == -1 && errno != ENOENT)
		err(EXIT_FAILURE, "%s: ioctl", __func__);
	return rv;
}

PUBLIC int
set_variable(int fd, struct efi_var_ioc *ev)
{

	return ioctl(fd, EFIIOC_VAR_SET, ev);
}

static inline int
get_variable_core(int fd, struct efi_var_ioc *ev)
{

	if (xioctl(fd, EFIIOC_VAR_GET, ev) == -1)
		return -1;

	ev->data = emalloc(ev->datasize);
	xioctl(fd, EFIIOC_VAR_GET, ev);
	return 0;
}

PUBLIC struct efi_var_ioc
get_variable(int fd, const char *name, struct uuid *vendor,
    uint32_t attrib)
{
	struct efi_var_ioc ev;

	efi_var_init(&ev, name, vendor, attrib);
	get_variable_core(fd, &ev);

	return ev;
}

PUBLIC struct efi_var_ioc *
get_next_variable(int fd, struct efi_var_ioc *ev)
{
	int rv;

	rv = ioctl(fd, EFIIOC_VAR_NEXT, ev);
	if (rv == -1 && errno != ENOENT)
		err(EXIT_FAILURE, "%s: ioctl", __func__);
	return ev;
}

PUBLIC void *
get_table(int fd, struct uuid *uuid, size_t *buflen)
{
	struct efi_get_table_ioc egt;

	memset(&egt, 0, sizeof(egt));
	memcpy(&egt.uuid, uuid, sizeof(egt.uuid));

	xioctl(fd, EFIIOC_GET_TABLE, &egt);
	egt.buf = ecalloc(egt.table_len, 1);
	egt.buf_len = egt.table_len;

	xioctl(fd, EFIIOC_GET_TABLE, &egt);
	*buflen = egt.table_len;
	return egt.buf;
}

PUBLIC size_t
get_variable_info(int fd, bool (*choose)(struct efi_var_ioc *, void *),
    int (*fn)(struct efi_var_ioc *ev, void *), void *arg)
{
	struct efi_var_ioc ev;
	size_t cnt;
	int rv;

	assert(fn != NULL);
	assert(arg != NULL);

	memset(&ev, 0, sizeof(ev));

	ev.name = ecalloc(EFI_VARNAME_MAXLENGTH, sizeof(*ev.name));
	cnt = 0;
	for (;;) {
		ev.namesize = EFI_VARNAME_MAXLENGTH;
		if (ioctl(fd, EFIIOC_VAR_NEXT, &ev) == -1) {
			char *buf;

			if (errno == ENOENT)
				break;

			/* XXX: ev is likely to be zero */
			buf = ucs2_to_utf8(ev.name, ev.namesize, NULL, NULL);
			err(EXIT_FAILURE, "%s: '%s'", __func__, buf);
		}

		if (choose != NULL && !choose(&ev, arg))
			continue;

		rv = get_variable_core(fd, &ev);
		assert(rv == 0);
		if (rv == -1)
			err(EXIT_FAILURE, "get_variable_core");

		cnt++;
		fn(&ev, arg);

		free(ev.data);
	}
	free(ev.name);
	return cnt;
}
