/* $NetBSD: tadpolectl.c,v 1.4 2000/03/14 21:27:41 jdc Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/envsys.h>
#include <machine/apmvar.h>
#include <machine/tctrl.h>

#define TCTRL_DEV	"/dev/tctrl0"

int aflag, nflag, wflag, dev;

#define PROTO(x) int x __P((int, int, int));
void usage __P((void));
static void parse __P((char *));
char *dashdot __P((char *));
int main __P((int, char *[]));
PROTO(hw_version)
PROTO(hw_microcontroller_version)
PROTO(hw_poweroncycles)
PROTO(hw_poweronseconds)
PROTO(hw_power_mains)
PROTO(hw_power_battery_int)
PROTO(hw_power_battery_ext)
PROTO(hw_power_battery_int_chargerate)
PROTO(hw_power_battery_ext_chargerate)
PROTO(hw_power_battery_int_chargelevel)
PROTO(hw_power_battery_ext_chargelevel)
PROTO(hw_video_external)
PROTO(hw_video_lid)
PROTO(hw_video_syncinva)
PROTO(hw_video_syncinvb)
PROTO(hw_video_compsync)
PROTO(hw_video_tft_brightness)
PROTO(hw_speaker_freq)
PROTO(hw_speaker_volume)
PROTO(hw_kbd_repeat_delay)
PROTO(hw_kbd_repeat_speed)
PROTO(hw_mouse_recalibrate)
PROTO(hw_power_battery_chargedisabled)
PROTO(hw_mouse_disable)
PROTO(hw_kbd_click)
PROTO(hw_mouse_intclick)
PROTO(hw_mouse_extclick)
PROTO(hw_mouse_sensitivity)
PROTO(hw_serial_power)

#define NUM_MIBS 29
#define TABLE(n) { __STRING(n), 0, n }

struct {
	char *mib;
	int value;
	int (*funcptr)(int, int, int);
} table[NUM_MIBS] = {
	TABLE(hw_microcontroller_version),
	TABLE(hw_version),
	TABLE(hw_poweroncycles),
	TABLE(hw_poweronseconds),
	TABLE(hw_power_mains),
	TABLE(hw_power_battery_int),
	TABLE(hw_power_battery_ext),
	TABLE(hw_power_battery_chargedisabled),
	TABLE(hw_power_battery_int_chargerate),
	TABLE(hw_power_battery_ext_chargerate),
	TABLE(hw_power_battery_int_chargelevel),
	TABLE(hw_power_battery_ext_chargelevel),
	TABLE(hw_video_external),
	TABLE(hw_video_lid),
	TABLE(hw_video_syncinva),
	TABLE(hw_video_syncinvb),
	TABLE(hw_video_compsync),
	TABLE(hw_video_tft_brightness),
	TABLE(hw_speaker_freq),
	TABLE(hw_speaker_volume),
	TABLE(hw_kbd_repeat_delay),
	TABLE(hw_kbd_repeat_speed),
	TABLE(hw_kbd_click),
	TABLE(hw_mouse_recalibrate),
	TABLE(hw_mouse_disable),
	TABLE(hw_mouse_intclick),
	TABLE(hw_mouse_extclick),
	TABLE(hw_mouse_sensitivity),
	TABLE(hw_serial_power),
};

#define FUNC(x) \
int \
x(read, new, num) \
	int read, new, num;

#define READ_REQ(a, b, c) \
	req.cmdbuf[0] = a; \
	req.cmdlen = b; \
	req.rsplen = c; \
	ioctl(dev, TCTRL_CMD_REQ, &req)

#define WRITE_REQ(a, b, c) \
	req.cmdbuf[0] = a; \
	req.cmdlen = b; \
	req.rsplen = c; \
	ioctl(dev, TCTRL_CMD_REQ, &req)

#define READ_ONLY \
	if (!read) \
		return(0)

/* hardware functions */

FUNC(hw_mouse_sensitivity)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x2c, 3, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else if (new > 255)
		req.cmdbuf[2] = 0xff;
	else
		req.cmdbuf[2] = new;
	req.cmdbuf[1] = 0x00;
	WRITE_REQ(0x2c, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x2c, 3, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}	

FUNC(hw_power_battery_chargedisabled)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x01 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x01;
	req.cmdbuf[1] = ~0x01;
	WRITE_REQ(0x22, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x01 ? 1 : 0;
	return(1);
}

FUNC(hw_mouse_disable)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x02 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x02;
	req.cmdbuf[1] = ~0x02;
	WRITE_REQ(0x22, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x02 ? 1 : 0;
	return(1);
}

FUNC(hw_kbd_click)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x04 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x04;
	req.cmdbuf[1] = ~0x04;
	WRITE_REQ(0x22, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x04 ? 1 : 0;
	return(1);
}

