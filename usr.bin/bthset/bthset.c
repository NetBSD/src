/*	$NetBSD: bthset.c,v 1.4.6.1 2009/05/13 19:19:44 jym Exp $	*/

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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2006 Itronix, Inc.  All rights reserved.");
__RCSID("$NetBSD: bthset.c,v 1.4.6.1 2009/05/13 19:19:44 jym Exp $");

#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <assert.h>
#include <bluetooth.h>
#include <err.h>
#include <event.h>
#include <fcntl.h>
#include <sdp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/btsco.h>

#include <netbt/rfcomm.h>

#define RING_INTERVAL	5	/* seconds */

int  main(int, char *[]);
void usage(void);

void do_signal(int, short, void *);
void do_ring(int, short, void *);
void do_mixer(int, short, void *);
void do_rfcomm(int, short, void *);
void do_server(int, short, void *);
int send_rfcomm(const char *, ...);

int init_mixer(struct btsco_info *, const char *);
int init_rfcomm(struct btsco_info *);
int init_server(struct btsco_info *, int);

void remove_pid(void);
int write_pid(void);

struct event sigint_ev;		/* bye bye */
struct event sigusr1_ev;	/* start ringing */
struct event sigusr2_ev;	/* stop ringing */
struct event mixer_ev;		/* mixer changed */
struct event rfcomm_ev;		/* headset speaks */
struct event server_ev;		/* headset connecting */
struct event ring_ev;		/* ring timer */

mixer_ctrl_t vgs;	/* speaker control */
mixer_ctrl_t vgm;	/* mic control */
int ringing;		/* we are ringing */
int verbose;		/* copy to stdout */
int mx;			/* mixer fd */
int rf;			/* rfcomm connection fd */
int ag;			/* rfcomm gateway fd */
sdp_session_t ss;	/* SDP server session */

char *command;		/* answer command */
char *pidfile;		/* PID file name */

/* Headset Audio Gateway service record */
uint8_t hset_data[] = {
	0x09, 0x00, 0x00,	//  uint16	ServiceRecordHandle
	0x0a, 0x00, 0x00, 0x00,	//  uint32	0x00000000
	0x00,

	0x09, 0x00, 0x01,	//  uint16	ServiceClassIDList
	0x35, 0x06,		//  seq8(6)
	0x19, 0x11, 0x12,	//   uuid16	HeadsetAudioGateway
	0x19, 0x12, 0x03,	//   uuid16	GenericAudio

	0x09, 0x00, 0x04,	//  uint16	ProtocolDescriptorList
	0x35, 0x0c,		//  seq8(12)
	0x35, 0x03,		//   seq8(3)
	0x19, 0x01, 0x00,	//    uuid16	L2CAP
	0x35, 0x05,		//   seq8(5)
	0x19, 0x00, 0x03,	//    uuid16	RFCOMM
	0x08, 0x00,		//    uint8	%hset_channel%

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
	0x19, 0x11, 0x08,	//    uuid16	Headset
	0x09, 0x01, 0x00,	//    uint16	v1.0

	0x09, 0x01, 0x00,	//  uint16	PrimaryLanguageBaseID + ServiceNameOffset
	0x25, 0x0d, 0x56, 0x6f,	//  str8(13)	"Voice Gateway"
	0x69, 0x63, 0x65, 0x20,
	0x47, 0x61, 0x74, 0x65,
	0x77, 0x61, 0x79
};

sdp_data_t hset_record =	{ hset_data + 0, hset_data + 91 };
sdp_data_t hset_channel =	{ hset_data + 36, hset_data + 37 };

