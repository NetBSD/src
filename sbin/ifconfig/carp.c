/* $NetBSD: carp.c,v 1.5 2008/05/06 04:33:42 dyoung Exp $ */

/*
 * Copyright (c) 2002 Michael Shalayeff. All rights reserved.
 * Copyright (c) 2003 Ryan McBride. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <netinet/ip_carp.h>
#include <net/route.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <util.h>

#include "env.h"
#include "parse.h"
#include "extern.h"
#include "carp.h"

static const char *carp_states[] = { CARP_STATES };

#ifndef INET_ONLY
struct kwinst carpstatekw[] = {
	  {.k_word = "INIT", .k_nextparser = &command_root.pb_parser}
	, {.k_word = "BACKUP", .k_nextparser = &command_root.pb_parser}
	, {.k_word = "MASTER", .k_nextparser = &command_root.pb_parser}
};

struct pinteger parse_advbase = PINTEGER_INITIALIZER1(&parse_advbase, "advbase",
    0, 255, 10, setcarp_advbase, "advbase", &command_root.pb_parser);

struct pinteger parse_advskew = PINTEGER_INITIALIZER1(&parse_advskew, "advskew",
    0, 254, 10, setcarp_advskew, "advskew", &command_root.pb_parser);

struct piface carpdev = PIFACE_INITIALIZER(&carpdev, "carpdev", setcarpdev,
    "carpdev", &command_root.pb_parser);

struct pkw carpstate = PKW_INITIALIZER(&carpstate, "carp state", NULL,
    "carp_state", carpstatekw, __arraycount(carpstatekw),
    &command_root.pb_parser);

struct pstr pass = PSTR_INITIALIZER(&pass, "pass", setcarp_passwd,
    "pass", &command_root.pb_parser);

struct pinteger parse_vhid = PINTEGER_INITIALIZER1(&vhid, "vhid",
    0, 255, 10, setcarp_vhid, "vhid", &command_root.pb_parser);
#endif

void
carp_status(prop_dictionary_t env)
{
	const char *state;
	struct ifreq ifr;
	struct carpreq carpr;
	int s;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		return;

	if (carpr.carpr_vhid <= 0)
		return;
	if (carpr.carpr_state > CARP_MAXSTATE)
		state = "<UNKNOWN>";
	else
		state = carp_states[carpr.carpr_state];

	printf("\tcarp: %s carpdev %s vhid %d advbase %d advskew %d\n",
	    state, carpr.carpr_carpdev[0] != '\0' ?
	    carpr.carpr_carpdev : "none", carpr.carpr_vhid,
	    carpr.carpr_advbase, carpr.carpr_advskew);
}

int
setcarp_passwd(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct carpreq carpr;
	struct ifreq ifr;
	int s;
	prop_data_t data;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	data = (prop_data_t)prop_dictionary_get(env, "pass");
	if (data == NULL) {
		errno = ENOENT;
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;

	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	memset(carpr.carpr_key, 0, sizeof(carpr.carpr_key));
	/* XXX Should hash the password into the key here, perhaps? */
	strlcpy((char *)carpr.carpr_key, prop_data_data_nocopy(data),
	    MIN(CARP_KEY_LEN, prop_data_size(data)));

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}

int
setcarp_vhid(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct ifreq ifr;
	struct carpreq carpr;
	int vhid;
	int s;
	prop_number_t num;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	num = (prop_number_t)prop_dictionary_get(env, "vhid");
	if (num == NULL) {
		errno = ENOENT;
		return -1;
	}

	vhid = (int)prop_number_integer_value(num);

	memset(&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = &carpr;

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_vhid = vhid;

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}

int
setcarp_advskew(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct ifreq ifr;
	struct carpreq carpr;
	int advskew;
	int s;
	prop_number_t num;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	num = (prop_number_t)prop_dictionary_get(env, "advskew");
	if (num == NULL) {
		errno = ENOENT;
		return -1;
	}

	advskew = (int)prop_number_integer_value(num);

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_advskew = advskew;

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}

/* ARGSUSED */
int
setcarp_advbase(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct carpreq carpr;
	int advbase;
	int s;
	prop_number_t num;
	struct ifreq ifr;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	num = (prop_number_t)prop_dictionary_get(env, "advbase");
	if (num == NULL) {
		errno = ENOENT;
		return -1;
	}

	advbase = (int)prop_number_integer_value(num);

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_advbase = advbase;

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}

/* ARGSUSED */
int
setcarp_state(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct carpreq carpr;
	int s;
	prop_number_t num;
	struct ifreq ifr;
	const char *ifname;

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	num = (prop_number_t)prop_dictionary_get(env, "carp_state");
	if (num == NULL) {
		errno = ENOENT;
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_state = (int)prop_number_integer_value(num);

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}

/* ARGSUSED */
int
setcarpdev(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct carpreq carpr;
	int s;
	prop_data_t data;
	struct ifreq ifr;
	const char *ifname;

	data = (prop_data_t)prop_dictionary_get(env, "carpdev");
	if (data == NULL) {
		errno = ENOENT;
		return -1;
	}

	if ((s = getsock(AF_UNSPEC)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	memset(&ifr, 0, sizeof(ifr));
	memset(&carpr, 0, sizeof(carpr));
	ifr.ifr_data = &carpr;
	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	strlcpy(carpr.carpr_carpdev, prop_data_data_nocopy(data),
	    MIN(sizeof(carpr.carpr_carpdev), prop_data_size(data)));

	if (ioctl(s, SIOCSVH, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
	return 0;
}
