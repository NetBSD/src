/*	$NetBSD: eehandlers.c,v 1.15.12.3 2014/08/20 00:05:07 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <sys/inttypes.h>

#include <machine/eeprom.h>

#include "defs.h"

extern	char *path_eeprom;
extern	int eval;
extern	int update_checksums;
extern	int ignore_checksum;
extern	int fix_checksum;
extern	int cksumfail;
extern	u_short writecount;

static	char err_str[BUFSIZE];

static	void badval(const struct keytabent *, char *);
static	int doio(const struct keytabent *, u_char *, ssize_t, int);

static	u_char ee_checksum(u_char *, size_t);
static	void ee_hwupdate(const struct keytabent *, char *);
static	void ee_num8(const struct keytabent *, char *);
static	void ee_num16(const struct keytabent *, char *);
static	void ee_screensize(const struct keytabent *, char *);
static	void ee_truefalse(const struct keytabent *, char *);
static	void ee_bootdev(const struct keytabent *, char *);
static	void ee_kbdtype(const struct keytabent *, char *);
static	void ee_constype(const struct keytabent *, char *);
static	void ee_diagpath(const struct keytabent *, char *);
static	void ee_banner(const struct keytabent *, char *);
static	void ee_notsupp(const struct keytabent *, char *);

static const struct	keytabent eekeytab[] = {
	{ "hwupdate",		0x10,	ee_hwupdate },
	{ "memsize",		0x14,	ee_num8 },
	{ "memtest",		0x15,	ee_num8 },
	{ "scrsize",		0x16,	ee_screensize },
	{ "watchdog_reboot",	0x17,	ee_truefalse },
	{ "default_boot",	0x18,	ee_truefalse },
	{ "bootdev",		0x19,	ee_bootdev },
	{ "kbdtype",		0x1e,	ee_kbdtype },
	{ "console",		0x1f,	ee_constype },
	{ "keyclick",		0x21,	ee_truefalse },
	{ "diagdev",		0x22,	ee_bootdev },
	{ "diagpath",		0x28,	ee_diagpath },
	{ "columns",		0x50,	ee_num8 },
	{ "rows",		0x51,	ee_num8 },
	{ "ttya_use_baud",	0x58,	ee_truefalse },
	{ "ttya_baud",		0x59,	ee_num16 },
	{ "ttya_no_rtsdtr",	0x5b,	ee_truefalse },
	{ "ttyb_use_baud",	0x60,	ee_truefalse },
	{ "ttyb_baud",		0x61,	ee_num16 },
	{ "ttyb_no_rtsdtr",	0x63,	ee_truefalse },
	{ "banner",		0x68,	ee_banner },
	{ "secure",		0,	ee_notsupp },
	{ "bad_login",		0,	ee_notsupp },
	{ "password",		0,	ee_notsupp },
	{ NULL,			0,	ee_notsupp },
};

#define BARF(kt) {							\
	badval((kt), arg);						\
	++eval;								\
	return;								\
}

#define FAILEDREAD(kt) {						\
	warnx("%s", err_str);						\
	warnx("failed to read field `%s'", (kt)->kt_keyword);		\
	++eval;								\
	return;								\
}

#define FAILEDWRITE(kt) {						\
	warnx("%s", err_str);						\
	warnx("failed to update field `%s'", (kt)->kt_keyword);		\
	++eval;								\
	return;								\
}

void
ee_action(char *keyword, char *arg)
{
	const struct keytabent *ktent;

	for (ktent = eekeytab; ktent->kt_keyword != NULL; ++ktent) {
		if (strcmp(ktent->kt_keyword, keyword) == 0) {
			(*ktent->kt_handler)(ktent, arg);
			return; 
		}
	}

	warnx("unknown keyword %s", keyword);
	++eval;
}

void
ee_dump(void)
{
	const struct keytabent *ktent;

	for (ktent = eekeytab; ktent->kt_keyword != NULL; ++ktent)
		(*ktent->kt_handler)(ktent, NULL);
}

static void
ee_hwupdate(const struct keytabent *ktent, char *arg)
{
	uint32_t hwtime;
	time_t t;
	char *cp, *cp2;

	if (arg) {
		if ((strcmp(arg, "now") == 0) ||
		    (strcmp(arg, "today") == 0)) {
			if ((t = time(NULL)) == (time_t)(-1)) {
				warnx("can't get current time");
				++eval;
				return;
			}
		} else
			if ((t = parsedate(arg, NULL, NULL)) == (time_t)(-1))
				BARF(ktent);
		hwtime = (uint32_t)t;	/* XXX 32 bit time_t on hardware */
		if (hwtime != t)
			warnx("time overflow");

		if (doio(ktent, (u_char *)&hwtime, sizeof(hwtime), IO_WRITE))
			FAILEDWRITE(ktent);
	} else {
		if (doio(ktent, (u_char *)&hwtime, sizeof(hwtime), IO_READ))
			FAILEDREAD(ktent);
		t = (time_t)hwtime;	/* XXX 32 bit time_t on hardware */
	}

	cp = ctime(&t);
	if (cp != NULL && (cp2 = strrchr(cp, '\n')) != NULL)
		*cp2 = '\0';

	printf("%s=%" PRId64, ktent->kt_keyword, (int64_t)t);
	if (cp != NULL)
		printf(" (%s)", cp);
	printf("\n");
}

