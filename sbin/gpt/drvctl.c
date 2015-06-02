/*	$NetBSD: drvctl.c,v 1.1.2.2 2015/06/02 19:49:38 snj Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: drvctl.c,v 1.1.2.2 2015/06/02 19:49:38 snj Exp $");

#include <sys/types.h>
#include <sys/disk.h>
#include <sys/drvctlio.h>

#include <prop/proplib.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "map.h"
#include "gpt.h"

int
getdisksize(const char *name, u_int *sector_size, off_t *media_size)
{
	prop_dictionary_t command_dict, args_dict, results_dict, data_dict,
	    disk_info, geometry;
	prop_string_t string;
	prop_number_t number;
	int dfd, res;
	char *dname, *p;

	if (*sector_size && *media_size)
		return 0;

	if ((dfd = open("/dev/drvctl", O_RDONLY)) == -1) {
		warn("%s: /dev/drvctl", __func__);
		return -1;
	}

	command_dict = prop_dictionary_create();
	args_dict = prop_dictionary_create();

	string = prop_string_create_cstring_nocopy("get-properties");
	prop_dictionary_set(command_dict, "drvctl-command", string);
	prop_object_release(string);

	if ((dname = strdup(name[0] == 'r' ? name + 1 : name)) == NULL) {
		(void)close(dfd);
		return -1;
	}
	for (p = dname; *p; p++)
		continue;
	for (--p; p >= dname && !isdigit((unsigned char)*p); *p-- = '\0')
		continue;

	string = prop_string_create_cstring(dname);
	free(dname);
	prop_dictionary_set(args_dict, "device-name", string);
	prop_object_release(string);

	prop_dictionary_set(command_dict, "drvctl-arguments", args_dict);
	prop_object_release(args_dict);

	res = prop_dictionary_sendrecv_ioctl(command_dict, dfd, DRVCTLCOMMAND,
	    &results_dict);
	(void)close(dfd);
	prop_object_release(command_dict);
	if (res) {
		warn("%s: prop_dictionary_sendrecv_ioctl", __func__);
		errno = res;
		return -1;
	}

	number = prop_dictionary_get(results_dict, "drvctl-error");
	if ((errno = prop_number_integer_value(number)) != 0)
		return -1;

	data_dict = prop_dictionary_get(results_dict, "drvctl-result-data");
	if (data_dict == NULL)
		goto out;

	disk_info = prop_dictionary_get(data_dict, "disk-info");
	if (disk_info == NULL)
		goto out;

	geometry = prop_dictionary_get(disk_info, "geometry");
	if (geometry == NULL)
		goto out;

	number = prop_dictionary_get(geometry, "sector-size");
	if (number == NULL)
		goto out;

	if (*sector_size == 0)
		*sector_size = prop_number_integer_value(number);

	number = prop_dictionary_get(geometry, "sectors-per-unit");
	if (number == NULL)
		goto out;

	if (*media_size == 0)
		*media_size = prop_number_integer_value(number) * *sector_size;

	return 0;
out:
	errno = EINVAL;
	return -1;
}
