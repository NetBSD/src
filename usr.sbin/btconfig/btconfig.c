/* $NetBSD: btconfig.c,v 1.15 2009/08/24 20:43:35 plunky Exp $ */

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
__RCSID("$NetBSD: btconfig.c,v 1.15 2009/08/24 20:43:35 plunky Exp $");

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

/* inquiry results storage */
struct result {
	bdaddr_t	bdaddr;
	uint8_t		page_scan_rep_mode;
	uint8_t		uclass[HCI_CLASS_SIZE];
	uint16_t	clock_offset;
	int8_t		rssi;
};

int main(int, char *[]);
void badarg(const char *);
void badparam(const char *);
void badval(const char *, const char *);
void usage(void);
int set_unit(unsigned long);
void config_unit(void);
void print_val(const char *, const char **, int);
void print_info(int);
void print_stats(void);
void print_class(const char *);
void print_voice(int);
void tag(const char *);
void print_features(const char *, uint8_t *);
void do_inquiry(void);
void print_result(int, struct result *, int);

void hci_req(uint16_t, uint8_t , void *, size_t, void *, size_t);
#define save_value(opcode, cbuf, clen)	hci_req(opcode, 0, cbuf, clen, NULL, 0)
#define load_value(opcode, rbuf, rlen)	hci_req(opcode, 0, NULL, 0, rbuf, rlen)
#define hci_cmd(opcode, cbuf, clen)	hci_req(opcode, 0, cbuf, clen, NULL, 0)

#define MAX_STR_SIZE	0xff

/* print width */
int width = 0;
#define MAX_WIDTH	70

/* global variables */
int hci;
struct btreq btr;

/* command line flags */
int verbose = 0;	/* more info */
int lflag = 0;		/* list devices */
int sflag = 0;		/* get/zero stats */

/* device up/down (flag) */
int opt_enable = 0;
int opt_reset = 0;
#define FLAGS_FMT	"\20"			\
			"\001UP"		\
			"\002RUNNING"		\
			"\003XMIT_CMD"		\
			"\004XMIT_ACL"		\
			"\005XMIT_SCO"		\
			"\006INIT_BDADDR"	\
			"\007INIT_BUFFER_SIZE"	\
			"\010INIT_FEATURES"	\
			"\011POWER_UP_NOOP"	\
			"\012INIT_COMMANDS"	\
			"\013MASTER"		\
			""

/* authorisation (flag) */
int opt_auth = 0;

/* encryption (flag) */
int opt_encrypt = 0;

/* scan enable options (flags) */
int opt_pscan = 0;
int opt_iscan = 0;

/* master role option */
int opt_master = 0;

/* link policy options (flags) */
int opt_switch = 0;
int opt_hold = 0;
int opt_sniff = 0;
int opt_park = 0;

/* class of device (hex value) */
int opt_class = 0;
uint32_t class;

/* packet type mask (hex value) */
int opt_ptype = 0;
uint32_t ptype;

/* unit name (string) */
int opt_name = 0;
char name[MAX_STR_SIZE];

/* pin type */
int opt_pin = 0;

/* Inquiry */
int opt_rssi = 0;			/* inquiry_with_rssi (obsolete flag) */
int opt_imode = 0;			/* inquiry mode */
int opt_inquiry = 0;
#define INQUIRY_LENGTH		10	/* about 12 seconds */
#define INQUIRY_MAX_RESPONSES	10
const char *imodes[] = { "std", "rssi", "ext", NULL };

/* Voice Settings */
int opt_voice = 0;
uint32_t voice;

/* Page Timeout */
int opt_pto = 0;
uint32_t pto;

/* set SCO mtu */
int opt_scomtu;
uint32_t scomtu;

struct parameter {
	const char	*name;
	enum { P_SET, P_CLR, P_STR, P_HEX, P_NUM, P_VAL } type;
	int		*opt;
	void		*val;
} parameters[] = {
	{ "up",		P_SET,	&opt_enable,	NULL	},
	{ "enable",	P_SET,	&opt_enable,	NULL	},
	{ "down",	P_CLR,	&opt_enable,	NULL	},
	{ "disable",	P_CLR,	&opt_enable,	NULL	},
	{ "name",	P_STR,	&opt_name,	name	},
	{ "pscan",	P_SET,	&opt_pscan,	NULL	},
	{ "-pscan",	P_CLR,	&opt_pscan,	NULL	},
	{ "iscan",	P_SET,	&opt_iscan,	NULL	},
	{ "-iscan",	P_CLR,	&opt_iscan,	NULL	},
	{ "master",	P_SET,	&opt_master,	NULL	},
	{ "-master",	P_CLR,	&opt_master,	NULL	},
	{ "switch",	P_SET,	&opt_switch,	NULL	},
	{ "-switch",	P_CLR,	&opt_switch,	NULL	},
	{ "hold",	P_SET,	&opt_hold,	NULL	},
	{ "-hold",	P_CLR,	&opt_hold,	NULL	},
	{ "sniff",	P_SET,	&opt_sniff,	NULL	},
	{ "-sniff",	P_CLR,	&opt_sniff,	NULL	},
	{ "park",	P_SET,	&opt_park,	NULL	},
	{ "-park",	P_CLR,	&opt_park,	NULL	},
	{ "auth",	P_SET,	&opt_auth,	NULL	},
	{ "-auth",	P_CLR,	&opt_auth,	NULL	},
	{ "encrypt",	P_SET,	&opt_encrypt,	NULL	},
	{ "-encrypt",	P_CLR,	&opt_encrypt,	NULL	},
	{ "ptype",	P_HEX,	&opt_ptype,	&ptype	},
	{ "class",	P_HEX,	&opt_class,	&class	},
	{ "fixed",	P_SET,	&opt_pin,	NULL	},
	{ "variable",	P_CLR,	&opt_pin,	NULL	},
	{ "inq",	P_SET,	&opt_inquiry,	NULL	},
	{ "inquiry",	P_SET,	&opt_inquiry,	NULL	},
	{ "imode",	P_VAL,	&opt_imode,	imodes	},
	{ "rssi",	P_SET,	&opt_rssi,	NULL	},
	{ "-rssi",	P_CLR,	&opt_rssi,	NULL	},
	{ "reset",	P_SET,	&opt_reset,	NULL	},
	{ "voice",	P_HEX,	&opt_voice,	&voice	},
	{ "pto",	P_NUM,	&opt_pto,	&pto	},
	{ "scomtu",	P_NUM,	&opt_scomtu,	&scomtu	},
	{ NULL,		0,	NULL,		NULL	}
};

