/* $NetBSD: bta2dpd.c,v 1.5 2018/01/13 10:20:45 nat Exp $ */

/*-
 * Copyright (c) 2015 - 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 *		This software is dedicated to the memory of -
 *	   Baron James Anlezark (Barry) - 1 Jan 1949 - 13 May 2012.
 *
 *		Barry was a man who loved his music.
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

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* This was based upon bthset.c */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2006 Itronix, Inc.  All rights reserved.");
__RCSID("$NetBSD");

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <bluetooth.h>
#include <err.h>
#include <event.h>
#include <fcntl.h>
#include <sdp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "avdtp_signal.h"
#include "sbc_encode.h"

#define A2DP_SINK	0x110b

/* A2DP Audio Source service record */
static uint8_t source_data[] = {
	0x09, 0x00, 0x00,	//  uint16	ServiceRecordHandle
	0x0a, 0x00, 0x00, 0x00,	//  uint32	0x00000000
	0x00,

	0x09, 0x00, 0x01,	//  uint16	ServiceClassIDList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x11, 0x0a,	//   uuid16	AudioSource

	0x09, 0x00, 0x04,	//  uint16	ProtocolDescriptorList
	0x35, 0x10,		//  seq8(16)
	0x35, 0x06,		//   seq8(6)
	0x19, 0x01, 0x00,	//    uuid16	L2CAP
	0x09, 0x00, 0x19,	//    uint16	%src_psm%
	0x35, 0x06,		//   seq8(6)
	0x19, 0x00, 0x19,	//    uuid16	AVDTP
	0x09, 0x01, 0x00,	//    uint16	Ver 1.0

	0x09, 0x00, 0x05,	//  uint16	BrowseGroupList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x10, 0x02,	//   uuid16	PublicBrowseGroup

	0x09, 0x00, 0x06,	//  uint16	LanguageBaseAttributeIDList
	0x35, 0x09,		//  seq8(9)
	0x09, 0x65, 0x6e,	//   uint16	0x656e	("en")
	0x09, 0x00, 0x6a,	//   uint16	106	(UTF-8)
	0x09, 0x01, 0x00,	//   uint16	PrimaryLanguageBaseID

	0x09, 0x00, 0x09,	//  uint16	BluetoothProfileDescriptorList
	0x35, 0x08,		//  seq8(8)
	0x35, 0x06,		//   seq8(6)
	0x19, 0x11, 0x0d,	//    uuid16	A2DP
	0x09, 0x01, 0x02,	//    uint16	v1.2

	0x09, 0x01, 0x00,	//  uint16	PrimaryLanguageBaseID +
				//		    ServiceNameOffset
	0x25, 0x09,		//  str8(9)	"Audio SRC"
	'A', 'u', 'd', 'i', 'o', ' ', 'S', 'R', 'C',

	0x09, 0x01, 0x02,	//  uint16	PrimaryLanguageBaseID +
				//		    ProviderName
	0x25, 0x06,		//  str8(6)	"NetBSD"
	'N', 'e', 't', 'B', 'S', 'D',

	0x09, 0x03, 0x11,	//  uint16	SupportedFeatures
	0x09, 0x01, 0x02	//  uint16	Headphone, Speaker
};

static sdp_data_t source_record =  { source_data + 0, source_data + 103 };
static sdp_data_t source_psm = { source_data + 26, source_data + 29 };


