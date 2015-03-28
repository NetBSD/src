/* $NetBSD: graph.c,v 1.3 2015/03/28 14:09:59 jmcneill Exp $ */

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
#include <unistd.h>

#include <dev/hdaudio/hdaudioio.h>
#include <dev/hdaudio/hdaudioreg.h>

#include "hdaudioctl.h"

static const char *pin_devices[16] = {
	"Line Out", "Speaker", "HP Out", "CD",
	"SPDIF Out", "Digital Out", "Modem Line", "Modem Handset",
	"Line In", "AUX", "Mic In", "Telephony",
	"SPDIF In", "Digital In", "Reserved", "Other"
};

int
hdaudioctl_graph(int fd, int argc, char *argv[])
{
	prop_dictionary_t request, response;
	prop_object_iterator_t iter;
	prop_number_t nnid;
	prop_array_t connlist;
	const char *name;
	int error, index;
	uint32_t cap, config;
	uint16_t reqnid, reqcodecid;
	uint16_t vendor, product;
	uint8_t type, nid;
	char buf[10] = "??h";

	if (argc != 2)
		usage();

	reqcodecid = strtol(argv[0], NULL, 0);
	reqnid = strtol(argv[1], NULL, 0);

	request = prop_dictionary_create();
	if (request == NULL) {
		fprintf(stderr, "out of memory\n");
		return ENOMEM;
	}

	prop_dictionary_set_uint16(request, "codecid", reqcodecid);
	prop_dictionary_set_uint16(request, "nid", reqnid);

	error = prop_dictionary_sendrecv_ioctl(request, fd,
	    HDAUDIO_FGRP_CODEC_INFO, &response);
	if (error != 0) {
		perror("HDAUDIO_FGRP_CODEC_INFO failed");
		prop_object_release(request);
		return error;
	}
	
	prop_dictionary_get_uint16(response, "vendor-id", &vendor);
	prop_dictionary_get_uint16(response, "product-id", &product);

	printf("digraph \"HD Audio %04X:%04X\" {\n",
	    vendor, product);

	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_FGRP_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_cstring_nocopy(response, "name", &name);
		prop_dictionary_get_uint32(response, "cap", &cap);
		prop_dictionary_get_uint32(response, "config", &config);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_AUDIO_OUTPUT:
			printf(" %s [label=\"%s\\naudio output\",shape=box,style=filled,fillcolor=\""
			    "#88ff88\"];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_INPUT:
			printf(" %s [label=\"%s\\naudio input\",shape=box,style=filled,fillcolor=\""
			    "#ff8888\"];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_MIXER:
			printf(" %s [label=\"%s\\naudio mixer\","
			    "shape=invhouse];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_AUDIO_SELECTOR:
			printf(" %s [label=\"%s\\naudio selector\","
			    "shape=invtrapezium];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_PIN_COMPLEX:
			printf(" %s [label=\"%s\\ndevice=%s\",style=filled",
			    buf, buf,
			    pin_devices[COP_CFG_DEFAULT_DEVICE(config)]);
			if (cap & COP_PINCAP_OUTPUT_CAPABLE &&
			    cap & COP_PINCAP_INPUT_CAPABLE)
				puts(",shape=doublecircle,fillcolor=\""
				    "#ffff88\"];");
			else if (cap & COP_PINCAP_OUTPUT_CAPABLE)
				puts(",shape=circle,fillcolor=\"#88ff88\"];");
			else if (cap & COP_PINCAP_INPUT_CAPABLE)
				puts(",shape=circle,fillcolor=\"#ff8888\"];");
			else
				puts(",shape=circle,fillcolor=\"#888888\"];");
			break;
		case COP_AWCAP_TYPE_POWER_WIDGET:
			printf(" %s [label=\"%s\\npower widget\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_VOLUME_KNOB:
			printf(" %s [label=\"%s\\nvolume knob\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_BEEP_GENERATOR:
			printf(" %s [label=\"%s\\nbeep generator\","
			    "shape=box];\n", buf, buf);
			break;
		case COP_AWCAP_TYPE_VENDOR_DEFINED:
			printf(" %s [label=\"%s\\nvendor defined\","
			    "shape=box];\n", buf, buf);
			break;
		}
		connlist = prop_dictionary_get(response, "connlist");
		if (connlist == NULL)
			goto next;
		iter = prop_array_iterator(connlist);
		prop_object_iterator_reset(iter);
		while ((nnid = prop_object_iterator_next(iter)) != NULL) {
			nid = prop_number_unsigned_integer_value(nnid);
			printf(" widget%02Xh -> %s [sametail=widget%02Xh];\n",
			    nid, buf, nid);
		}
		prop_object_iterator_release(iter);
next:
		prop_object_release(response);
	}

	printf(" {rank=min;");
	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_AFG_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_cstring_nocopy(response, "name", &name);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_AUDIO_OUTPUT:
		case COP_AWCAP_TYPE_AUDIO_INPUT:
			printf(" %s;", buf);
			break;
		}
		prop_object_release(response);
	}
	printf("}\n");

	printf(" {rank=max;");
	for (index = 0;; index++) {
		prop_dictionary_set_uint16(request, "index", index);
		error = prop_dictionary_sendrecv_ioctl(request, fd,
		    HDAUDIO_AFG_WIDGET_INFO, &response);
		if (error != 0)
			break;
		prop_dictionary_get_cstring_nocopy(response, "name", &name);
		prop_dictionary_get_uint8(response, "type", &type);
		prop_dictionary_get_uint8(response, "nid", &nid);

		sprintf(buf, "widget%02Xh", nid);

		switch (type) {
		case COP_AWCAP_TYPE_PIN_COMPLEX:
			printf(" %s;", buf);
			break;
		}
		prop_object_release(response);
	}
	printf("}\n");

	printf("}\n");

	prop_object_release(request);

	return 0;
}
