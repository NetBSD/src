/* $NetBSD: carp.c,v 1.1 2006/05/18 09:05:50 liamjfoy Exp $ */

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

#include "extern.h"

void	carp_status(void);
void	setcarp_advbase(const char *,int);
void	setcarp_advskew(const char *, int);
void	setcarp_passwd(const char *, int);
void	setcarp_vhid(const char *, int);
void	setcarp_state(const char *, int);
void	setcarpdev(const char *, int);
void	unsetcarpdev(const char *, int);

static const char *carp_states[] = { CARP_STATES };

void
carp_status(void)
{
	const char *state;
	struct carpreq carpr;

	memset((char *)&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		return;

	if (carpr.carpr_vhid > 0) {
		if (carpr.carpr_state > CARP_MAXSTATE)
			state = "<UNKNOWN>";
		else
			state = carp_states[carpr.carpr_state];

		printf("\tcarp: %s carpdev %s vhid %d advbase %d advskew %d\n",
		    state, carpr.carpr_carpdev[0] != '\0' ?
		    carpr.carpr_carpdev : "none", carpr.carpr_vhid,
		    carpr.carpr_advbase, carpr.carpr_advskew);
	}
}

/* ARGSUSED */
void
setcarp_passwd(const char *val, int d)
{
	struct carpreq carpr;

	memset((char *)&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	/* XXX Should hash the password into the key here, perhaps? */
	strlcpy((char *)carpr.carpr_key, val, CARP_KEY_LEN);

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

/* ARGSUSED */
void
setcarp_vhid(const char *val, int d)
{
	struct carpreq carpr;
	long tmp;
	char *ep;
	int vhid;

	errno = 0;
	tmp = strtol(val, &ep, 10);

	if (*ep != '\0' || tmp < 0 || tmp > 255
	    || errno == ERANGE)
		errx(EXIT_FAILURE, "vhid: %s: must be between 0 and 255",
		    val);

	vhid = (int)tmp;

	memset((char *)&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_vhid = vhid;

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

/* ARGSUSED */
void
setcarp_advskew(const char *val, int d)
{
	struct carpreq carpr;
	long tmp;
	char *ep;
	int advskew;

	errno = 0;
	tmp = strtol(val, &ep, 10);

	if (*ep != '\0' || tmp < 0 || tmp > 254
	    || errno == ERANGE)
		errx(EXIT_FAILURE, "advskew: %s: must be between 0 and 254",
		    val);

	advskew = (int)tmp;

	memset((char *)&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_advskew = advskew;

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

/* ARGSUSED */
void
setcarp_advbase(const char *val, int d)
{
	struct carpreq carpr;
	long tmp;
	char *ep;
	int advbase;

	errno = 0;
	tmp = strtol(val, &ep, 10);

	if (*ep != '\0' || tmp < 0 || tmp > 255
	    || errno == ERANGE)
		errx(EXIT_FAILURE, "advbase: %s: must be between 0 and 255",
		    val);

	advbase = (int)tmp;

	memset((char *)&carpr, 0, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	carpr.carpr_advbase = advbase;

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

/* ARGSUSED */
void
setcarp_state(const char *val, int d)
{
	struct carpreq carpr;
	int i;

	bzero((char *)&carpr, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	for (i = 0; i <= CARP_MAXSTATE; i++) {
		if (!strcasecmp(val, carp_states[i])) {
			carpr.carpr_state = i;
			break;
		}
	}

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

/* ARGSUSED */
void
setcarpdev(const char *val, int d)
{
	struct carpreq carpr;

	bzero((char *)&carpr, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	strlcpy(carpr.carpr_carpdev, val, sizeof(carpr.carpr_carpdev));

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}

void
unsetcarpdev(const char *val, int d)
{
	struct carpreq carpr;

	bzero((char *)&carpr, sizeof(struct carpreq));
	ifr.ifr_data = (caddr_t)&carpr;

	if (ioctl(s, SIOCGVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCGVH");

	bzero((char *)&carpr.carpr_carpdev, sizeof(carpr.carpr_carpdev));

	if (ioctl(s, SIOCSVH, (caddr_t)&ifr) == -1)
		err(EXIT_FAILURE, "SIOCSVH");
}