/* A2DP Audio Sink service record */
static uint8_t sink_data[] = {
	0x09, 0x00, 0x00,	//  uint16	ServiceRecordHandle
	0x0a, 0x00, 0x00, 0x00,	//  uint32	0x00000000
	0x00,

	0x09, 0x00, 0x01,	//  uint16	ServiceClassIDList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x11, 0x0b,	//   uuid16	AudioSink

	0x09, 0x00, 0x04,	//  uint16	ProtocolDescriptorList
	0x35, 0x10,		//  seq8(16)
	0x35, 0x06,		//   seq8(6)
	0x19, 0x01, 0x00,	//    uuid16	L2CAP
	0x09, 0x00, 0x19,	//    uint16	%sink_psm%
	0x35, 0x06,		//   seq8(6)
	0x19, 0x00, 0x19,	//    uuid16	AVDTP
	0x09, 0x01, 0x00,	//    uint16	Ver 1.0

	0x09, 0x00, 0x05,	//  uint16	BrowseGroupList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x10, 0x02,	//   uuid16	PublicBrowseGroup

	0x09, 0x00, 0x06,	//  uint16	LanguageBaseAttributeIDList
	0x35, 0x09,		//  seq8(9)
	0x09, 0x65, 0x6e,	//   uint16	0x656e	("en")
	0x09, 0x00, 0x6a,	//   uint16	106	(UTF-8)
	0x09, 0x01, 0x00,	//   uint16	PrimaryLanguageBaseID

	0x09, 0x00, 0x09,	//  uint16	BluetoothProfileDescriptorList
	0x35, 0x08,		//  seq8(8)
	0x35, 0x06,		//   seq8(6)
	0x19, 0x11, 0x0d,	//    uuid16	A2DP
	0x09, 0x01, 0x02,	//    uint16	v1.2

	0x09, 0x01, 0x00,	//  uint16	PrimaryLanguageBaseID +
				//		    ServiceNameOffset
	0x25, 0x09,		//  str8(9)	"Audio SNK"
	'A', 'u', 'd', 'i', 'o', ' ', 'S', 'N', 'K',

	0x09, 0x01, 0x02,	//  uint16	PrimaryLanguageBaseID +
				//		    ProviderName
	0x25, 0x06,		//  str8(6)	"NetBSD"
	'N', 'e', 't', 'B', 'S', 'D',

	0x09, 0x03, 0x11,	//  uint16	SupportedFeatures
	0x09, 0x00, 0x03	//  uint16	Headphone, Speaker
};

static sdp_data_t sink_record =	{ sink_data + 0, sink_data + 103 };
static sdp_data_t sink_psm = { sink_data + 26, sink_data + 29 };

struct l2cap_info {
	bdaddr_t laddr;
	bdaddr_t raddr;
};

__dead static void usage(void);

static int init_server(struct l2cap_info *);
static int init_client(struct l2cap_info *);
static void client_query(void);
static void exit_func(void);

static int sc;		/* avdtp streaming channel */
static int orighc;	/* avdtp listening socket */
static int hc;		/* avdtp control/command channel */
struct event_base *base;
static struct event interrupt_ev;		/* audio event */
static struct event recv_ev;			/* audio sink event */
static struct event ctl_ev;			/* avdtp ctl event */

struct l2cap_info	info;
static bool		asSpeaker;
static bool		initDiscover;	/* initiate avdtp discover */
static bool 		verbose;	/* copy to stdout */
static bool 		test_mode;	/* copy to stdout */
static uint8_t		channel_mode = MODE_STEREO;
static uint8_t		alloc_method = ALLOC_LOUDNESS;
static uint8_t		frequency = FREQ_44_1K;
static uint8_t		freqs[4];
static uint8_t		blocks_config[4];
static uint8_t		channel_config[4];
static uint8_t		bands_config[2];
static uint8_t		alloc_config[2];
static uint8_t		bitpool = 0;
static uint8_t		bands = BANDS_8;
static uint8_t		blocks = BLOCKS_16;
static int		volume = 0;
static int		state = 0;
static uint16_t		l2cap_psm = 0;
static int		l2cap_mode;
uint16_t mtu; 		
uint16_t userset_mtu = 1000, service_class = A2DP_SINK;
static const char* service_type = "A2DP";
struct avdtp_sepInfo mySepInfo;
static sdp_session_t ss;	/* SDP server session */

char *files2open[255];
static int numfiles = 0;
static int startFileInd = 0;
int audfile;

static void do_interrupt(int, short, void *);
static void do_recv(int, short, void *);
static void do_ctlreq(int, short, void *);

#define log_err(x, ...)		if (verbose) { fprintf (stderr, x "\n",\
						__VA_ARGS__); }
#define log_info(x, ...)	if (verbose) { fprintf (stderr, x "\n",\
						__VA_ARGS__); }

