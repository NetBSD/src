/* $NetBSD: setvar.c,v 1.3 2025/03/02 00:23:59 riastradh Exp $ */

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
__RCSID("$NetBSD: setvar.c,v 1.3 2025/03/02 00:23:59 riastradh Exp $");
#endif /* not lint */

#include <sys/efiio.h>

#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid.h>

#include "efiio.h"
#include "defs.h"
#include "bootvar.h"
#include "setvar.h"
#include "utils.h"

#define BOOTNEXT	"BootNext"
#define BOOTORDER	"BootOrder"
#define TIMEOUT		"Timeout"

static size_t
parse_csus(const char *csus, uint16_t **array, int base)
{
	uint16_t *data;
	const char *p;
	char *q;
	size_t n;

	n = 1;
	for (p = csus; *(p = strchrnul(p, ',')) != '\0'; p++)
		n++;

	data = emalloc(n * sizeof(*data));

	n = 0;
	p = csus;
	for (;;) {
		data[n++] = strtous(p, &q, base);
		if (*q != ',' && *q != '\0')
			errx(EXIT_FAILURE, "invalid CSUS string: '%s' (at %s)\n",
			    csus, p);
		if (*q == '\0')
			break;
		p = q + 1;
	}

	*array = data;
	return n;
}

PUBLIC int
prefix_bootorder(int fd, const char *target, const char *csus,
    uint16_t bootnum)
{
	struct efi_var_ioc ev;
	char *targetorder;
	uint16_t *data;
	size_t datasize, n;
	int rv;

	easprintf(&targetorder, "%sOrder", target);
	ev = get_variable(fd, targetorder,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);

	free(targetorder);

	if (csus != NULL)
		n = parse_csus(csus, &data, 16);
	else {
		data = emalloc(sizeof(*data));
		*data = bootnum;
		n = 1;
	}

	datasize = ev.datasize + n * sizeof(*data);
	data = erealloc(data, datasize);
	memcpy(data + n, ev.data, ev.datasize);

//	free(ev.data);	/* XXX: ??? */

	ev.data = data;
	ev.datasize = datasize;

	rv = set_variable(fd, &ev);
	free(ev.data);
	if (rv == -1)
		err(EXIT_FAILURE, "prefix_bootorder: %s %s", target, csus);
	return rv;
}

PUBLIC int
remove_bootorder(int fd, const char *target, const char *csus,
    uint16_t bootnum)
{
	struct efi_var_ioc ev;
	char *targetorder;
	uint16_t *data, *dp, *rmlist;
	size_t j, n, r;
	int rv;

	easprintf(&targetorder, "%sOrder", target);
	ev = get_variable(fd, targetorder, &EFI_GLOBAL_VARIABLE, 0);
	free(targetorder);

	if (ev.datasize == 0)	/* no such variable */
		return 0;

	if (csus != NULL)
		r = parse_csus(csus, &rmlist, 16);
	else {
		rmlist = emalloc(sizeof(*rmlist));
		*rmlist = bootnum;
		r = 1;
	}

	data = emalloc(ev.datasize);
	dp = ev.data;
	n = ev.datasize / sizeof(*data);
	j = 0;
	for (size_t i = 0; i < n; i++) {
		size_t k;
		for (k = 0; k < r; k++) {
			if (dp[i] == rmlist[k])
				break;
		}
		if (k == r)
			data[j++] = dp[i];
	}
	ev.data = data;
	ev.datasize = j * sizeof(*data);

	rv = set_variable(fd, &ev);
	free(ev.data);
	if (rv == -1)
		err(EXIT_FAILURE, "remove_bootorder: %s %u", target, bootnum);
	return rv;
}

PUBLIC int
set_bootorder(int fd, const char *target, const char *bootorder)
{
	struct efi_var_ioc ev;
	char *targetorder;
	uint16_t *data;
	size_t n;
	int rv;

	easprintf(&targetorder, "%sOrder", target);
	efi_var_init(&ev, targetorder,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);
	free(targetorder);

	n = parse_csus(bootorder, &data, 16);
	ev.data = data;
	ev.datasize = n * sizeof(*data);

	rv = set_variable(fd, &ev);
	if (rv == -1)
		warn("set_variable");

	free(ev.name);
	return rv;
}

static int
delete_variable(int fd, const char *varname)
{
	struct efi_var_ioc ev;
	int rv;

	efi_var_init(&ev, varname,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);

	rv = set_variable(fd, &ev);
	free(ev.name);
	return rv;
}