FUNC(hw_mouse_intclick)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x08 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x08;
	req.cmdbuf[1] = ~0x08;
	WRITE_REQ(0x22, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x08 ? 1 : 0;
	return(1);
}

FUNC(hw_mouse_extclick)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x10 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x10;
	req.cmdbuf[1] = ~0x10;
	WRITE_REQ(0x22, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x22, 3, 2);
	table[num].value = req.rspbuf[0]&0x10 ? 1 : 0;
	return(1);
}

/* ARGSUSED */
FUNC(hw_mouse_recalibrate)
{
	struct tctrl_req req;

	table[num].value = 0;
	if (read)
		return(1);
	READ_REQ(0x36, 1, 1);
	return(1);
}

FUNC(hw_kbd_repeat_delay)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x28, 3, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else if (new > 255)
		req.cmdbuf[2] = 0xff;
	else
		req.cmdbuf[2] = new;
	req.cmdbuf[1] = 0x00;
	WRITE_REQ(0x28, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x28, 3, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}	

FUNC(hw_kbd_repeat_speed)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x29, 3, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else if (new > 255)
		req.cmdbuf[2] = 0xff;
	else
		req.cmdbuf[2] = new;
	req.cmdbuf[1] = 0x00;
	WRITE_REQ(0x29, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x29, 3, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}	

FUNC(hw_speaker_freq)
{
	struct tctrl_req req;

	table[num].value = 0;
	if (read)
		return(1);
	req.cmdbuf[1] = new * 256;
	req.cmdbuf[2] = new % 256;
	WRITE_REQ(0x37, 3, 1);
	return(1);
}

FUNC(hw_speaker_volume)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x23, 3, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else if (new > 255)
		req.cmdbuf[2] = 0xff;
	else
		req.cmdbuf[2] = new;
	req.cmdbuf[1] = 0x00;
	WRITE_REQ(0x23, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x23, 3, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}	

FUNC(hw_video_tft_brightness)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x24, 3, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else if (new > 255)
		req.cmdbuf[2] = 0xff;
	else
		req.cmdbuf[2] = new;
	req.cmdbuf[1] = 0x00;
	WRITE_REQ(0x24, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x24, 3, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}	

FUNC(hw_video_syncinva)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x02 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x02;
	req.cmdbuf[1] = ~0x02;
	WRITE_REQ(0x21, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x02 ? 1 : 0;
	return(1);
}

FUNC(hw_video_syncinvb)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x04 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x04;
	req.cmdbuf[1] = ~0x04;
	WRITE_REQ(0x21, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x04 ? 1 : 0;
	return(1);
}

FUNC(hw_video_compsync)
{
	struct tctrl_req req;

	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x10 ? 1 : 0;
	if (read)
		return(1);
	if (new == 0)
		req.cmdbuf[2] = 0x00;
	else
		req.cmdbuf[2] = 0x10;
	req.cmdbuf[1] = ~0x10;
	WRITE_REQ(0x21, 3, 2);
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	READ_REQ(0x21, 3, 2);
	table[num].value = req.rspbuf[0]&0x10 ? 1 : 0;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_video_lid)
{
	struct tctrl_req req;
	short i;

	READ_ONLY;
	READ_REQ(0x11, 1, 3);
	i = (req.rspbuf[0]<<8) + req.rspbuf[1];
	table[num].value = i&0x0040 ? 0 : 1;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_video_external)
{
	struct tctrl_req req;
	short i;

	READ_ONLY;
	READ_REQ(0x11, 1, 3);
	i = (req.rspbuf[0]<<8) + req.rspbuf[1];
	table[num].value = i&0x0008 ? 1 : 0;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_power_battery_int_chargelevel)
{
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x7a, 1, 3);
	table[num].value = req.rspbuf[0] == 0xfb ? 0 : req.rspbuf[0];
	return(1);
	
}

/* ARGSUSED */                                              
FUNC(hw_power_battery_ext_chargelevel)
{
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x7b, 1, 3);
	table[num].value = req.rspbuf[0] == 0xfb ? 0 : req.rspbuf[0];
	return(1);
}