int
main(int ac, char *av[])
{
	int enc, i, n, m, l, j, k, o, ch, freqnum, blocksnum;
	u_int tmpbitpool;
	bdaddr_copy(&info.laddr, BDADDR_ANY);

	sc = hc = -1;
	verbose = asSpeaker = test_mode = initDiscover = false;
	n = m = l = i = j = o = 0;
	freqs[0] = frequency;
	channel_config[0] = channel_mode;
	blocks_config[0] = blocks;
	bands_config[0] = bands;
	alloc_config[0] = alloc_method;
	channel_config[0] = channel_mode;

	while ((ch = getopt(ac, av, "A:a:B:b:d:e:f:IKM:m:p:r:tV:v")) != EOF) {
		switch (ch) {
		case 'A':
			for (k = 0; k < (int)strlen(optarg); k++) {
				if (o > 1)
					break;
				switch (optarg[k]) {
				case '0':
					alloc_config[o++] = ALLOC_ANY;
					break;
				case 'S':
					alloc_config[o++] = ALLOC_SNR;
					break;
				case 'L':
					alloc_config[o++] = ALLOC_LOUDNESS;
					break;
				default:
					usage();
				}
			}
			break;
		case 'a':
			if (!bt_aton(optarg, &info.raddr)) {
				struct hostent  *he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					usage();

				bdaddr_copy(&info.raddr,
				    (bdaddr_t *)he->h_addr);
			}
			break;
		case 'B': /* Max bitpool value */
			bitpool = (uint8_t)atoi(optarg);
			break;
		case 'b':
			if (m > 3)
				break;
			blocksnum = atoi(optarg);
			switch (blocksnum) {
			case 0:
				blocks_config[m++] = BLOCKS_ANY;
				break;
			case 4:
				blocks_config[m++] = BLOCKS_4;
				break;
			case 8:
				blocks_config[m++] = BLOCKS_8;
				break;
			case 12:
				blocks_config[m++] = BLOCKS_12;
				break;
			case 16:
				blocks_config[m++] = BLOCKS_16;
				break;
			default:
				usage();
			}
			break;
		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &info.laddr))
				usage();
			break;
		case 'e':
			if (l > 1)
				break;
			enc = atoi(optarg);
			switch (enc) {
			case 0:
				bands_config[l++] = BANDS_ANY;
				break;
			case 4:
				bands_config[l++] = BANDS_4;
				break;
			case 8:
				bands_config[l++] = BANDS_8;
				break;
			default:
				usage();
			}
			break;
		case 'f':
			for (k = 0; k < (int)strlen(optarg); k++) {
				if (n > 3)
					break;
				switch (optarg[k]) {
				case '0':
					channel_config[n++] = MODE_ANY;
					break;
				case '2':
					channel_config[n++] = MODE_DUAL;
					break;
				case 'j':
					channel_config[n++] = MODE_JOINT;
					break;
				case 'm':
					channel_config[n++] = MODE_MONO;
					break;
				case 's':
					channel_config[n++] = MODE_STEREO;
					break;
				default:
					usage();
				}
			}
			break;
		case 'I':
			initDiscover = true;
			break;
		case 'K':
			asSpeaker = true;
			break;
		case 'M':
			userset_mtu = (uint16_t)atoi(optarg);
			break;
		case 'm': /* link mode */
			if (strcasecmp(optarg, "auth") == 0)
				l2cap_mode = L2CAP_LM_AUTH;
			else if (strcasecmp(optarg, "encrypt") == 0)
				l2cap_mode = L2CAP_LM_ENCRYPT;
			else if (strcasecmp(optarg, "secure") == 0)
				l2cap_mode = L2CAP_LM_SECURE;
			else if (strcasecmp(optarg, "none"))
				errx(EXIT_FAILURE, "%s: unknown mode", optarg);

			break;
		case 'p':
			l2cap_psm = (uint16_t)atoi(optarg);
			break;
		case 'r':
			if (i > 3)
				break;
			freqnum = atoi(optarg);
			switch (freqnum) {
			case 0:
				freqs[i++] = FREQ_ANY;
				break;
			case 16000:
				freqs[i++] = FREQ_16K;
				break;
			case 32000:
				freqs[i++] = FREQ_32K;
				break;
			case 44100:
				freqs[i++] = FREQ_44_1K;
				break;
			case 48000:
				freqs[i++] = FREQ_48K;
				break;
			default:
				usage();
			}
			break;
		case 't':
			test_mode = true;
			break;
		case 'V': /* Volume Multiplier */
			volume = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}
	av += optind;
	ac -= optind;

	numfiles = ac;

	for (i = 0; i < numfiles; i++)
		files2open[i] = av[i];

	if (bdaddr_any(&info.raddr) && (!asSpeaker && !test_mode))
		usage();

	if (volume < 0 || volume > 2)
		usage();

	if (!asSpeaker && l2cap_psm)
		usage();

	if (asSpeaker) {
		if (l == 0)
			bands = BANDS_ANY;
		if (m == 0)
			blocks = BLOCKS_ANY;
		if (n == 0)
			channel_mode = MODE_ANY;
		if (o == 0)
			alloc_method = ALLOC_ANY;
	}

	if (i) {
		frequency = 0;
		for (j = 0; j < i; j++)
			frequency |= freqs[j]; 
	}
	if (m) {
		blocks = 0;
		for (j = 0; j < m; j++)
			blocks |= blocks_config[j];
	}
	if (l) {
		bands = 0;
		for (j = 0; j < l; j++)
			bands |= bands_config[j];
	}
	if (n) {
		channel_mode = 0;
		for (j = 0; j < n; j++)
			channel_mode |= channel_config[j];
	}
	if (o) {
		alloc_method = 0;
		for (j = 0; j < o; j++)
			alloc_method |= alloc_config[j];
	}

	if (channel_mode == MODE_MONO || channel_mode == MODE_DUAL)
		tmpbitpool = 16;
	else 
		tmpbitpool = 32;

	if (bands == BANDS_8)
		tmpbitpool *= 8;
	else
		tmpbitpool *= 4;

	if (tmpbitpool > DEFAULT_MAXBPOOL)
		tmpbitpool = DEFAULT_MAXBPOOL;

	if (bitpool == 0 || tmpbitpool < bitpool)
		bitpool = (uint8_t)tmpbitpool;

