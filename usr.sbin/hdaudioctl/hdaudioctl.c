/* $NetBSD: hdaudioctl.c,v 1.1 2009/10/11 08:57:54 sborrill Exp $ */

/*
 * Copyright (c) 2009 Precedence Technologies Ltd <support@precedence.co.uk>
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Precedence Technologies Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <prop/proplib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <dev/pci/hdaudio/hdaudioio.h>
#include <dev/pci/hdaudio/hdaudioreg.h>

#include "hdaudioctl.h"

#define DEVPATH_HDAUDIO	"/dev/hdaudio0"

void
usage(void)
{
	const char *prog;
	prog = getprogname();
	
	fprintf(stderr, "usage: %s [-f dev] list\n", prog);
	fprintf(stderr, "       %s [-f dev] get <codecid> <nid>\n", prog);
	fprintf(stderr, "       %s [-f dev] set <codecid> <nid> [plist]\n",
	    prog);
	fprintf(stderr, "       %s [-f dev] graph <codecid> <nid>\n", prog);
	exit(EXIT_FAILURE);
}

static int
hdaudioctl_list(int fd)
{
	prop_dictionary_t request, response;
	prop_dictionary_t dict;
	prop_object_iterator_t iter;
	prop_object_t obj;
	prop_array_t array;
	uint16_t nid, codecid;
	uint16_t vendor, product;
	uint32_t subsystem;
	const char *device = NULL;
	int error;

	request = prop_dictionary_create();
	if (request == NULL) {
		fprintf(stderr, "out of memory\n");
		return ENOMEM;
	}

	error = prop_dictionary_sendrecv_ioctl(request, fd,
	    HDAUDIO_FGRP_INFO, &response);
	if (error != 0) {
		perror("HDAUDIO_FGRP_INFO failed");
		return error;
	}

	array = prop_dictionary_get(response, "function-group-info");
	iter = prop_array_iterator(array);
	prop_object_iterator_reset(iter);
	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		dict = (prop_dictionary_t)obj;
		prop_dictionary_get_uint16(dict, "codecid", &codecid);
		prop_dictionary_get_uint16(dict, "nid", &nid);
		prop_dictionary_get_uint16(dict, "vendor-id", &vendor);
		prop_dictionary_get_uint16(dict, "product-id", &product);
		prop_dictionary_get_uint32(dict, "subsystem-id", &subsystem);
		prop_dictionary_get_cstring_nocopy(dict, "device", &device);

		printf("codecid 0x%02X nid 0x%02X vendor 0x%04X "
		    "product 0x%04X subsystem 0x%08X device %s\n",
		    codecid, nid, vendor, product, subsystem,
		    device ? device : "<none>");
	}

	prop_object_release(array);
	prop_object_release(response);
	prop_object_release(request);

	return 0;
}

static int
hdaudioctl_get(int fd, int argc, char *argv[])
{
	prop_dictionary_t request, response;
	prop_array_t config;
	uint16_t nid, codecid;
	const char *xml;
	int error;

	if (argc != 2)
		usage();

	codecid = strtol(argv[0], NULL, 0);
	nid = strtol(argv[1], NULL, 0);

	request = prop_dictionary_create();
	if (request == NULL) {
		fprintf(stderr, "out of memory\n");
		return ENOMEM;
	}

	prop_dictionary_set_uint16(request, "codecid", codecid);
	prop_dictionary_set_uint16(request, "nid", nid);

	error = prop_dictionary_sendrecv_ioctl(request, fd,
	    HDAUDIO_FGRP_GETCONFIG, &response);
	if (error != 0) {
		perror("HDAUDIO_FGRP_GETCONFIG failed");
		return error;
	}

	config = prop_dictionary_get(response, "pin-config");
	xml = prop_array_externalize(config);

	printf("%s\n", xml);

	prop_object_release(response);
	prop_object_release(request);

	return 0;
}

static int
hdaudioctl_set(int fd, int argc, char *argv[])
{
	prop_dictionary_t request, response;
	prop_array_t config = NULL;
	uint16_t nid, codecid;
	int error;

	if (argc < 2 || argc > 3)
		usage();

	codecid = strtol(argv[0], NULL, 0);
	nid = strtol(argv[1], NULL, 0);
	if (argc == 3) {
		config = prop_array_internalize_from_file(argv[2]);
		if (config == NULL) {
			fprintf(stderr,
			    "couldn't load configuration from %s\n", argv[2]);
			return EIO;
		}
	}

	request = prop_dictionary_create();
	if (request == NULL) {
		fprintf(stderr, "out of memory\n");
		return ENOMEM;
	}

	prop_dictionary_set_uint16(request, "codecid", codecid);
	prop_dictionary_set_uint16(request, "nid", nid);
	if (config)
		prop_dictionary_set(request, "pin-config", config);

	error = prop_dictionary_sendrecv_ioctl(request, fd,
	    HDAUDIO_FGRP_SETCONFIG, &response);
	if (error != 0) {
		perror("HDAUDIO_FGRP_SETCONFIG failed");
		return error;
	}

	prop_object_release(response);
	prop_object_release(request);

	return 0;
}


int
main(int argc, char *argv[])
{
	int fd, error;
	int ch;
	const char *devpath = DEVPATH_HDAUDIO;
	
	while ((ch = getopt(argc, argv, "f:h")) != -1) {
		switch (ch) {
		case 'f':
			devpath = strdup(optarg);
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	fd = open(devpath, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error opening %s: %s\n", devpath,
		    strerror(errno));
		return EXIT_FAILURE;
	}
	
	error = 0;
	if (strcmp(argv[0], "list") == 0)
		error = hdaudioctl_list(fd);
	else if (strcmp(argv[0], "get") == 0)
		error = hdaudioctl_get(fd, argc - 1, argv + 1);
	else if (strcmp(argv[0], "set") == 0)
		error = hdaudioctl_set(fd, argc - 1, argv + 1);
	else if (strcmp(argv[0], "graph") == 0)
		error = hdaudioctl_graph(fd, argc - 1, argv + 1);
	else
		usage();

	close(fd);

	if (error)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