FUNC(hw_power_battery_int_chargerate)
{
	struct tctrl_req req;

	READ_REQ(0x18, 1, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	req.cmdbuf[1] = new < 255 ? new : 255;
	WRITE_REQ(0x39, 2, 1);
	READ_REQ(0x18, 1, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}

FUNC(hw_power_battery_ext_chargerate)
{
	struct tctrl_req req;

	READ_REQ(0x18, 1, 2);
	table[num].value = req.rspbuf[0];
	if (read)
		return(1);
	req.cmdbuf[1] = new < 255 ? new : 255;
	WRITE_REQ(0x39, 2, 1);
	READ_REQ(0x18, 1, 2);
	table[num].value = req.rspbuf[0];
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_power_battery_ext)
{
	int i;
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x11, 1, 3);
	i = (req.rspbuf[0]<<8) + req.rspbuf[1];
	table[num].value = i&0x0004 ? 1 : 0;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_power_battery_int)
{
	int i;
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x11, 1, 3);
	i = (req.rspbuf[0]<<8) + req.rspbuf[1];
	table[num].value = i&0x0002 ? 1 : 0;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_power_mains)
{
	int i;
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x11, 1, 3);
	i = (req.rspbuf[0]<<8) + req.rspbuf[1];
	table[num].value = i&0x0001 ? 1 : 0;
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_poweroncycles)
{
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x09, 1, 5);
	table[num].value = (req.rspbuf[0]<<24)+(req.rspbuf[1]<<16)+
	    (req.rspbuf[2]<<8)+req.rspbuf[3];
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_poweronseconds)
{
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x0a, 1, 5);
	table[num].value = (req.rspbuf[0]<<24)+(req.rspbuf[1]<<16)+
	    (req.rspbuf[2]<<8)+req.rspbuf[3];
	return(1);
}

/* ARGSUSED */                                              
FUNC(hw_microcontroller_version)
{
	char *bufp, buf[BUFSIZ];
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x04, 1, 3);
	bufp = buf;
	sprintf(bufp, "%d%d", req.rspbuf[0]*1000, req.rspbuf[1]*10);
	table[num].value = atoi(strdup(bufp));
	return(1);
}


/* ARGSUSED */                                              
FUNC(hw_version)
{
	char *bufp, buf[BUFSIZ];
	struct tctrl_req req;

	READ_ONLY;
	READ_REQ(0x03, 1, 3);
	bufp = buf;
	sprintf(bufp, "%d%d", req.rspbuf[0]*1000, req.rspbuf[1]*10);
	table[num].value = atoi(strdup(bufp));
	return(1);
}

FUNC(hw_serial_power)
{
	struct tctrl_pwr pwrreq;

	if (!read) {
		pwrreq.rw = 0x00;
		pwrreq.state = new;
		ioctl(dev, TCTRL_SERIAL_PWR, &pwrreq);
	}
	pwrreq.rw = 0x01;
	ioctl(dev, TCTRL_SERIAL_PWR, &pwrreq);
	table[num].value = pwrreq.state;
	return(1);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: tadpolectl [-n] name ...\n"
	    "       tadpolectl [-n] -w name=value\n"
	    "       tadpolectl [-n] -a\n");
	exit(1);
}

static void
parse(string)
	char *string;
{
	char *cp, buf[BUFSIZ];
	int newval = 0;
	int i, j, ret;

	string = dashdot(string);
	snprintf(buf, (size_t)BUFSIZ, "%s", string);
	if ((cp = strchr(string, '=')) != NULL) {
		if (!wflag)
			errx(2, "Must specify -w to set variables");
		*strchr(buf, '=') = '\0';
		*cp++ = '\0';
		while (isspace((unsigned char) *cp))
			cp++;
		newval = atoi(cp);
	}
	for (j=0,i=-1; j < NUM_MIBS; j++) {
		if (strcmp(string, table[j].mib) == 0) {
			i = j;
			break;
		}
	}
	if (i == -1)
		errx(2, "Named value does not exist");

	if (wflag) {
		ret = (*table[i].funcptr)(0, newval, i);
		if (!ret)
			errx(2, "Cannot modify this value");
	} else
		ret = (*table[i].funcptr)(1, 0, i);
	if (nflag)
		printf("%d\n", table[i].value);
	else
		printf("%s = %d\n", dashdot(table[i].mib), table[i].value);
}

char *
dashdot(string)
	char *string;
{
	char *p;
	char *save;

	p = strdup(string);
	save = p;

	for (; (*p = *string) != '\0'; ++p, ++string) {
		if (*p == '.')
			*p = '_';
		else if (*p == '_')
			*p = '.';
	}
	return(save);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	int ch, j;

	while ((ch = getopt(argc, argv, "anw")) != -1) {
		switch (ch) {

		case 'a':
			aflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((dev = open(TCTRL_DEV, O_RDONLY, NULL)) == -1)
		err(1, "%s", TCTRL_DEV);

	if (aflag) {
		for (j=0; j < NUM_MIBS; j++) {
			(void)(*table[j].funcptr)(1, 0, j);
			if (nflag)
				printf("%d\n", table[j].value);
			else
				printf("%s = %d\n", dashdot(table[j].mib),
				    table[j].value);
		}
		return(0);
	}
	if (argc == 0)
		usage();
	while (argc-- > 0)
		parse(*argv++);
	return(0);
}
