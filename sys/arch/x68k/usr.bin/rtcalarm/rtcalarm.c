/*	$NetBSD: rtcalarm.c,v 1.10 2009/11/28 02:56:14 isaki Exp $	*/
/*
 * Copyright (c) 1995 MINOURA Makoto.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Minoura Makoto.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* rtcalarm: X68k RTC alarm test program */

#ifndef __GNUC__
#define volatile
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <machine/powioctl.h>

static void usage(void) __attribute__((__noreturn__));
static void showinfo(void);
static char *numstr(unsigned int);
static void showontime(unsigned int);
static void disablealarm(void);
static void setinfo(int, char **);
static int strnum(const char *, int);

static char *devicefile = "/dev/pow0";	/* default path */

int
main(int argc, char *argv[])
{

	if (argc == 1)
		showinfo();
	else if (argc == 2 && strcmp(argv[1], "off") == 0)
		disablealarm();
	else
		setinfo(argc, argv);

	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
		"Usage: %s [[-w day-of-the-week] [-d day-of-the-month]\n" 
		"                [-m minites] [-s seconds] [-c channel] HH:MM]\n",
		getprogname());

	exit(1);
}

static void
showinfo(void)
{
	struct x68k_alarminfo alarminfo;
	int             fd;

	fd = open(devicefile, O_RDONLY);
	if (fd < 0)
		err("Opening %s", devicefile);

	if (ioctl(fd, POWIOCGALARMINFO, &alarminfo) < 0)
		err("POWIOCGALARMINFO");
	close(fd);

	if (alarminfo.al_enable) {
		showontime(alarminfo.al_ontime);
		if (alarminfo.al_dowhat == 0 || alarminfo.al_dowhat == -1)
			printf("Computer mode.\n");
		else if (alarminfo.al_dowhat >= 0x30 && alarminfo.al_dowhat <= 0x3b)
			printf("TV channel %d.\n", alarminfo.al_dowhat - 0x30 + 1);
		else if (alarminfo.al_dowhat < 0x40)
			printf("TV mode.\n");
		else
			printf("Computer mode. ADDR=%8.8x\n", alarminfo.al_dowhat);

		if (alarminfo.al_offtime == 0)
			printf("Never shut down automatically.\n");
		else
			printf("Shut down in %d seconds (%d minutes).\n",
			       alarminfo.al_offtime,
			       alarminfo.al_offtime / 60);
	} else {
		printf("RTC alarm is disabled.\n");
	}
}

static char *
numstr(unsigned int num)
{
	static char     buffer[4];

	if ((num & 0xf0) == 0xf0)
		buffer[0] = 'x';
	else
		buffer[0] = ((num & 0xf0) >> 4) + '0';

	if ((num & 0x0f) == 0x0f)
		buffer[1] = 'x';
	else
		buffer[1] = (num & 0x0f) + '0';

	buffer[2] = 0;

	return buffer;
}

const char * const weekname[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static void
showontime(unsigned int ontime)
{

	printf("At %s:", numstr((ontime & 0x0000ff00) >> 8));
	printf("%s ", numstr(ontime & 0x000000ff));

	if ((ontime & 0x0fff0000) != 0x0fff0000) {
		if ((ontime & 0x0f000000) != 0x0f000000) {
			if ((ontime & 0x00ff0000) != 0x00ff0000)
				printf("on %s, %sth in every month, \n",
				       weekname[(ontime & 0x0f000000) >> 24],
				       numstr((ontime & 0x00ff0000) >> 16));
			else
				printf("on every %s, \n",
				     weekname[(ontime & 0x0f000000) >> 24]);
		} else {
			printf("on %sth in every month, \n",
			       numstr((ontime & 0x00ff0000) >> 16));
		}
	} else {
		printf("everyday.\n");
	}
}

static void
disablealarm(void)
{
	int             fd;
	struct x68k_alarminfo alarminfo;

	alarminfo.al_enable = 0;

	fd = open(devicefile, O_WRONLY);
	if (fd < 0)
		err("Opening %s", devicefile);
	if (ioctl(fd, POWIOCSALARMINFO, &alarminfo) < 0)
		err("POWIOCSALARMINFO");
	close(fd);
}

static void
setinfo(int argc, char **argv)
{
	int             ch;
	int             week = 0x0f;
	int             hour = 0xffff;
	int             day = 0xff;
	int             offtime = 0;
	int             dowhat = 0;
	int             fd;
	struct x68k_alarminfo alarminfo;

	while ((ch = getopt(argc, argv, "w:d:m:s:c:")) != -1) {
		switch (ch) {
		case 'w':	/* day of the week */
			if ((week = strnum(optarg, 1)) < 0)
				usage();
			break;
		case 'd':	/* day of the month */
			if ((day = strnum(optarg, 2)) < 0)
				usage();
			break;
		case 'm':	/* for X minits */
			offtime = atoi(optarg);
			break;
		case 's':	/* for X seconds */
			offtime = atoi(optarg) / 60;
			break;
		case 'c':	/* channel */
			dowhat = atoi(optarg) + 0x30 - 1;
			if (dowhat < 0x30 || dowhat > 0x3b)
				usage();
			break;
		}
	}
	if (optind != argc - 1)
		usage();

#ifdef DEBUG
	printf("week: %x, day:%x, for:%d, hour:%x\n",
	       week, day, offtime, strnum(argv[optind], 5));
#endif

	hour = strnum(argv[optind], 5);
	if (hour < 0)
		usage();

	alarminfo.al_enable = 1;
	alarminfo.al_ontime = (week << 24) | (day << 16) | hour;
	alarminfo.al_dowhat = dowhat;
	alarminfo.al_offtime = offtime * 60;

	fd = open(devicefile, O_WRONLY);
	if (fd < 0)
		err("Opening %s", devicefile);
	if (ioctl(fd, POWIOCSALARMINFO, &alarminfo) < 0)
		err("POWIOCSALARMINFO");
	close(fd);
}

static int
strnum(const char *str, int wid)
{
	int             r;

	if (strlen(str) != wid)
		return -1;

	if (str[0] >= '0' && str[0] <= '9')
		r = str[0] - '0';
	else
		r = 0xf;
	if (wid == 1)
		return r;

	r <<= 4;
	if (str[1] >= '0' && str[1] <= '9')
		r += str[1] - '0';
	else
		r += 0xf;
	if (wid == 2)
		return r;

	if (str[2] != ':')
		return -1;
	r <<= 4;
	if (str[3] >= '0' && str[3] <= '9')
		r += str[3] - '0';
	else
		r += 0xf;
	r <<= 4;
	if (str[4] >= '0' && str[4] <= '9')
		r += str[4] - '0';
	else
		r += 0xf;

	return r;
}