int
main(int ac, char *av[])
{
	int ch;
	struct parameter *p;

	while ((ch = getopt(ac, av, "hlsvz")) != -1) {
		switch(ch) {
		case 'l':
			lflag = 1;
			break;

		case 's':
			sflag = 1;
			break;

		case 'v':
			verbose++;
			break;

		case 'z':
			sflag = 2;
			break;

		case 'h':
		default:
			usage();
		}
	}
	av += optind;
	ac -= optind;

	if (lflag && sflag)
		usage();

	hci = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (hci == -1)
		err(EXIT_FAILURE, "socket");

	if (ac == 0) {
		verbose++;

		memset(&btr, 0, sizeof(btr));
		while (set_unit(SIOCNBTINFO) != -1) {
			print_info(verbose);
			print_stats();
		}

		tag(NULL);
	} else {
		strlcpy(btr.btr_name, *av, HCI_DEVNAME_SIZE);
		av++, ac--;

		if (set_unit(SIOCGBTINFO) < 0)
			err(EXIT_FAILURE, "%s", btr.btr_name);

		if (ac == 0)
			verbose += 2;

		while (ac > 0) {
			for (p = parameters ; ; p++) {
				if (p->name == NULL)
					badparam(*av);

				if (strcmp(*av, p->name) == 0)
					break;
			}

			switch(p->type) {
			case P_SET:
				*(p->opt) = 1;
				break;

			case P_CLR:
				*(p->opt) = -1;
				break;

			case P_STR:
				if (--ac < 1) badarg(p->name);
				strlcpy((char *)(p->val), *++av, MAX_STR_SIZE);
				*(p->opt) = 1;
				break;

			case P_HEX:
				if (--ac < 1) badarg(p->name);
				*(uint32_t *)(p->val) = strtoul(*++av, NULL, 16);
				*(p->opt) = 1;
				break;

			case P_NUM:
				if (--ac < 1) badarg(p->name);
				*(uint32_t *)(p->val) = strtoul(*++av, NULL, 10);
				*(p->opt) = 1;
				break;

			case P_VAL:
				if (--ac < 1) badarg(p->name);
				++av;
				ch = 0;
				do {
					if (((char **)(p->val))[ch] == NULL)
						badval(p->name, *av);
				} while (strcmp(((char **)(p->val))[ch++], *av));
				*(p->opt) = ch;
				break;
			}

			av++, ac--;
		}

		config_unit();
		print_info(verbose);
		print_stats();
		do_inquiry();
	}

	close(hci);
	return EXIT_SUCCESS;
}

void
badparam(const char *param)
{

	fprintf(stderr, "unknown parameter '%s'\n", param);
	exit(EXIT_FAILURE);
}

void
badarg(const char *param)
{

	fprintf(stderr, "parameter '%s' needs argument\n", param);
	exit(EXIT_FAILURE);
}

void
badval(const char *param, const char *value)
{

	fprintf(stderr, "bad value '%s' for parameter '%s'\n", value, param);
	exit(EXIT_FAILURE);
}

void
usage(void)
{

	fprintf(stderr, "usage:	%s [-svz] [device [parameters]]\n", getprogname());
	fprintf(stderr, "	%s -l\n", getprogname());
	exit(EXIT_FAILURE);
}

/*
 * pretty printing feature
 */
void
tag(const char *f)
{

	if (f == NULL) {
		if (width > 0)
			printf("\n");

		width = 0;
	} else {
		width += printf("%*s%s",
				(width == 0 ? (lflag ? 0 : 8) : 1),
				"", f);

		if (width > MAX_WIDTH) {
			printf("\n");
			width = 0;
		}
	}
}

/*
 * basic HCI cmd request function with argument return.
 *
 * Normally, this will return on COMMAND_STATUS or COMMAND_COMPLETE for the given
 * opcode, but if event is given then it will ignore COMMAND_STATUS (unless error)
 * and wait for the specified event.
 *
 * if rbuf/rlen is given, results will be copied into the result buffer for
 * COMMAND_COMPLETE/event responses.
 */
