/* $NetBSD: drvctl.c,v 1.2 2010/09/02 02:17:35 jmcneill Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "drvctl.h"

static int
drvctl_list(int fd, const char *name, struct devlistargs *laa)
{
	size_t children;

	memset(laa, 0, sizeof(*laa));
	strlcpy(laa->l_devname, name, sizeof(laa->l_devname));
	if (ioctl(fd, DRVLISTDEV, laa) == -1)
		return -1;
	children = laa->l_children;
	laa->l_childname = malloc(children * sizeof(laa->l_childname[0]));
	if (laa->l_childname == NULL)
		return -1;
	if (ioctl(fd, DRVLISTDEV, laa) == -1)
		return -1;
	return 0;
}

static bool
drvctl_get_properties(int fd, const char *devnode, prop_dictionary_t *props)
{
	prop_dictionary_t command_dict;
	prop_dictionary_t args_dict;
	prop_dictionary_t results_dict;
	int8_t perr;
	int error;
	bool rv;

	command_dict = prop_dictionary_create();
	args_dict = prop_dictionary_create();

	prop_dictionary_set_cstring_nocopy(command_dict, "drvctl-command",
	    "get-properties");
	prop_dictionary_set_cstring_nocopy(args_dict, "device-name", devnode);
	prop_dictionary_set(command_dict, "drvctl-arguments", args_dict);
	prop_object_release(args_dict);

	error = prop_dictionary_sendrecv_ioctl(command_dict, fd,
	    DRVCTLCOMMAND, &results_dict);
	prop_object_release(command_dict);
	if (error)
		return false;

	rv = prop_dictionary_get_int8(results_dict, "drvctl-error", &perr);
	if (rv == false || perr != 0) {
		prop_object_release(results_dict);
		return false;
	}

	if (props) {
		prop_dictionary_t result_data;
		result_data = prop_dictionary_get(results_dict,
		    "drvctl-result-data");
		if (result_data)
			*props = prop_dictionary_copy(result_data);
	}

	prop_object_release(results_dict);

	return true;
}

static int
drvctl_search(int fd, const char *curnode, const char *dvname,
    void (*callback)(void *, const char *, const char *, unsigned int),
    void *args)
{
	struct devlistargs laa;
	unsigned int i;
	uint32_t unit;
	bool rv;

	if (drvctl_list(fd, curnode, &laa) == -1)
		return -1;

	for (i = 0; i < laa.l_children; i++) {
		prop_dictionary_t props = NULL;
		const char *curdvname;

		rv = drvctl_get_properties(fd, laa.l_childname[i], &props);
		if (rv == false || props == NULL)
			continue;
		rv = prop_dictionary_get_cstring_nocopy(props,
		    "device-driver", &curdvname);
		if (rv == true && strcmp(curdvname, dvname) == 0) {
			rv = prop_dictionary_get_uint32(props,
			    "device-unit", &unit);
			if (rv == true)
				callback(args, curnode,
				    laa.l_childname[i], unit);
		}
		prop_object_release(props);

		if (drvctl_search(fd, laa.l_childname[i], dvname,
		    callback, args) == -1)
			return -1;
	}

	return 0;
}

int
drvctl_foreach(int fd, const char *dvname,
    void (*callback)(void *, const char *, const char *, unsigned int),
    void *args)
{
	return drvctl_search(fd, "", dvname, callback, args);
}