static void
ee_num8(const struct keytabent *ktent, char *arg)
{
	u_char num8 = 0;
	u_int num32;
	int i;

	if (arg) {
		for (i = 0; i < (int)strlen(arg) - 1; ++i)
			if (!isdigit((unsigned char)arg[i]))
				BARF(ktent);
		num32 = atoi(arg);
		if (num32 > 0xff)
			BARF(ktent);
		num8 += num32;
		if (doio(ktent, &num8, sizeof(num8), IO_WRITE))
			FAILEDWRITE(ktent);
	} else
		if (doio(ktent, &num8, sizeof(num8), IO_READ))
			FAILEDREAD(ktent);

	printf("%s=%d\n", ktent->kt_keyword, num8);
}

static void
ee_num16(const struct keytabent *ktent, char *arg)
{
	u_int16_t num16 = 0;
	u_int num32;
	int i;

	if (arg) {
		for (i = 0; i < (int)strlen(arg) - 1; ++i)
			if (!isdigit((unsigned char)arg[i]))
				BARF(ktent);
		num32 = atoi(arg);
		if (num32 > 0xffff)
			BARF(ktent);
		num16 += num32;
		if (doio(ktent, (u_char *)&num16, sizeof(num16), IO_WRITE))
			FAILEDWRITE(ktent);
	} else
		if (doio(ktent, (u_char *)&num16, sizeof(num16), IO_READ))
			FAILEDREAD(ktent);

	printf("%s=%d\n", ktent->kt_keyword, num16);
}

static	const struct strvaltabent scrsizetab[] = {
	{ "1152x900",		EE_SCR_1152X900 },
	{ "1024x1024",		EE_SCR_1024X1024 },
	{ "1600x1280",		EE_SCR_1600X1280 },
	{ "1440x1440",		EE_SCR_1440X1440 },
	{ NULL,			0 },
};

static void
ee_screensize(const struct keytabent *ktent, char *arg)
{
	const struct strvaltabent *svp;
	u_char scsize;

	if (arg) {
		for (svp = scrsizetab; svp->sv_str != NULL; ++svp)
			if (strcmp(svp->sv_str, arg) == 0)
				break;
		if (svp->sv_str == NULL)
			BARF(ktent);
		
		scsize = svp->sv_val;
		if (doio(ktent, &scsize, sizeof(scsize), IO_WRITE))
			FAILEDWRITE(ktent);
	} else {
		if (doio(ktent, &scsize, sizeof(scsize), IO_READ))
			FAILEDREAD(ktent);

		for (svp = scrsizetab; svp->sv_str != NULL; ++svp)
			if (svp->sv_val == scsize)
				break;
		if (svp->sv_str == NULL) {
			warnx("unknown %s value %d", ktent->kt_keyword,
			    scsize);
			return;
		}
	}
	printf("%s=%s\n", ktent->kt_keyword, svp->sv_str);
}

static	const struct strvaltabent truthtab[] = {
	{ "true",		EE_TRUE },
	{ "false",		EE_FALSE },
	{ NULL,			0 },
};

static void
ee_truefalse(const struct keytabent *ktent, char *arg)
{
	const struct strvaltabent *svp;
	u_char truth;

	if (arg) {
		for (svp = truthtab; svp->sv_str != NULL; ++svp)
			if (strcmp(svp->sv_str, arg) == 0)
				break;
		if (svp->sv_str == NULL)
			BARF(ktent);

		truth = svp->sv_val;
		if (doio(ktent, &truth, sizeof(truth), IO_WRITE))
			FAILEDWRITE(ktent);
	} else {
		if (doio(ktent, &truth, sizeof(truth), IO_READ))
			FAILEDREAD(ktent);

		for (svp = truthtab; svp->sv_str != NULL; ++svp)
			if (svp->sv_val == truth)
				break;
		if (svp->sv_str == NULL) {
			warnx("unknown truth value 0x%x for %s", truth,
			    ktent->kt_keyword);
			return;
		}
	}
	printf("%s=%s\n", ktent->kt_keyword, svp->sv_str);
}