void
hci_req(uint16_t opcode, uint8_t event, void *cbuf, size_t clen, void *rbuf, size_t rlen)
{
	uint8_t msg[sizeof(hci_cmd_hdr_t) + HCI_CMD_PKT_SIZE];
	hci_event_hdr_t *ep;
	hci_cmd_hdr_t *cp;

	cp = (hci_cmd_hdr_t *)msg;
	cp->type = HCI_CMD_PKT;
	cp->opcode = opcode = htole16(opcode);
	cp->length = clen = MIN(clen, sizeof(msg) - sizeof(hci_cmd_hdr_t));

	if (clen) memcpy((cp + 1), cbuf, clen);

	if (send(hci, msg, sizeof(hci_cmd_hdr_t) + clen, 0) < 0)
		err(EXIT_FAILURE, "HCI Send");

	ep = (hci_event_hdr_t *)msg;
	for(;;) {
		if (recv(hci, msg, sizeof(msg), 0) < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;

			err(EXIT_FAILURE, "HCI Recv");
		}

		if (ep->event == HCI_EVENT_COMMAND_STATUS) {
			hci_command_status_ep *cs;

			cs = (hci_command_status_ep *)(ep + 1);
			if (cs->opcode != opcode)
				continue;

			if (cs->status)
				errx(EXIT_FAILURE,
				    "HCI cmd (%4.4x) failed (status %d)",
				    opcode, cs->status);

			if (event == 0)
				break;

			continue;
		}

		if (ep->event == HCI_EVENT_COMMAND_COMPL) {
			hci_command_compl_ep *cc;
			uint8_t *ptr;

			cc = (hci_command_compl_ep *)(ep + 1);
			if (cc->opcode != opcode)
				continue;

			if (rbuf == NULL)
				break;

			ptr = (uint8_t *)(cc + 1);
			if (*ptr)
				errx(EXIT_FAILURE,
				    "HCI cmd (%4.4x) failed (status %d)",
				    opcode, *ptr);

			memcpy(rbuf, ++ptr, rlen);
			break;
		}

		if (ep->event == event) {
			if (rbuf == NULL)
				break;

			memcpy(rbuf, (ep + 1), rlen);
			break;
		}
	}
}

int
set_unit(unsigned long cmd)
{

	if (ioctl(hci, cmd, &btr) == -1)
		return -1;

	if (btr.btr_flags & BTF_UP) {
		struct sockaddr_bt sa;

		sa.bt_len = sizeof(sa);
		sa.bt_family = AF_BLUETOOTH;
		bdaddr_copy(&sa.bt_bdaddr, &btr.btr_bdaddr);

		if (bind(hci, (struct sockaddr *)&sa, sizeof(sa)) < 0)
			err(EXIT_FAILURE, "bind");

		if (connect(hci, (struct sockaddr *)&sa, sizeof(sa)) < 0)
			err(EXIT_FAILURE, "connect");
	}

	return 0;
}

/*
 * apply configuration parameters to unit
 */