PUBLIC int
del_bootorder(int fd, const char *target __unused)
{
	int rv;

	rv = delete_variable(fd, BOOTORDER);
	if (rv == -1)
		warn("delete_variable");

	return rv;
}

PUBLIC int
del_bootorder_dups(int fd, const char *target)
{
	struct efi_var_ioc ev;
	char *targetorder;
	uint16_t *data, *dp;
	size_t i, j, k, n;
	int rv;

	easprintf(&targetorder, "%sOrder", target);
	ev = get_variable(fd, targetorder, &EFI_GLOBAL_VARIABLE, 0);
	free(targetorder);

	if (ev.datasize == 0)
		return 0;

	data = emalloc(ev.datasize);
	dp = ev.data;

	n = ev.datasize / sizeof(*data);
	j = 0;
	for (i = 0; i < n; i++) {
		for (k = 0; k < j; k++) {  /* XXX: O(n^2) */
			if (data[k] == dp[i])
				break;
		}
		if (k == j)
			data[j++] = dp[i];
	}

	ev.data = data;
	ev.datasize = j * sizeof(*data);

	rv = set_variable(fd, &ev);
	free(ev.data);
	if (rv == -1)
		err(EXIT_FAILURE, "del_bootorder_dups");
	return rv;
}

PUBLIC int
set_bootnext(int fd, uint16_t bootnum)
{
	struct efi_var_ioc ev;
	int rv;

	efi_var_init(&ev, BOOTNEXT,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);

	ev.data = &bootnum;
	ev.datasize = sizeof(bootnum);

	printf("set BootNext = Boot%04X\n", bootnum);

	rv = set_variable(fd, &ev);
	if (rv == -1)
		warn("set_variable");

	free(ev.name);
	return rv;
}

PUBLIC int
del_bootnext(int fd)
{
	int rv;

	rv = delete_variable(fd, BOOTNEXT);
	if (rv == -1)
		warn("delete_variable");

	return rv;
}

PUBLIC int
set_timeout(int fd, uint16_t timeout)
{
	struct efi_var_ioc ev;
	int rv;

	efi_var_init(&ev, TIMEOUT,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);

	ev.data = &timeout;
	ev.datasize = sizeof(timeout);

	printf("set Timeout = %u seconds\n", timeout);

	rv = set_variable(fd, &ev);
	if (rv == -1)
		warn("set_variable");

	free(ev.name);
	return rv;
}

PUBLIC int
del_timeout(int fd)
{
	int rv;

	rv = delete_variable(fd, TIMEOUT);
	if (rv == -1)
		warn("del_variable");

	return rv;
}

PUBLIC int
set_active(int efi_fd, const char *target, uint16_t bootnum, bool active)
{
	struct efi_var_ioc ev;
	boot_var_t *bb;
	char *name;
	int rv;

	easprintf(&name, "%s%04X", target, bootnum);
	ev = get_variable(efi_fd, name, &EFI_GLOBAL_VARIABLE, 0);
	free(name);

	bb = ev.data;
	if (active)
		bb->Attributes |= LOAD_OPTION_ACTIVE;
	else
		bb->Attributes &= (uint32_t)(~LOAD_OPTION_ACTIVE);

	rv = set_variable(efi_fd, &ev);
	if (rv == -1)
		err(EXIT_FAILURE, "set_variable");

	return rv;
}

#if 0
PUBLIC int
del_variable(int fd, const char *varname)
{
	struct efi_var_ioc ev;
	int rv;

	efi_var_init(&ev, varname,
	    &EFI_GLOBAL_VARIABLE,
	    EFI_VARIABLE_NON_VOLATILE |
	    EFI_VARIABLE_BOOTSERVICE_ACCESS |
	    EFI_VARIABLE_RUNTIME_ACCESS);

	rv = set_variable(fd, &ev);
	free(ev.name);
	return rv;
}
#endif

PUBLIC int
del_variable(int efi_fd, const char *target, uint16_t bootnum)
{
	char *name;
	int rv;

	easprintf(&name, "%s%04X", target, bootnum);
	printf("deleting '%s'\n", name);
	rv = delete_variable(efi_fd, name);
	free(name);

	if (rv == -1)
		err(EXIT_FAILURE, "del_variable");

	rv = remove_bootorder(efi_fd, target, NULL, bootnum);
	if (rv == -1)
		err(EXIT_FAILURE, "remove_bootorder");

	return rv;
}