static void
ee_bootdev(const struct keytabent *ktent, char *arg)
{
	u_char dev[5];
	int i;
	size_t arglen;
	char *cp;

	if (arg) {
		/*
		 * The format of the string we accept is the following:
		 *	cc(n,n,n)
		 * where:
		 *	c -- an alphabetical character [a-z]
		 *	n -- a number in hexadecimal, between 0 and ff,
		 *	     with no leading `0x'.
		 */
		arglen = strlen(arg);
		if (arglen < 9 || arglen > 12 || arg[2] != '(' ||
		     arg[arglen - 1] != ')')
			BARF(ktent);

		/* Handle the first 2 letters. */
		for (i = 0; i < 2; ++i) {
			if (arg[i] < 'a' || arg[i] > 'z')
				BARF(ktent);
			dev[i] = (u_char)arg[i];
		}

		/* Handle the 3 `0x'-less hex values. */
		cp = &arg[3];
		for (i = 2; i < 5; ++i) {
			if (*cp == '\0')
				BARF(ktent);

			if (*cp >= '0' && *cp <= '9')
				dev[i] = *cp++ - '0';
			else if (*cp >= 'a' && *cp <= 'f')
				dev[i] = 10 + (*cp++ - 'a');
			else
				BARF(ktent);

			/* Deal with a second digit. */
			if (*cp >= '0' && *cp <= '9') {
				dev[i] <<= 4;
				dev[i] &= 0xf0;
				dev[i] += *cp++ - '0';
			} else if (*cp >= 'a' && *cp <= 'f') {
				dev[i] <<= 4;
				dev[i] &= 0xf0;
				dev[i] += 10 + (*cp++ - 'a');
			}

			/* Ensure we have the correct delimiter. */
			if ((*cp == ',' && i < 4) || (*cp == ')' && i == 4)) {
				++cp;
				continue;
			} else
				BARF(ktent);
		}
		if (doio(ktent, (u_char *)&dev[0], sizeof(dev), IO_WRITE))
			FAILEDWRITE(ktent);
	} else
		if (doio(ktent, (u_char *)&dev[0], sizeof(dev), IO_READ))
			FAILEDREAD(ktent);

	printf("%s=%c%c(%x,%x,%x)\n", ktent->kt_keyword, dev[0],
	     dev[1], dev[2], dev[3], dev[4]);
}

static void
ee_kbdtype(const struct keytabent *ktent, char *arg)
{
	u_char kbd = 0;
	u_int kbd2;
	int i;

	if (arg) {
		for (i = 0; i < (int)strlen(arg) - 1; ++i)
			if (!isdigit((unsigned char)arg[i]))
				BARF(ktent);
		kbd2 = atoi(arg);
		if (kbd2 > 0xff)
			BARF(ktent);
		kbd += kbd2;
		if (doio(ktent, &kbd, sizeof(kbd), IO_WRITE))
			FAILEDWRITE(ktent);
	} else
		if (doio(ktent, &kbd, sizeof(kbd), IO_READ))
			FAILEDREAD(ktent);

	printf("%s=%d (%s)\n", ktent->kt_keyword, kbd, kbd ? "other" : "Sun");
}

static	const struct strvaltabent constab[] = {
	{ "b&w",		EE_CONS_BW },
	{ "ttya",		EE_CONS_TTYA },
	{ "ttyb",		EE_CONS_TTYB },
	{ "color",		EE_CONS_COLOR },
	{ "p4opt",		EE_CONS_P4OPT },
	{ NULL,			0 },
};

static void
ee_constype(const struct keytabent *ktent, char *arg)
{
	const struct strvaltabent *svp;
	u_char cons;

	if (arg) {
		for (svp = constab; svp->sv_str != NULL; ++svp)
			if (strcmp(svp->sv_str, arg) == 0)
				break;
		if (svp->sv_str == NULL)
			BARF(ktent);

		cons = svp->sv_val;
		if (doio(ktent, &cons, sizeof(cons), IO_WRITE))
			FAILEDWRITE(ktent);
	} else {
		if (doio(ktent, &cons, sizeof(cons), IO_READ))
			FAILEDREAD(ktent);

		for (svp = constab; svp->sv_str != NULL; ++svp)
			if (svp->sv_val == cons)
				break;
		if (svp->sv_str == NULL) {
			warnx("unknown type 0x%x for %s", cons,
			    ktent->kt_keyword);
			return;
		}
	}
	printf("%s=%s\n", ktent->kt_keyword, svp->sv_str);
		
}