void
config_unit(void)
{

	if (opt_enable) {
		if (opt_enable > 0)
			btr.btr_flags |= BTF_UP;
		else
			btr.btr_flags &= ~BTF_UP;

		if (ioctl(hci, SIOCSBTFLAGS, &btr) < 0)
			err(EXIT_FAILURE, "SIOCSBTFLAGS");

		if (set_unit(SIOCGBTINFO) < 0)
			err(EXIT_FAILURE, "%s", btr.btr_name);
	}

	if (opt_reset) {
		hci_cmd(HCI_CMD_RESET, NULL, 0);

		btr.btr_flags |= BTF_INIT;
		if (ioctl(hci, SIOCSBTFLAGS, &btr) < 0)
			err(EXIT_FAILURE, "SIOCSBTFLAGS");

		/*
		 * although the reset command will automatically
		 * carry out these commands, we do them manually
		 * just so we can wait for completion.
		 */
		hci_cmd(HCI_CMD_READ_BDADDR, NULL, 0);
		hci_cmd(HCI_CMD_READ_BUFFER_SIZE, NULL, 0);
		hci_cmd(HCI_CMD_READ_LOCAL_FEATURES, NULL, 0);

		if (set_unit(SIOCGBTINFO) < 0)
			err(EXIT_FAILURE, "%s", btr.btr_name);
	}

	if (opt_master) {
		if (opt_master > 0)
			btr.btr_flags |= BTF_MASTER;
		else
			btr.btr_flags &= ~BTF_MASTER;

		if (ioctl(hci, SIOCSBTFLAGS, &btr) < 0)
			err(EXIT_FAILURE, "SIOCSBTFLAGS");
	}

	if (opt_switch || opt_hold || opt_sniff || opt_park) {
		uint16_t val = btr.btr_link_policy;

		if (opt_switch > 0) val |= HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
		if (opt_switch < 0) val &= ~HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
		if (opt_hold > 0)   val |= HCI_LINK_POLICY_ENABLE_HOLD_MODE;
		if (opt_hold < 0)   val &= ~HCI_LINK_POLICY_ENABLE_HOLD_MODE;
		if (opt_sniff > 0)  val |= HCI_LINK_POLICY_ENABLE_SNIFF_MODE;
		if (opt_sniff < 0)  val &= ~HCI_LINK_POLICY_ENABLE_SNIFF_MODE;
		if (opt_park > 0)   val |= HCI_LINK_POLICY_ENABLE_PARK_MODE;
		if (opt_park < 0)   val &= ~HCI_LINK_POLICY_ENABLE_PARK_MODE;

		btr.btr_link_policy = val;
		if (ioctl(hci, SIOCSBTPOLICY, &btr) < 0)
			err(EXIT_FAILURE, "SIOCSBTPOLICY");
	}

	if (opt_ptype) {
		btr.btr_packet_type = ptype;
		if (ioctl(hci, SIOCSBTPTYPE, &btr) < 0)
			err(EXIT_FAILURE, "SIOCSBTPTYPE");
	}

	if (opt_pscan || opt_iscan) {
		uint8_t val;

		load_value(HCI_CMD_READ_SCAN_ENABLE, &val, sizeof(val));
		if (opt_pscan > 0) val |= HCI_PAGE_SCAN_ENABLE;
		if (opt_pscan < 0) val &= ~HCI_PAGE_SCAN_ENABLE;
		if (opt_iscan > 0) val |= HCI_INQUIRY_SCAN_ENABLE;
		if (opt_iscan < 0) val &= ~HCI_INQUIRY_SCAN_ENABLE;
		save_value(HCI_CMD_WRITE_SCAN_ENABLE, &val, sizeof(val));
	}

	if (opt_auth) {
		uint8_t val = (opt_auth > 0 ? 1 : 0);

		save_value(HCI_CMD_WRITE_AUTH_ENABLE, &val, sizeof(val));
	}

	if (opt_encrypt) {
		uint8_t val = (opt_encrypt > 0 ? 1 : 0);

		save_value(HCI_CMD_WRITE_ENCRYPTION_MODE, &val, sizeof(val));
	}

	if (opt_name)
		save_value(HCI_CMD_WRITE_LOCAL_NAME, name, HCI_UNIT_NAME_SIZE);

	if (opt_class) {
		uint8_t val[HCI_CLASS_SIZE];

		val[0] = (class >> 0) & 0xff;
		val[1] = (class >> 8) & 0xff;
		val[2] = (class >> 16) & 0xff;

		save_value(HCI_CMD_WRITE_UNIT_CLASS, val, HCI_CLASS_SIZE);
	}

	if (opt_pin) {
		uint8_t val;

		if (opt_pin > 0)	val = 1;
		else			val = 0;

		save_value(HCI_CMD_WRITE_PIN_TYPE, &val, sizeof(val));
	}

	if (opt_voice) {
		uint16_t val;

		val = htole16(voice & 0x03ff);
		save_value(HCI_CMD_WRITE_VOICE_SETTING, &val, sizeof(val));
	}

	if (opt_pto) {
		uint16_t val;

		val = htole16(pto * 8 / 5);
		save_value(HCI_CMD_WRITE_PAGE_TIMEOUT, &val, sizeof(val));
	}

	if (opt_scomtu) {
		if (scomtu > 0xff) {
			warnx("Invalid SCO mtu %d", scomtu);
		} else {
			btr.btr_sco_mtu = scomtu;

			if (ioctl(hci, SIOCSBTSCOMTU, &btr) < 0)
				warn("SIOCSBTSCOMTU");
		}
	}

	if (opt_imode | opt_rssi) {
		uint8_t val = (opt_rssi > 0 ? 1 : 0);

		if (opt_imode)
			val = opt_imode - 1;

		save_value(HCI_CMD_WRITE_INQUIRY_MODE, &val, sizeof(val));
	}
}

/*
 * print value from NULL terminated array given index
 */
void
print_val(const char *hdr, const char **argv, int idx)
{
	int i = 0;

	while (i < idx && *argv != NULL)
		i++, argv++;

	printf("\t%s: %s\n", hdr, *argv == NULL ? "unknown" : *argv);
}

/*
 * Print info for Bluetooth Device with varying verbosity levels
 */