int
main(int ac, char *av[])
{
	struct btsco_info	info;
	const char		*mixer;
	int			ch, channel;

	ag = rf = -1;
	verbose = 0;
	channel = 0;
	pidfile = getenv("BTHSET_PIDFILE");
	command = getenv("BTHSET_COMMAND");
	mixer = getenv("BTHSET_MIXER");
	if (mixer == NULL)
		mixer = "/dev/mixer";

	while ((ch = getopt(ac, av, "hc:m:p:s:v")) != EOF) {
		switch (ch) {
		case 'c':
			command = optarg;
			break;

		case 'm':
			mixer = optarg;
			break;

		case 'p':
			pidfile = optarg;
			break;

		case 's':
			channel = atoi(optarg);
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
		default:
			usage();
		}
	}

	if (mixer == NULL)
		usage();

	if ((channel < RFCOMM_CHANNEL_MIN || channel > RFCOMM_CHANNEL_MAX)
	    && channel != 0)
		usage();

	if (write_pid() < 0)
		err(EXIT_FAILURE, "%s", pidfile);

	event_init();

	ringing = 0;
	evtimer_set(&ring_ev, do_ring, NULL);

	signal_set(&sigusr1_ev, SIGUSR1, do_signal, NULL);
	if (signal_add(&sigusr1_ev, NULL) < 0)
		err(EXIT_FAILURE, "SIGUSR1");

	signal_set(&sigusr2_ev, SIGUSR2, do_signal, NULL);
	if (signal_add(&sigusr2_ev, NULL) < 0)
		err(EXIT_FAILURE, "SIGUSR2");

	signal_set(&sigint_ev, SIGINT, do_signal, NULL);
	if (signal_add(&sigint_ev, NULL) < 0)
		err(EXIT_FAILURE, "SIGINT");

	if (init_mixer(&info, mixer) < 0)
		err(EXIT_FAILURE, "%s", mixer);

	if (channel == 0 && init_rfcomm(&info) < 0)
		err(EXIT_FAILURE, "%s", bt_ntoa(&info.raddr, NULL));

	if (channel && init_server(&info, channel) < 0)
		err(EXIT_FAILURE, "%d", channel);

	if (verbose) {
		printf("Headset Info:\n");
		printf("\tmixer: %s\n", mixer);
		printf("\tladdr: %s\n", bt_ntoa(&info.laddr, NULL));
		printf("\traddr: %s\n", bt_ntoa(&info.raddr, NULL));
		printf("\tchannel: %d\n", info.channel);
		printf("\tvgs.dev: %d, vgm.dev: %d\n", vgs.dev, vgm.dev);
		if (channel) printf("\tserver channel: %d\n", channel);
	}

	event_dispatch();

	err(EXIT_FAILURE, "event_dispatch");
}

void
usage(void)
{

	fprintf(stderr,
		"usage: %s [-hv] [-c command] [-m mixer] [-p file] [-s channel]\n"
		"Where:\n"
		"\t-h           display this message\n"
		"\t-v           verbose output\n"
		"\t-c command   command to execute on answer\n"
		"\t-m mixer     mixer path\n"
		"\t-p file      write PID to file\n"
		"\t-s channel   register as audio gateway on channel\n"
		"", getprogname());

	exit(EXIT_FAILURE);
}

void
do_signal(int s, short ev, void *arg)
{

	switch (s) {
	case SIGUSR1:
		ringing = 1;	/* start ringing */
		do_ring(0, 0, NULL);
		break;

	case SIGUSR2:
		ringing = 0;
		break;

	case SIGINT:
	default:
		exit(EXIT_SUCCESS);
	}
}

void
do_ring(int s, short ev, void *arg)
{
	static struct timeval tv = { RING_INTERVAL, 0 };

	if (!ringing)
		return;

	send_rfcomm("RING");
	evtimer_add(&ring_ev, &tv);
}

/*
 * The mixer device has been twiddled. We check mic and speaker
 * settings and send the appropriate commands to the headset,
 */
void
do_mixer(int s, short ev, void *arg)
{
	mixer_ctrl_t mc;
	int level;

	memcpy(&mc, &vgs, sizeof(mc));
	if (ioctl(mx, AUDIO_MIXER_READ, &mc) < 0)
		return;

	if (memcmp(&vgs, &mc, sizeof(mc))) {
		memcpy(&vgs, &mc, sizeof(mc));
		level = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] / BTSCO_DELTA;

		send_rfcomm("+VGS=%d", level);
	}

	memcpy(&mc, &vgm, sizeof(mc));
	if (ioctl(mx, AUDIO_MIXER_READ, &mc) < 0)
		return;

	if (memcmp(&vgm, &mc, sizeof(mc))) {
		memcpy(&vgm, &mc, sizeof(mc));
		level = mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] / BTSCO_DELTA;

		send_rfcomm("+VGM=%d", level);
	}
}

/*
 * RFCOMM socket event.
 */
void
do_rfcomm(int fd, short ev, void *arg)
{
	char buf[128];
	int len, level;

	memset(buf, 0, sizeof(buf));
	len = recv(rf, buf, sizeof(buf), 0);
	if (len <= 0) {
		if (ag < 0)
			errx(EXIT_FAILURE, "Connection Lost");

		event_del(&rfcomm_ev);
		close(rf);
		rf = -1;
		ringing = 0;
		return;
	}

	if (verbose)
		printf("> %.*s\n", len, buf);

	if (len >= 7 && strncmp(buf, "AT+CKPD", 7) == 0) {
		if (ringing && command != NULL) {
			if (verbose)
				printf("%% %s\n", command);

			system(command);
		}

		ringing = 0;
		send_rfcomm("OK");
		return;
	}

	if (len >= 7 && strncmp(buf, "AT+VGS=", 7) == 0) {
		level = atoi(buf + 7);
		if (level < 0 || level > 15)
			return;

		vgs.un.value.level[AUDIO_MIXER_LEVEL_MONO] = level * BTSCO_DELTA;
		if (ioctl(mx, AUDIO_MIXER_WRITE, &vgs) < 0)
			return;

		send_rfcomm("OK");
		return;
	}

	if (len >= 7 && strncmp(buf, "AT+VGM=", 7) == 0) {
		level = atoi(buf + 7);
		if (level < 0 || level > 15)
			return;

		vgm.un.value.level[AUDIO_MIXER_LEVEL_MONO] = level * BTSCO_DELTA;
		if (ioctl(mx, AUDIO_MIXER_WRITE, &vgm) < 0)
			return;

		send_rfcomm("OK");
		return;
	}

	send_rfcomm("ERROR");
}

