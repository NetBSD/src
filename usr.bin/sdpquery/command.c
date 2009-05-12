/*	$NetBSD: command.c,v 1.1 2009/05/12 18:37:50 plunky Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Iain Hibbert.
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
__RCSID("$NetBSD: command.c,v 1.1 2009/05/12 18:37:50 plunky Exp $");

#include <bluetooth.h>
#include <err.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>

#include "sdpquery.h"

static sdp_session_t open_session(void);
static void build_ssp(sdp_data_t *, int, const char **);

static struct alias {
	uint16_t	uuid;
	const char *	name;
	const char *	desc;
} aliases[] = {
	{ SDP_SERVICE_CLASS_ADVANCED_AUDIO_DISTRIBUTION,
	  "A2DP",	"Advanced Audio Distribution Profile"		},
	{ SDP_UUID_PROTOCOL_BNEP,
	  "BNEP",	"Bluetooth Network Encapsulation Protocol"	},
	{ SDP_SERVICE_CLASS_COMMON_ISDN_ACCESS,
	  "CIP",	"Common ISDN Access Service"			},
	{ SDP_SERVICE_CLASS_CORDLESS_TELEPHONY,
	  "CTP",	"Cordless Telephony Service"			},
	{ SDP_SERVICE_CLASS_DIALUP_NETWORKING,
	  "DUN",	"Dial Up Networking Service"			},
	{ SDP_SERVICE_CLASS_FAX,
	  "FAX",	"Fax Service"					},
	{ SDP_SERVICE_CLASS_OBEX_FILE_TRANSFER,
	  "FTRN",	"File Transfer Service"				},
	{ SDP_SERVICE_CLASS_GN,
	  "GN",		"Group ad-hoc Network Service"			},
	{ SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE,
	  "HID",	"Human Interface Device Service"		},
	{ SDP_SERVICE_CLASS_HANDSFREE,
	  "HF",		"Handsfree Service"				},
	{ SDP_SERVICE_CLASS_HEADSET,
	  "HSET",	"Headset Service"				},
	{ SDP_UUID_PROTOCOL_L2CAP,
	  "L2CAP",	"Logical Link Control and Adapatation Protocol"	},
	{ SDP_SERVICE_CLASS_LAN_ACCESS_USING_PPP,
	  "LAN",	"Lan access using PPP Service"			},
	{ SDP_SERVICE_CLASS_NAP,
	  "NAP",	"Network Access Point Service"			},
	{ SDP_UUID_PROTOCOL_OBEX,
	  "OBEX",	"Object Exchange Protocol"			},
	{ SDP_SERVICE_CLASS_OBEX_OBJECT_PUSH,
	  "OPUSH",	"Object Push Service"				},
	{ SDP_SERVICE_CLASS_PANU,
	  "PANU",	"Personal Area Networking User Service"		},
	{ SDP_UUID_PROTOCOL_RFCOMM,
	  "RFCOMM",	"RFCOMM Protocol"				},
	{ SDP_UUID_PROTOCOL_SDP,
	  "SDP",	"Service Discovery Protocol"			},
	{ SDP_SERVICE_CLASS_SERIAL_PORT,
	  "SP",		"Serial Port Service"				},
	{ SDP_SERVICE_CLASS_IR_MC_SYNC,
	  "SYNC",	"IrMC Sync Client Service"			},
};

int
do_sdp_browse(int argc, const char **argv)
{
#define STR(x)	__STRING(x)
	const char *av = STR(SDP_SERVICE_CLASS_PUBLIC_BROWSE_GROUP);
#undef STR

	if (argc > 1)
		errx(EXIT_FAILURE, "Too many arguments");

	if (argc == 0) {
		argc = 1;
		argv = &av;
	}

	return do_sdp_search(argc, argv);
}

int
do_sdp_record(int argc, const char **argv)
{
	sdp_session_t	ss;
	sdp_data_t	rsp;
	char *		ep;
	unsigned long	handle;
	bool		rv;

	if (argc == 0)
		errx(EXIT_FAILURE, "Record handle required");

	ss = open_session();

	for (; argc-- > 0; argv++) {
		handle = strtoul(*argv, &ep, 0);
		if (*argv[0] == '\0' || *ep != '\0' || handle > UINT32_MAX)
			errx(EXIT_FAILURE, "Invalid handle: %s\n", *argv);

		rv = sdp_service_attribute(ss, handle, NULL, &rsp);
		if (!rv)
			warn("%s", *argv);
		else
			print_record(&rsp);

		if (argc > 0)
			printf("\n\n");
	}

	sdp_close(ss);
	return EXIT_SUCCESS;
}

int
do_sdp_search(int argc, const char **argv)
{
	sdp_session_t	ss;
	sdp_data_t	ssp, rec, rsp;
	bool		rv;

	if (argc < 1)
		errx(EXIT_FAILURE, "UUID required");

	if (argc > 12)
		errx(EXIT_FAILURE, "Too many UUIDs");

	build_ssp(&ssp, argc, argv);

	ss = open_session();

	rv = sdp_service_search_attribute(ss, &ssp, NULL, &rsp);
	if (!rv)
		err(EXIT_FAILURE, "sdp_service_search_attribute");

	while (sdp_get_seq(&rsp, &rec)) {
		if (!rv)
			printf("\n\n");
		else
			rv = false;

		print_record(&rec);
	}

	if (rsp.next != rsp.end) {
		printf("\n\nAdditional Data:\n");
		sdp_data_print(&rsp, 4);
	}

	sdp_close(ss);

	return EXIT_SUCCESS;
}

static sdp_session_t
open_session(void)
{
	sdp_session_t ss;

	if (bdaddr_any(&remote_addr))
		ss = sdp_open_local(control_socket);
	else
		ss = sdp_open(&local_addr, &remote_addr);

	if (ss == NULL)
		err(EXIT_FAILURE, "sdp_open");

	return ss;
}

/*
 * build ServiceSearchPattern from arglist
 */
static void
build_ssp(sdp_data_t *ssp, int argc, const char **argv)
{
	static uint8_t	data[36];	/* 12 * sizeof(uuid16) */
	char *		ep;
	unsigned long	uuid;
	int		i;

	ssp->next = data;
	ssp->end = data + sizeof(data);

	for (; argc-- > 0; argv++) {
		uuid = strtoul(*argv, &ep, 0);
		if (*argv[0] == '\0' || *ep != '\0') {
			for (i = 0;; i++) {
				if (i == __arraycount(aliases))
					errx(EXIT_FAILURE,
					    "Unknown alias \"%s\"", *argv);

				if (strcasecmp(aliases[i].name, *argv) == 0)
					break;
			}

			uuid = aliases[i].uuid;
		}

		if (uuid > UINT16_MAX)
			errx(EXIT_FAILURE, "%s: Not 16-bit UUID", *argv);

		sdp_put_uuid16(ssp, uuid);
	}

	ssp->end = ssp->next;
	ssp->next = data;
	return;
}