void
print_info(int level)
{
	uint8_t version, val, buf[MAX_STR_SIZE];
	uint16_t val16;

	if (lflag) {
		tag(btr.btr_name);
		return;
	}

	if (level-- < 1)
		return;

	snprintb((char *)buf, MAX_STR_SIZE, FLAGS_FMT, btr.btr_flags);

	printf("%s: bdaddr %s flags %s\n", btr.btr_name,
		bt_ntoa(&btr.btr_bdaddr, NULL), buf);

	if (level-- < 1)
		return;

	printf("\tnum_cmd = %d\n"
	       "\tnum_acl = %d, acl_mtu = %d\n"
	       "\tnum_sco = %d, sco_mtu = %d\n",
	       btr.btr_num_cmd,
	       btr.btr_num_acl, btr.btr_acl_mtu,
	       btr.btr_num_sco, btr.btr_sco_mtu);

	if (level-- < 1 || (btr.btr_flags & BTF_UP) == 0)
		return;

	load_value(HCI_CMD_READ_LOCAL_VER, &version, sizeof(version));
	printf("\tHCI version: ");
	switch(version) {
	case HCI_SPEC_V10:	printf("1.0\n");	break;
	case HCI_SPEC_V11:	printf("1.0b\n");	break;
	case HCI_SPEC_V12:	printf("1.2\n");	break;
	case HCI_SPEC_V20:	printf("2.0\n");	break;
	case HCI_SPEC_V21:	printf("2.1\n");	break;
	default:		printf("unknown\n");	break;
	}

	load_value(HCI_CMD_READ_UNIT_CLASS, buf, HCI_CLASS_SIZE);
	class = (buf[2] << 16) | (buf[1] << 8) | (buf[0]);
	print_class("\t");

	load_value(HCI_CMD_READ_LOCAL_NAME, buf, HCI_UNIT_NAME_SIZE);
	printf("\tname: \"%s\"\n", buf);

	load_value(HCI_CMD_READ_VOICE_SETTING, buf, sizeof(uint16_t));
	voice = (buf[1] << 8) | buf[0];
	print_voice(level);

	load_value(HCI_CMD_READ_PIN_TYPE, &val, sizeof(val));
	printf("\tpin: %s\n", val ? "fixed" : "variable");

	val = 0;
	if (version >= HCI_SPEC_V12)
		load_value(HCI_CMD_READ_INQUIRY_MODE, &val, sizeof(val));

	print_val("inquiry mode", imodes, val);

	width = printf("\toptions:");

	load_value(HCI_CMD_READ_SCAN_ENABLE, &val, sizeof(val));
	if (val & HCI_INQUIRY_SCAN_ENABLE)	tag("iscan");
	else if (level > 0)			tag("-iscan");

	if (val & HCI_PAGE_SCAN_ENABLE)		tag("pscan");
	else if (level > 0)			tag("-pscan");

	load_value(HCI_CMD_READ_AUTH_ENABLE, &val, sizeof(val));
	if (val)				tag("auth");
	else if (level > 0)			tag("-auth");

	load_value(HCI_CMD_READ_ENCRYPTION_MODE, &val, sizeof(val));
	if (val)				tag("encrypt");
	else if (level > 0)			tag("-encrypt");

	val = btr.btr_link_policy;
	if (val & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)	tag("switch");
	else if (level > 0)				tag("-switch");
	if (val & HCI_LINK_POLICY_ENABLE_HOLD_MODE)	tag("hold");
	else if (level > 0)				tag("-hold");
	if (val & HCI_LINK_POLICY_ENABLE_SNIFF_MODE)	tag("sniff");
	else if (level > 0)				tag("-sniff");
	if (val & HCI_LINK_POLICY_ENABLE_PARK_MODE)	tag("park");
	else if (level > 0)				tag("-park");

	tag(NULL);

	if (level-- < 1)
		return;

	ptype = btr.btr_packet_type;
	width = printf("\tptype: [0x%04x]", ptype);
	if (ptype & HCI_PKT_DM1)		tag("DM1");
	if (ptype & HCI_PKT_DH1)		tag("DH1");
	if (ptype & HCI_PKT_DM3)		tag("DM3");
	if (ptype & HCI_PKT_DH3)		tag("DH3");
	if (ptype & HCI_PKT_DM5)		tag("DM5");
	if (ptype & HCI_PKT_DH5)		tag("DH5");
	if ((ptype & HCI_PKT_2MBPS_DH1) == 0)	tag("2-DH1");
	if ((ptype & HCI_PKT_3MBPS_DH1) == 0)	tag("3-DH1");
	if ((ptype & HCI_PKT_2MBPS_DH3) == 0)	tag("2-DH3");
	if ((ptype & HCI_PKT_3MBPS_DH3) == 0)	tag("3-DH3");
	if ((ptype & HCI_PKT_2MBPS_DH5) == 0)	tag("2-DH5");
	if ((ptype & HCI_PKT_3MBPS_DH5) == 0)	tag("3-DH5");
	tag(NULL);

	load_value(HCI_CMD_READ_PAGE_TIMEOUT, &val16, sizeof(val16));
	printf("\tpage timeout: %d ms\n", val16 * 5 / 8);

	if (level-- < 1)
		return;

	load_value(HCI_CMD_READ_LOCAL_FEATURES, buf, HCI_FEATURES_SIZE);
	print_features("\tfeatures:", buf);
}

void
print_stats(void)
{

	if (sflag == 0)
		return;

	if (sflag == 1) {
		if (ioctl(hci, SIOCGBTSTATS, &btr) < 0)
			err(EXIT_FAILURE, "SIOCGBTSTATS");
	} else  {
		if (ioctl(hci, SIOCZBTSTATS, &btr) < 0)
			err(EXIT_FAILURE, "SIOCZBTSTATS");
	}

	printf( "\tTotal bytes sent %d, recieved %d\n"
		"\tCommands sent %d, Events received %d\n"
		"\tACL data packets sent %d, received %d\n"
		"\tSCO data packets sent %d, received %d\n"
		"\tInput errors %d, Output errors %d\n",
		btr.btr_stats.byte_tx, btr.btr_stats.byte_rx,
		btr.btr_stats.cmd_tx, btr.btr_stats.evt_rx,
		btr.btr_stats.acl_tx, btr.btr_stats.acl_rx,
		btr.btr_stats.sco_tx, btr.btr_stats.sco_rx,
		btr.btr_stats.err_rx, btr.btr_stats.err_tx);
}