/*
 * got an incoming connection on the AG socket.
 */
void
do_server(int fd, short ev, void *arg)
{
	bdaddr_t *raddr = arg;
	struct sockaddr_bt addr;
	socklen_t len;
	int s;

	assert(raddr != NULL);

	len = sizeof(addr);
	s = accept(fd, (struct sockaddr *)&addr, &len);
	if (s < 0)
		return;

	if (rf >= 0
	    || len != sizeof(addr)
	    || addr.bt_len != sizeof(addr)
	    || addr.bt_family != AF_BLUETOOTH
	    || !bdaddr_same(raddr, &addr.bt_bdaddr)) {
		close(s);
		return;
	}

	rf = s;
	event_set(&rfcomm_ev, rf, EV_READ | EV_PERSIST, do_rfcomm, NULL);
	if (event_add(&rfcomm_ev, NULL) < 0)
		err(EXIT_FAILURE, "rfcomm_ev");
}

/*
 * send a message to the RFCOMM socket
 */
int
send_rfcomm(const char *msg, ...)
{
	char buf[128], fmt[128];
	va_list ap;
	int len;

	va_start(ap, msg);

	if (verbose) {
		snprintf(fmt, sizeof(fmt), "< %s\n", msg);
		vprintf(fmt, ap);
	}

	snprintf(fmt, sizeof(fmt), "\r\n%s\r\n", msg);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	len = send(rf, buf, strlen(buf), 0);

	va_end(ap);
	return len;
}

/*
 * Initialise mixer event
 */
int
init_mixer(struct btsco_info *info, const char *mixer)
{

	mx = open(mixer, O_WRONLY, 0);
	if (mx < 0)
		return -1;

	if (ioctl(mx, BTSCO_GETINFO, info) < 0)
		return -1;

	/* get initial vol settings */
	memset(&vgs, 0, sizeof(vgs));
	vgs.dev = info->vgs;
	if (ioctl(mx, AUDIO_MIXER_READ, &vgs) < 0)
		return -1;

	memset(&vgm, 0, sizeof(vgm));
	vgm.dev = info->vgm;
	if (ioctl(mx, AUDIO_MIXER_READ, &vgm) < 0)
		return -1;

	/* set up mixer changed event */
	if (fcntl(mx, F_SETFL, O_ASYNC) < 0)
		return -1;

	signal_set(&mixer_ev, SIGIO, do_mixer, NULL);
	if (signal_add(&mixer_ev, NULL) < 0)
		return -1;

	return 0;
}

/*
 * Initialise RFCOMM socket
 */
int
init_rfcomm(struct btsco_info *info)
{
	struct sockaddr_bt addr;

	rf = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (rf < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.bt_len = sizeof(addr);
	addr.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&addr.bt_bdaddr, &info->laddr);

	if (bind(rf, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -1;

	bdaddr_copy(&addr.bt_bdaddr, &info->raddr);
	addr.bt_channel = info->channel;

	if (connect(rf, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -1;

	event_set(&rfcomm_ev, rf, EV_READ | EV_PERSIST, do_rfcomm, NULL);
	if (event_add(&rfcomm_ev, NULL) < 0)
		return -1;

	return 0;
}

/*
 * Initialise server socket
 */
int
init_server(struct btsco_info *info, int channel)
{
	struct sockaddr_bt addr;

	ag = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (ag < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.bt_len = sizeof(addr);
	addr.bt_family = AF_BLUETOOTH;
	addr.bt_channel = channel;
	bdaddr_copy(&addr.bt_bdaddr, &info->laddr);

	if (bind(ag, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return -1;

	if (listen(ag, 1) < 0)
		return -1;

	event_set(&server_ev, ag, EV_READ | EV_PERSIST, do_server, &info->raddr);
	if (event_add(&server_ev, NULL) < 0)
		return -1;

	sdp_set_uint(&hset_channel, channel);

	ss = sdp_open_local(NULL);
	if (ss == NULL)
		return -1;

	if (!sdp_record_insert(ss, &info->laddr, NULL, &hset_record)) {
		sdp_close(ss);
		return -1;
	}

	return 0;
}

void
remove_pid(void)
{

	if (pidfile == NULL)
		return;

	unlink(pidfile);
}

int
write_pid(void)
{
	char *buf;
	int fd, len;

	if (pidfile == NULL)
		return 0;

	fd = open(pidfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return -1;

	len = asprintf(&buf, "%d\n", getpid());
	if (len > 0)
		write(fd, buf, len);

	if (len >= 0 && buf != NULL)
		free(buf);

	close (fd);

	return atexit(remove_pid);
}