again:
	base = event_init();

	if (asSpeaker == 0) {
		if (init_client(&info) < 0)
			err(EXIT_FAILURE, "init client");
	} else {
		if (init_server(&info) < 0)
			err(EXIT_FAILURE, "init server");
	}

	if (verbose) {
		fprintf(stderr, "A2DP:\n");
		fprintf(stderr, "\tladdr: %s\n", bt_ntoa(&info.laddr, NULL));
		fprintf(stderr, "\traddr: %s\n", bt_ntoa(&info.raddr, NULL));
	}

	event_base_loop(base, 0);
	event_base_free(base);

	sdp_close(ss);

	if (audfile != -1 && audfile != STDIN_FILENO && audfile !=
	    STDOUT_FILENO) {
		close(audfile);
		audfile = -1;
	}

	if (asSpeaker)
		goto again;

	return EXIT_SUCCESS;
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage:\t%s [-v] [-d device] [-m mode] [-r rate] [-M mtu]\n"
	    "\t\t[-V volume] [-f mode] [-b blocks] [-e bands] [-A alloc]\n"
	    "\t\t[-B bitpool] -a address files...\n"
	    "\t%s [-v] [-d device] [-m mode] [-p psm] [-B bitpool]\n"
	    "\t\t[-a address] [-M mtu] [-r rate] [-I] -K file\n"
	    "\t%s -t [-v] [-K] [-r rate] [-M mtu] [-V volume] [-f mode]\n"
	    "\t\t[-b blocks] [-e bands] [-A alloc] [-B bitpool] files...\n"
	    "Where:\n"
	    "\t-d device    Local device address\n"
	    "\t-a address   Remote device address\n"
	    "\tfiles...     Files to read from (Defaults to stdin/stdout)\n"
	    "\t-v           Verbose output\n"
	    "\t-M mtu       MTU for transmission\n"
	    "\t-B bitpool   Maximum bitpool value for encoding\n"
	    "\t-V volume    Volume multiplier 0,1,2.\n"
	    "\t             WARNING Can be VERY loud\n"
	    "\t-K           Register as audio sink - Listens for incoming\n"
	    "\t             connections.\n" 
	    "\t-I           Initiate DISCOVER - Used with audio sink\n"
	    "\t-m mode     L2CAP link mode\n"
	    "\t-p psm       Listens for incoming connections on psm\n"
	    "\t-t           TEST MODE encodes from file to stdout or -K\n"
	    "\t             read from stdin and decode to file/stdout.\n" 
	    "\t             Test mode does not communicate with bluetooth\n"
	    "\t             only performs SBC de/encoding.\n"
	    "ENCODING OPTIONS:\n"
	    "\t-f mode\n"
	    "\t\t0  All input modes are accepted (only used with -K)\n"
	    "\t\t2  Input is dual channel\n"
	    "\t\tj  Input is in joint stereo\n"
	    "\t\tm  Input is mono\n"
	    "\t\ts  Input is stereo (this is the default)\n"
	    "\t-r rate\n"
	    "\t\t0      All combinations of frequencies are accepted\n"
	    "\t\t16000  Encode with a rate of 16000Hz\n"
	    "\t\t32000  Encode with a rate of 32000Hz\n"
	    "\t\t44100  Encode with a rate of 44100Hz "
	    "(this is the default)\n"
	    "\t\t48000  Encode with a rate of 48000Hz\n"
	    "\t-b blocks\n"
	    "\t\t0   All combinations of blocks are accepted\n"
	    "\t\t4   Encode with 4 blocks\n"
	    "\t\t8   Encode with 8 blocks\n"
	    "\t\t12  Encode with 12 blocks\n"
	    "\t\t16  Encode with 16 blocks (this is the default)\n"
	    "\t-A alloc\n"
	    "\t\t0  All combinations of allocation methods are accepted\n"
	    "\t\tS  Signal to Noise Ratio (SNR) bit allocation\n"
	    "\t\tL  Loudness bit allocation (this is the default)\n"
	    "\n"
	    "\tWithout specifiying any mode rate enoding and allocation\n"
	    "\tthe channel the default is stereo, 16 blocks, 8 subbands,\n"
	    "\tloudness bit allocation, 441000 Hz.\n"
	    , getprogname(), getprogname(), getprogname());

	exit(EXIT_FAILURE);
}