void
print_features(const char *str, uint8_t *f)
{

	width = printf("%s", str);

	/* ------------------- byte 0 --------------------*/
	if (*f & HCI_LMP_3SLOT)		    tag("<3 slot>");
	if (*f & HCI_LMP_5SLOT)		    tag("<5 slot>");
	if (*f & HCI_LMP_ENCRYPTION)	    tag("<encryption>");
	if (*f & HCI_LMP_SLOT_OFFSET)	    tag("<slot offset>");
	if (*f & HCI_LMP_TIMIACCURACY)	    tag("<timing accuracy>");
	if (*f & HCI_LMP_ROLE_SWITCH)	    tag("<role switch>");
	if (*f & HCI_LMP_HOLD_MODE)	    tag("<hold mode>");
	if (*f & HCI_LMP_SNIFF_MODE)	    tag("<sniff mode>");
	f++;

	/* ------------------- byte 1 --------------------*/
	if (*f & HCI_LMP_PARK_MODE)	    tag("<park mode>");
	if (*f & HCI_LMP_RSSI)		    tag("<RSSI>");
	if (*f & HCI_LMP_CHANNEL_QUALITY)   tag("<channel quality>");
	if (*f & HCI_LMP_SCO_LINK)	    tag("<SCO link>");
	if (*f & HCI_LMP_HV2_PKT)	    tag("<HV2>");
	if (*f & HCI_LMP_HV3_PKT)	    tag("<HV3>");
	if (*f & HCI_LMP_ULAW_LOG)	    tag("<u-Law log>");
	if (*f & HCI_LMP_ALAW_LOG)	    tag("<A-Law log>");
	f++;

	/* ------------------- byte 1 --------------------*/
	if (*f & HCI_LMP_CVSD)		    tag("<CVSD data>");
	if (*f & HCI_LMP_PAGISCHEME)	    tag("<paging parameter>");
	if (*f & HCI_LMP_POWER_CONTROL)	    tag("<power control>");
	if (*f & HCI_LMP_TRANSPARENT_SCO)   tag("<transparent SCO>");
	if (*f & HCI_LMP_FLOW_CONTROL_LAG0) tag("<flow control lag 0>");
	if (*f & HCI_LMP_FLOW_CONTROL_LAG1) tag("<flow control lag 1>");
	if (*f & HCI_LMP_FLOW_CONTROL_LAG2) tag("<flow control lag 2>");
	if (*f & HCI_LMP_BC_ENCRYPTION)	    tag("<broadcast encryption>");
	f++;

	/* ------------------- byte 3 --------------------*/
	if (*f & HCI_LMP_EDR_ACL_2MBPS)	    tag("<EDR ACL 2Mbps>");
	if (*f & HCI_LMP_EDR_ACL_3MBPS)	    tag("<EDR ACL 3Mbps>");
	if (*f & HCI_LMP_ENHANCED_ISCAN)    tag("<enhanced inquiry scan>");
	if (*f & HCI_LMP_INTERLACED_ISCAN)  tag("<interlaced inquiry scan>");
	if (*f & HCI_LMP_INTERLACED_PSCAN)  tag("<interlaced page scan>");
	if (*f & HCI_LMP_RSSI_INQUIRY)	    tag("<RSSI with inquiry result>");
	if (*f & HCI_LMP_EV3_PKT)	    tag("<EV3 packets>");
	f++;

	/* ------------------- byte 4 --------------------*/
	if (*f & HCI_LMP_EV4_PKT)	    tag("<EV4 packets>");
	if (*f & HCI_LMP_EV5_PKT)	    tag("<EV5 packets>");
	if (*f & HCI_LMP_AFH_CAPABLE_SLAVE) tag("<AFH capable slave>");
	if (*f & HCI_LMP_AFH_CLASS_SLAVE)   tag("<AFH class slave>");
	if (*f & HCI_LMP_3SLOT_EDR_ACL)	    tag("<3 slot EDR ACL>");
	f++;

	/* ------------------- byte 5 --------------------*/
	if (*f & HCI_LMP_5SLOT_EDR_ACL)	    tag("<5 slot EDR ACL>");
	if (*f & HCI_LMP_SNIFF_SUBRATING)   tag("<sniff subrating>");
	if (*f & HCI_LMP_PAUSE_ENCRYPTION)  tag("<pause encryption>");
	if (*f & HCI_LMP_AFH_CAPABLE_MASTER)tag("<AFH capable master>");
	if (*f & HCI_LMP_AFH_CLASS_MASTER)  tag("<AFH class master>");
	if (*f & HCI_LMP_EDR_eSCO_2MBPS)    tag("<EDR eSCO 2Mbps>");
	if (*f & HCI_LMP_EDR_eSCO_3MBPS)    tag("<EDR eSCO 3Mbps>");
	if (*f & HCI_LMP_3SLOT_EDR_eSCO)    tag("<3 slot EDR eSCO>");
	f++;

	/* ------------------- byte 6 --------------------*/
	if (*f & HCI_LMP_EXTENDED_INQUIRY)  tag("<extended inquiry>");
	if (*f & HCI_LMP_SIMPLE_PAIRING)    tag("<secure simple pairing>");
	if (*f & HCI_LMP_ENCAPSULATED_PDU)  tag("<encapsulated PDU>");
	if (*f & HCI_LMP_ERRDATA_REPORTING) tag("<errdata reporting>");
	if (*f & HCI_LMP_NOFLUSH_PB_FLAG)   tag("<no flush PB flag>");
	f++;

	/* ------------------- byte 7 --------------------*/
	if (*f & HCI_LMP_LINK_SUPERVISION_TO)tag("<link supervision timeout changed>");
	if (*f & HCI_LMP_INQ_RSP_TX_POWER)  tag("<inquiry rsp TX power level>");
	if (*f & HCI_LMP_EXTENDED_FEATURES) tag("<extended features>");

	tag(NULL);
}