static void
ee_diagpath(const struct keytabent *ktent, char *arg)
{
	char path[40];

	memset(path, 0, sizeof(path));
	if (arg) {
		if (strlen(arg) > sizeof(path))
			BARF(ktent);
		memcpy(path, arg, sizeof path);
		if (doio(ktent, (u_char *)&path[0], sizeof(path), IO_WRITE))
			FAILEDWRITE(ktent);
	} else
		if (doio(ktent, (u_char *)&path[0], sizeof(path), IO_READ))
			FAILEDREAD(ktent);

	printf("%s=%s\n", ktent->kt_keyword, path);
}

static void
ee_banner(const struct keytabent *ktent, char *arg)
{
	char string[80];
	u_char enable;
	struct keytabent kt;

	kt.kt_keyword = "enable_banner";
	kt.kt_offset = EE_BANNER_ENABLE_LOC;
	kt.kt_handler = ee_notsupp;

	memset(string, '\0', sizeof(string));
	if (arg) {
		if (strlen(arg) > sizeof(string))
			BARF(ktent);
		if (*arg != '\0') {
			enable = EE_TRUE;
			memcpy(string, arg, sizeof string);
			if (doio(ktent, (u_char *)string,
			    sizeof(string), IO_WRITE))
				FAILEDWRITE(ktent);
		} else {
			enable = EE_FALSE;
			if (doio(ktent, (u_char *)string,
			    sizeof(string), IO_READ))
				FAILEDREAD(ktent);
		}

		if (doio(&kt, &enable, sizeof(enable), IO_WRITE))
			FAILEDWRITE(&kt);
	} else {
		if (doio(ktent, (u_char *)string, sizeof(string), IO_READ))
			FAILEDREAD(ktent);
		if (doio(&kt, &enable, sizeof(enable), IO_READ))
			FAILEDREAD(&kt);
	}
	printf("%s=%s (%s)\n", ktent->kt_keyword, string,
	    enable == EE_TRUE ? "enabled" : "disabled");
}

/* ARGSUSED */
static void
ee_notsupp(const struct keytabent *ktent, char *arg)
{

	warnx("field `%s' not yet supported", ktent->kt_keyword);
}

static void
badval(const struct keytabent *ktent, char *arg)
{

	warnx("inappropriate value `%s' for field `%s'", arg,
	    ktent->kt_keyword);
}

static int
doio(const struct keytabent *ktent, u_char *buf, ssize_t len, int wr)
{
	int fd, rval = 0;
	u_char *buf2;

	buf2 = (u_char *)calloc(1, len);
	if (buf2 == NULL) {
		memcpy(err_str, "memory allocation failed", sizeof err_str);
		return (1);
	}

	fd = open(path_eeprom, wr == IO_WRITE ? O_RDWR : O_RDONLY, 0640);
	if (fd < 0) {
		(void)snprintf(err_str, sizeof err_str, "open: %s: %s", path_eeprom,
		    strerror(errno));
		free(buf2);
		return (1);
	}

	if (lseek(fd, (off_t)ktent->kt_offset, SEEK_SET) < (off_t)0) {
		(void)snprintf(err_str, sizeof err_str, "lseek: %s: %s",
		    path_eeprom, strerror(errno));
		rval = 1;
		goto done;
	}

	if (read(fd, buf2, len) != len) {
		(void)snprintf(err_str, sizeof err_str, "read: %s: %s",
		    path_eeprom, strerror(errno));
		return (1);
	}

	if (wr == IO_WRITE) {
		if (memcmp(buf, buf2, len) == 0)
			goto done;

		if (lseek(fd, (off_t)ktent->kt_offset, SEEK_SET) < (off_t)0) {
			(void)snprintf(err_str, sizeof err_str, "lseek: %s: %s",
			    path_eeprom, strerror(errno));
			rval = 1;
			goto done;
		}

		++update_checksums;
		if (write(fd, buf, len) < 0) {
			(void)snprintf(err_str, sizeof err_str, "write: %s: %s",
			    path_eeprom, strerror(errno));
			rval = 1;
			goto done;
		}
	} else
		memmove(buf, buf2, len);

 done:
	free(buf2);
	(void)close(fd);
	return (rval);
}