static void
do_ctlreq(int fd, short ev, void *arg)
{
	bool isCommand;
	uint8_t sep, signal;
	static struct sockaddr_bt addr;
	size_t bufflen;
	socklen_t len, mtusize;
	static uint8_t trans;
	uint8_t buff[1024];
	static size_t offset = 0;
	int result;

	if(avdtpCheckResponse(fd, &isCommand, &trans, &signal, NULL, buff,
	    &bufflen, &sep) == ENOMEM) {
		event_del(&ctl_ev);
		close(fd);

		if (asSpeaker) {
			close(orighc);
			orighc = -1;
			event_del(&recv_ev);
		} else
			event_del(&interrupt_ev);

		if (sc != -1)
			close(sc);
		sc = hc = -1;
		state = 0;

		return;
	} else if (isCommand) {
		switch (signal) {
		case AVDTP_ABORT:
		case AVDTP_CLOSE:
			avdtpSendAccept(fd, fd, trans, signal);
			if (asSpeaker) {
				event_del(&recv_ev);
			} else
				event_del(&interrupt_ev);

			if (sc != -1)
				close(sc);
			sc = -1;
			state = 0;
			break;
		case AVDTP_DISCOVER:
			avdtpSendDiscResponseAudio(fd, fd, trans,
			    (asSpeaker ? SNK_SEP : SRC_SEP), asSpeaker);
			state = 1;
			break;
		case AVDTP_GET_CAPABILITIES:
			avdtpSendCapabilitiesResponseSBC(fd, fd, trans,
			    (asSpeaker ? SNK_SEP : SRC_SEP), bitpool,
			    frequency , channel_mode, bands, blocks,
			    alloc_method);
			state = 2;
			break;
		case AVDTP_SET_CONFIGURATION:
			avdtpSendAccept(fd, fd, trans, signal);
			state = 3;
			break;
		case AVDTP_OPEN:
			avdtpSendAccept(fd, fd, trans, signal);
			if (state < 5)
				state = 5;
			break;
		case AVDTP_SUSPEND:
		case AVDTP_START:
			avdtpSendAccept(fd, fd, trans, signal);
			if (state < 6)
				state = 6;
			break;
		default:
			avdtpSendReject(fd, fd, trans, signal);
		}
		if (verbose)
			fprintf(stderr, "Received command %d\n",signal);
	} else {
		switch (signal) {
		case AVDTP_DISCOVER:
			if (offset >= bufflen)
				offset = 0;
			avdtpDiscover(buff + offset, bufflen - offset,
			    &mySepInfo, !asSpeaker);
			avdtpGetCapabilities(fd, fd, mySepInfo.sep);
			break;
		case AVDTP_GET_CAPABILITIES:
			result = avdtpAutoConfigSBC(fd, fd, buff, bufflen,
			    mySepInfo.sep, &frequency, &channel_mode,
			    &alloc_method, &bitpool, &bands, &blocks,
			    (asSpeaker ? SNK_SEP : SRC_SEP));

			if (result == EINVAL) {
				offset += 2;
				avdtpSendCommand(hc, AVDTP_DISCOVER, 0,
				    NULL, 0);
			}

			if (!result && verbose)
				fprintf(stderr, "Bitpool value = %d\n",bitpool);
			state = 3;
			break;
		case AVDTP_SET_CONFIGURATION:
			if (state == 3 && !asSpeaker)
				avdtpOpen(fd, fd, mySepInfo.sep);
			state = 4;
			break;
		case AVDTP_OPEN:
			if (state == 4)
				state = 5;
			break;
		case AVDTP_SUSPEND:
		case AVDTP_START:
			if (state < 6)
				state = 6;
			break;
		default:
			avdtpSendReject(fd, fd, trans, signal);
		}
		if (verbose)
			fprintf(stderr, "Responded to command %d\n",signal);
	}


	if (state < 5 || state > 7)
		return;

	if (asSpeaker && state == 6) {
		len = sizeof(addr);
		if ((sc = accept(orighc,(struct sockaddr*)&addr, &len)) < 0)
			err(EXIT_FAILURE, "stream accept");

	}
	if (state == 6)
		goto opened_connection;

	memset(&addr, 0, sizeof(addr));

	addr.bt_len = sizeof(addr);
	addr.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&addr.bt_bdaddr, &info.laddr);
	addr.bt_psm = l2cap_psm;

	sc = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (sc < 0)
		return;

	if (bind(sc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return;

	if (setsockopt(sc, BTPROTO_L2CAP, SO_L2CAP_LM,
	    &l2cap_mode, sizeof(l2cap_mode)) == -1) {
		log_err("Could not set link mode (0x%4.4x)", l2cap_mode);
		exit(EXIT_FAILURE);
	}

	bdaddr_copy(&addr.bt_bdaddr, &info.raddr);
	if (connect(sc,(struct sockaddr*)&addr, sizeof(addr)) < 0)
		return;

	if (!asSpeaker)
		avdtpStart(fd, fd, mySepInfo.sep); 

	return;

opened_connection:
	if (asSpeaker) {
		event_set(&recv_ev, sc, EV_READ | EV_PERSIST, do_recv, NULL);
		if (event_add(&recv_ev, NULL) < 0)
			err(EXIT_FAILURE, "recv_ev");
		state = 7;
	} else {
		event_set(&interrupt_ev, audfile, EV_READ | EV_PERSIST,
		    do_interrupt, NULL);
		if (event_add(&interrupt_ev, NULL) < 0)
			err(EXIT_FAILURE, "interrupt_ev");
		state = 7;
	}

	mtusize = sizeof(uint16_t);
	getsockopt(sc, BTPROTO_L2CAP, SO_L2CAP_OMTU, &mtu, &mtusize);
	if (userset_mtu != 0 && userset_mtu > 100 && userset_mtu < mtu)
		mtu = userset_mtu;
	else if (userset_mtu == 0 && mtu >= 500)
		mtu /= 2;
}

static void
do_recv(int fd, short ev, void *arg)
{
	ssize_t len;

	len = recvstream(fd, audfile);

	if (verbose)
		fprintf(stderr, "Recving %zd bytes\n",len);

	if (len < 0) {
		event_del(&recv_ev);
		close(fd);
		fd = -1;
		exit(1);
	}
}


static void
do_interrupt(int fd, short ev, void *arg)
{
	static int currentFileInd = 0;
	ssize_t len;

	if (currentFileInd == 0)
		currentFileInd = startFileInd;

	len = stream(fd, sc, channel_mode, frequency, bands, blocks,
	    alloc_method, bitpool, mtu, volume);

	if (len == -1 && currentFileInd >= numfiles -1) {
		event_del(&interrupt_ev);
		close(fd);
		fd = -1;
		exit(1);
	} else if (len == -1) {
		close(fd);
next_file:
		currentFileInd++;
		audfile = open(files2open[currentFileInd], O_RDONLY);
		if (audfile < 0) {
			warn("error opening file %s",
			    files2open[currentFileInd]);
			goto next_file;
		}

		event_del(&interrupt_ev);
		event_set(&interrupt_ev, audfile, EV_READ |
		    EV_PERSIST, do_interrupt, NULL);
		if (event_add(&interrupt_ev, NULL) < 0)
			err(EXIT_FAILURE, "interrupt_ev");
	}

	if (verbose)
		fprintf(stderr, "Streaming %zd bytes\n",len);
}

/*
 * Initialise as an audio sink
 */
static int
init_server(struct l2cap_info *myInfo)
{
	struct sockaddr_bt addr;
	socklen_t len;
	int flags = O_WRONLY | O_CREAT; 
	static bool first_time = true;

	if (atexit(exit_func))
		err(EXIT_FAILURE,"atexit failed to initialize");

	if (numfiles == 0)
		audfile = STDOUT_FILENO;
	else if (numfiles > 1)
		usage();
	else {
		if (first_time)
			flags |= O_TRUNC;
		else
			flags |= O_APPEND;

		first_time = false;

		audfile = open(files2open[0], flags, 0600);
		if (audfile < 0) {
			err(EXIT_FAILURE, "error opening file %s",
			    files2open[0]);
		}
	}
	if (test_mode)
	    goto just_decode;

	if (l2cap_psm == 0)
		l2cap_psm = L2CAP_PSM_AVDTP;

	sdp_set_uint(&sink_psm, l2cap_psm);

	ss = sdp_open_local(NULL);
	if (ss == NULL)
		return -1;

	if (!sdp_record_insert(ss, &myInfo->laddr, NULL, &sink_record)) {
		sdp_close(ss);
		return -1;
	}

	orighc = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (orighc < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.bt_len = sizeof(addr);
	addr.bt_family = AF_BLUETOOTH;
	addr.bt_psm = l2cap_psm;
	bdaddr_copy(&addr.bt_bdaddr, &myInfo->laddr);

	if (bind(orighc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -1;

	if (setsockopt(orighc, BTPROTO_L2CAP, SO_L2CAP_LM,
	    &l2cap_mode, sizeof(l2cap_mode)) == -1) {
		log_err("Could not set link mode (0x%4.4x)", l2cap_mode);
		exit(EXIT_FAILURE);
	}

	bdaddr_copy(&addr.bt_bdaddr, &myInfo->raddr);
	if (listen(orighc,0) < 0)
		return -1;

	len = sizeof(addr);
	if ((hc = accept(orighc,(struct sockaddr*)&addr, &len)) < 0)
		return -1;

	if (initDiscover)
		avdtpSendCommand(hc, AVDTP_DISCOVER, 0, NULL, 0);

	event_set(&ctl_ev, hc, EV_READ | EV_PERSIST, do_ctlreq, NULL);
	if (event_add(&ctl_ev, NULL) < 0)
		err(EXIT_FAILURE, "ctl_ev");

just_decode:
	if (test_mode) {
		if (userset_mtu)
			mtu = userset_mtu;
		sc = STDIN_FILENO;

		event_set(&recv_ev, sc, EV_READ | EV_PERSIST, do_recv, NULL);
		if (event_add(&recv_ev, NULL) < 0)
			err(EXIT_FAILURE, "recv_ev");
	}

	return 0;
}


/*
 * Initialise as an audio source
 */
static int
init_client(struct l2cap_info *myInfo)
{
	struct sockaddr_bt addr;
	int i;

	if (atexit(exit_func))
		err(EXIT_FAILURE,"atexit failed to initialize");

	if (numfiles == 0)
		audfile = STDIN_FILENO;
	else {
		for (i = 0; i < numfiles; i++) {
			audfile = open(files2open[i], O_RDONLY);
			if (audfile < 0)
				warn("error opening file %s",files2open[i]);
			else
				break;
		}
		startFileInd = i;
		if (startFileInd > numfiles - 1)
			errx(EXIT_FAILURE,"error opening file%s",
			    (numfiles > 1) ? "s":"");
	}

	if (test_mode)
		goto just_encode;

	if (l2cap_psm == 0)
		l2cap_psm = L2CAP_PSM_AVDTP;

	sdp_set_uint(&source_psm, l2cap_psm);

	ss = sdp_open_local(NULL);
	if (ss == NULL)
		return -1;

	if (!sdp_record_insert(ss, &myInfo->laddr, NULL, &source_record)) {
		sdp_close(ss);
		return -1;
	}

	client_query();

	orighc = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (orighc < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.bt_len = sizeof(addr);
	addr.bt_family = AF_BLUETOOTH;
	addr.bt_psm = l2cap_psm;
	bdaddr_copy(&addr.bt_bdaddr, &myInfo->laddr);

	if (bind(orighc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -1;

	if (setsockopt(orighc, BTPROTO_L2CAP, SO_L2CAP_LM,
	    &l2cap_mode, sizeof(l2cap_mode)) == -1) {
		log_err("Could not set link mode (0x%4.4x)", l2cap_mode);
		exit(EXIT_FAILURE);
	}

	bdaddr_copy(&addr.bt_bdaddr, &myInfo->raddr);
	if (connect(orighc,(struct sockaddr*)&addr, sizeof(addr)) < 0)
		return -1;

	hc = orighc;
	avdtpSendCommand(hc, AVDTP_DISCOVER, 0, NULL, 0);

	event_set(&ctl_ev, hc, EV_READ | EV_PERSIST, do_ctlreq, NULL);
	if (event_add(&ctl_ev, NULL) < 0)
		err(EXIT_FAILURE, "ctl_ev");

just_encode:
	if (test_mode) {
		if (userset_mtu)
			mtu = userset_mtu;
		sc = STDOUT_FILENO;
		event_set(&interrupt_ev, audfile, EV_READ | EV_PERSIST,
		    do_interrupt, NULL);
		if (event_add(&interrupt_ev, NULL) < 0)
			err(EXIT_FAILURE, "interrupt_ev");
	}

	return 0;
}

static void
client_query(void)
{
	uint8_t buf[12];	/* enough for SSP and AIL both */
	sdp_session_t myss;
	sdp_data_t ssp, ail, rsp, rec, value, pdl, seq;
	uintmax_t psm, ver;
	uint16_t attr;
	bool rv;

	myss = sdp_open(&info.laddr, &info.raddr);
	if (myss == NULL) {
		log_err("%s: Could not open sdp session", service_type);
		exit(EXIT_FAILURE);
	}

	log_info("Searching for %s service at %s",
	    service_type, bt_ntoa(&info.raddr, NULL));

	seq.next = buf;
	seq.end = buf + sizeof(buf);

	/*
	 * build ServiceSearchPattern (9 bytes)
	 *
	 *	uuid16	"service_class"
	 *	uuid16	L2CAP
	 *	uuid16	AVDTP
	 */
	ssp.next = seq.next;
	sdp_put_uuid16(&seq, service_class);
	sdp_put_uuid16(&seq, SDP_UUID_PROTOCOL_L2CAP);
	sdp_put_uuid16(&seq, SDP_UUID_PROTOCOL_AVDTP);
	ssp.end = seq.next;

	/*
	 * build AttributeIDList (3 bytes)
	 *
	 *	uint16	ProtocolDescriptorList
	 */
	ail.next = seq.next;
	sdp_put_uint16(&seq, SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST);
	ail.end = seq.next;

	rv = sdp_service_search_attribute(myss, &ssp, &ail, &rsp);
	if (!rv) {
		log_err("%s: Required sdp record not found", service_type);
		exit(EXIT_FAILURE);
	}

	/*
	 * we expect the response to contain a list of records
	 * containing a ProtocolDescriptorList. Find the first
	 * one containing L2CAP and AVDTP >= 1.0, and extract
	 * the PSM.
	 */
	rv = false;
	while (!rv && sdp_get_seq(&rsp, &rec)) {
		if (!sdp_get_attr(&rec, &attr, &value)
		    || attr != SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST)
			continue;

		sdp_get_alt(&value, &value);	/* drop any alt header */
		while (!rv && sdp_get_seq(&value, &pdl)) {
			if (sdp_get_seq(&pdl, &seq) &&
			    sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_L2CAP) &&
			    sdp_get_uint(&seq, &psm) &&
			    sdp_get_seq(&pdl, &seq) &&
			    sdp_match_uuid16(&seq, SDP_UUID_PROTOCOL_AVDTP) &&
			    sdp_get_uint(&seq, &ver) && ver >= 0x0100)
				rv = true;
		}
	}

	sdp_close(myss);

	if (!rv) {
		log_err("%s query failed", service_type);
		exit(EXIT_FAILURE);
	}

	l2cap_psm = (uint16_t)psm;
	log_info("Found PSM %u for service %s", l2cap_psm, service_type);
}

static void
exit_func(void)
{
	avdtpAbort(hc, hc, mySepInfo.sep); 
	avdtpClose(hc, hc, mySepInfo.sep); 
	close(sc);
	close(hc);
	sc = hc = -1;
}