void
print_class(const char *str)
{
	int major, minor;

	major = (class & 0x1f00) >> 8;
	minor = (class & 0x00fc) >> 2;

	width = printf("%sclass: [0x%6.6x]", str, class);

	switch (major) {
	case 1:	/* Computer */
		switch (minor) {
		case 1: tag("Desktop");				break;
		case 2: tag("Server");				break;
		case 3: tag("Laptop");				break;
		case 4: tag("Handheld");			break;
		case 5: tag("Palm Sized");			break;
		case 6: tag("Wearable");			break;
		}
		tag("Computer");
		break;

	case 2:	/* Phone */
		switch (minor) {
		case 1: tag("Cellular Phone");			break;
		case 2: tag("Cordless Phone");			break;
		case 3: tag("Smart Phone");			break;
		case 4: tag("Wired Modem/Phone Gateway");	break;
		case 5: tag("Common ISDN");			break;
		default:tag("Phone");				break;
		}
		break;

	case 3:	/* LAN */
		tag("LAN");
		switch ((minor & 0x38) >> 3) {
		case 0: tag("[Fully available]");		break;
		case 1: tag("[1-17% utilised]");		break;
		case 2: tag("[17-33% utilised]");		break;
		case 3: tag("[33-50% utilised]");		break;
		case 4: tag("[50-67% utilised]");		break;
		case 5: tag("[67-83% utilised]");		break;
		case 6: tag("[83-99% utilised]");		break;
		case 7: tag("[No service available]");		break;
		}
		break;

	case 4:	/* Audio/Visual */
		switch (minor) {
		case 1: tag("Wearable Headset");		break;
		case 2: tag("Hands-free Audio");		break;
		case 4: tag("Microphone");			break;
		case 5: tag("Loudspeaker");			break;
		case 6: tag("Headphones");			break;
		case 7: tag("Portable Audio");			break;
		case 8: tag("Car Audio");			break;
		case 9: tag("Set-top Box");			break;
		case 10: tag("HiFi Audio");			break;
		case 11: tag("VCR");				break;
		case 12: tag("Video Camera");			break;
		case 13: tag("Camcorder");			break;
		case 14: tag("Video Monitor");			break;
		case 15: tag("Video Display and Loudspeaker");	break;
		case 16: tag("Video Conferencing");		break;
		case 18: tag("A/V [Gaming/Toy]");		break;
		default: tag("Audio/Visual");			break;
		}
		break;

	case 5:	/* Peripheral */
		switch (minor & 0x0f) {
		case 1: tag("Joystick");			break;
		case 2: tag("Gamepad");				break;
		case 3: tag("Remote Control");			break;
		case 4: tag("Sensing Device");			break;
		case 5: tag("Digitiser Tablet");		break;
		case 6: tag("Card Reader");			break;
		default: tag("Peripheral");			break;
		}

		if (minor & 0x10) tag("Keyboard");
		if (minor & 0x20) tag("Mouse");
		break;

	case 6:	/* Imaging */
		if (minor & 0x20) tag("Printer");
		if (minor & 0x10) tag("Scanner");
		if (minor & 0x08) tag("Camera");
		if (minor & 0x04) tag("Display");
		if ((minor & 0x3c) == 0) tag("Imaging");
		break;

	case 7:	/* Wearable */
		switch (minor) {
		case 1: tag("Wrist Watch");			break;
		case 2: tag("Pager");				break;
		case 3: tag("Jacket");				break;
		case 4: tag("Helmet");				break;
		case 5: tag("Glasses");				break;
		default: tag("Wearable");			break;
		}
		break;

	case 8:	/* Toy */
		switch (minor) {
		case 1: tag("Robot");				break;
		case 2: tag("Vehicle");				break;
		case 3: tag("Doll / Action Figure");		break;
		case 4: tag("Controller");			break;
		case 5: tag("Game");				break;
		default: tag("Toy");				break;
		}
		break;

	default:
		break;
	}

	if (class & 0x002000)	tag("<Limited Discoverable>");
	if (class & 0x010000)	tag("<Positioning>");
	if (class & 0x020000)	tag("<Networking>");
	if (class & 0x040000)	tag("<Rendering>");
	if (class & 0x080000)	tag("<Capturing>");
	if (class & 0x100000)	tag("<Object Transfer>");
	if (class & 0x200000)	tag("<Audio>");
	if (class & 0x400000)	tag("<Telephony>");
	if (class & 0x800000)	tag("<Information>");
	tag(NULL);
}

void
print_voice(int level)
{
	printf("\tvoice: [0x%4.4x]\n", voice);

	if (level == 0)
		return;

	printf("\t\tInput Coding: ");
	switch ((voice & 0x0300) >> 8) {
	case 0x00:	printf("Linear PCM [%d-bit, pos %d]",
			(voice & 0x0020 ? 16 : 8),
			(voice & 0x001c) >> 2);		break;
	case 0x01:	printf("u-Law");		break;
	case 0x02:	printf("A-Law");		break;
	case 0x03:	printf("unknown");		break;
	}

	switch ((voice & 0x00c0) >> 6) {
	case 0x00:	printf(", 1's complement");	break;
	case 0x01:	printf(", 2's complement");	break;
	case 0x02:	printf(", sign magnitude");	break;
	case 0x03:	printf(", unsigned");		break;
	}

	printf("\n\t\tAir Coding: ");
	switch (voice & 0x0003) {
	case 0x00:	printf("CVSD");			break;
	case 0x01:	printf("u-Law");		break;
	case 0x02:	printf("A-Law");		break;
	case 0x03:	printf("Transparent");		break;
	}

	printf("\n");
}