/*
 * Read from eeLastHwUpdate to just before eeReserved.  Calculate
 * a checksum, and deposit 3 copies of it sequentially starting at
 * eeChecksum[0].  Increment the write count, and deposit 3 copies
 * of it sequentially starting at eeWriteCount[0].
 */
void
ee_updatechecksums(void)
{
	struct keytabent kt;
	u_char checkme[EE_SIZE - EE_HWUPDATE_LOC];
	u_char checksum;
	int i;

	kt.kt_keyword = "eeprom contents";
	kt.kt_offset = EE_HWUPDATE_LOC;
	kt.kt_handler = ee_notsupp;

	if (doio(&kt, checkme, sizeof(checkme), IO_READ)) {
		cksumfail = 1;
		FAILEDREAD(&kt);
	}

	checksum = ee_checksum(checkme, sizeof(checkme));

	kt.kt_keyword = "eeprom checksum";
	for (i = 0; i < 4; ++i) {
		kt.kt_offset = EE_CKSUM_LOC + (i * sizeof(checksum));
		if (doio(&kt, &checksum, sizeof(checksum), IO_WRITE)) {
			cksumfail = 1;
			FAILEDWRITE(&kt);
		}
	}

	kt.kt_keyword = "eeprom writecount";
	for (i = 0; i < 4; ++i) {
		kt.kt_offset = EE_WC_LOC + (i * sizeof(writecount));
		if (doio(&kt, (u_char *)&writecount, sizeof(writecount),
		    IO_WRITE)) {
			cksumfail = 1;
			FAILEDWRITE(&kt);
		}
	}
}

void
ee_verifychecksums(void)
{
	struct keytabent kt;
	u_char checkme[EE_SIZE - EE_HWUPDATE_LOC];
	u_char checksum, ochecksum[3];
	u_short owritecount[3];

	/*
	 * Verify that the EEPROM's write counts match, and update the
	 * global copy for use later.
	 */
	kt.kt_keyword = "eeprom writecount";
	kt.kt_offset = EE_WC_LOC;
	kt.kt_handler = ee_notsupp;
	
	if (doio(&kt, (u_char *)&owritecount, sizeof(owritecount), IO_READ)) {
		cksumfail = 1;
		FAILEDREAD(&kt);
	}

	if (owritecount[0] != owritecount[1] ||
	    owritecount[0] != owritecount[2]) {
		warnx("eeprom writecount mismatch %s",
		    ignore_checksum ? "(ignoring)" :
		    (fix_checksum ? "(fixing)" : ""));

		if (!ignore_checksum && !fix_checksum) {
			cksumfail = 1;
			return;
		}

		writecount = MAXIMUM(owritecount[0], owritecount[1]);
		writecount = MAXIMUM(writecount, owritecount[2]);
	} else
		writecount = owritecount[0];

	/*
	 * Verify that the EEPROM's checksums match and are correct.
	 */
	kt.kt_keyword = "eeprom checksum";
	kt.kt_offset = EE_CKSUM_LOC;

	if (doio(&kt, ochecksum, sizeof(ochecksum), IO_READ)) {
		cksumfail = 1;
		FAILEDREAD(&kt);
	}

	if (ochecksum[0] != ochecksum[1] ||
	    ochecksum[0] != ochecksum[2]) {
		warnx("eeprom checksum mismatch %s",
		    ignore_checksum ? "(ignoring)" :
		    (fix_checksum ? "(fixing)" : ""));

		if (!ignore_checksum && !fix_checksum) {
			cksumfail = 1;
			return;
		}
	}

	kt.kt_keyword = "eeprom contents";
	kt.kt_offset = EE_HWUPDATE_LOC;

	if (doio(&kt, checkme, sizeof(checkme), IO_READ)) {
		cksumfail = 1;
		FAILEDREAD(&kt);
	}

	checksum = ee_checksum(checkme, sizeof(checkme));

	if (ochecksum[0] != checksum) {
		warnx("eeprom checksum incorrect %s",
		    ignore_checksum ? "(ignoring)" :
		    (fix_checksum ? "(fixing)" : ""));

		if (!ignore_checksum && !fix_checksum) {
			cksumfail = 1;
			return;
		}
	}

	if (fix_checksum)
		ee_updatechecksums();
}

static u_char
ee_checksum(u_char *area, size_t len)
{
	u_char sum = 0;

	while (len--)
		sum += *area++;

	return (0x100 - sum);
}