void
print_result(int num, struct result *r, int rssi)
{
	hci_remote_name_req_cp ncp;
	hci_remote_name_req_compl_ep nep;
	struct hostent *hp;

	printf("%3d: bdaddr %s",
			num,
			bt_ntoa(&r->bdaddr, NULL));

	hp = bt_gethostbyaddr((const char *)&r->bdaddr, sizeof(bdaddr_t), AF_BLUETOOTH);
	if (hp != NULL)
		printf(" (%s)", hp->h_name);

	printf("\n");

	memset(&ncp, 0, sizeof(ncp));
	bdaddr_copy(&ncp.bdaddr, &r->bdaddr);
	ncp.page_scan_rep_mode = r->page_scan_rep_mode;
	ncp.clock_offset = r->clock_offset;

	hci_req(HCI_CMD_REMOTE_NAME_REQ,
		HCI_EVENT_REMOTE_NAME_REQ_COMPL,
		&ncp, sizeof(ncp),
		&nep, sizeof(nep));

	printf("   : name \"%s\"\n", nep.name);

	class = (r->uclass[2] << 16) | (r->uclass[1] << 8) | (r->uclass[0]);
	print_class("   : ");

	printf("   : page scan rep mode 0x%02x\n", r->page_scan_rep_mode);
	printf("   : clock offset %d\n", le16toh(r->clock_offset));

	if (rssi)
		printf("   : rssi %d\n", r->rssi);

	printf("\n");
}

void
do_inquiry(void)
{
	uint8_t buf[HCI_EVENT_PKT_SIZE];
	struct result result[INQUIRY_MAX_RESPONSES];
	hci_inquiry_cp inq;
	struct hci_filter f;
	hci_event_hdr_t *hh;
	int i, j, num, rssi;

	if (opt_inquiry == 0)
		return;

	printf("Device Discovery from device: %s ...", btr.btr_name);
	fflush(stdout);

	memset(&f, 0, sizeof(f));
	hci_filter_set(HCI_EVENT_COMMAND_STATUS, &f);
	hci_filter_set(HCI_EVENT_COMMAND_COMPL, &f);
	hci_filter_set(HCI_EVENT_INQUIRY_RESULT, &f);
	hci_filter_set(HCI_EVENT_RSSI_RESULT, &f);
	hci_filter_set(HCI_EVENT_INQUIRY_COMPL, &f);
	hci_filter_set(HCI_EVENT_REMOTE_NAME_REQ_COMPL, &f);
	hci_filter_set(HCI_EVENT_READ_REMOTE_FEATURES_COMPL, &f);
	if (setsockopt(hci, BTPROTO_HCI, SO_HCI_EVT_FILTER, &f, sizeof(f)) < 0)
		err(EXIT_FAILURE, "Can't set event filter");

	/* General Inquiry LAP is 0x9e8b33 */
	inq.lap[0] = 0x33;
	inq.lap[1] = 0x8b;
	inq.lap[2] = 0x9e;
	inq.inquiry_length = INQUIRY_LENGTH;
	inq.num_responses = INQUIRY_MAX_RESPONSES;

	hci_cmd(HCI_CMD_INQUIRY, &inq, sizeof(inq));

	num = 0;
	rssi = 0;
	hh = (hci_event_hdr_t *)buf;

	for (;;) {
		if (recv(hci, buf, sizeof(buf), 0) <= 0)
			err(EXIT_FAILURE, "recv");

		if (hh->event == HCI_EVENT_INQUIRY_COMPL)
			break;

		if (hh->event == HCI_EVENT_INQUIRY_RESULT) {
			hci_inquiry_result_ep *ep = (hci_inquiry_result_ep *)(hh + 1);
			hci_inquiry_response *ir = (hci_inquiry_response *)(ep + 1);

			for (i = 0 ; i < ep->num_responses ; i++) {
				if (num == INQUIRY_MAX_RESPONSES)
					break;

				/* some devices keep responding, ignore dupes */
				for (j = 0 ; j < num ; j++)
					if (bdaddr_same(&result[j].bdaddr, &ir[i].bdaddr))
						break;

				if (j < num)
					continue;

				bdaddr_copy(&result[num].bdaddr, &ir[i].bdaddr);
				memcpy(&result[num].uclass, &ir[i].uclass, HCI_CLASS_SIZE);
				result[num].page_scan_rep_mode = ir[i].page_scan_rep_mode;
				result[num].clock_offset = ir[i].clock_offset;
				result[num].rssi = 0;
				num++;
				printf(".");
				fflush(stdout);
			}
			continue;
		}

		if (hh->event == HCI_EVENT_RSSI_RESULT) {
			hci_rssi_result_ep *ep = (hci_rssi_result_ep *)(hh + 1);
			hci_rssi_response *rr = (hci_rssi_response *)(ep + 1);

			for (i = 0 ; i < ep->num_responses ; i++) {
				if (num == INQUIRY_MAX_RESPONSES)
					break;

				/* some devices keep responding, ignore dupes */
				for (j = 0 ; j < num ; j++)
					if (bdaddr_same(&result[j].bdaddr, &rr[i].bdaddr))
						break;

				if (j < num)
					continue;

				bdaddr_copy(&result[num].bdaddr, &rr[i].bdaddr);
				memcpy(&result[num].uclass, &rr[i].uclass, HCI_CLASS_SIZE);
				result[num].page_scan_rep_mode = rr[i].page_scan_rep_mode;
				result[num].clock_offset = rr[i].clock_offset;
				result[num].rssi = rr[i].rssi;
				rssi = 1;
				num++;
				printf(".");
				fflush(stdout);
			}
			continue;
		}
	}

	printf(" %d response%s\n", num, (num == 1 ? "" : "s"));

	for (i = 0 ; i < num ; i++)
		print_result(i + 1, &result[i], rssi);
}
