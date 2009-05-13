/*	$NetBSD: bootpref.c,v 1.3.14.1 2009/05/13 17:16:31 jym Exp $	*/
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <sys/mman.h>
#include "bootpref.h"

static void	usage(void);
static int	openNVRAM(void);
static void	closeNVRAM(int);
static u_char	readNVRAM(int, int);
static void	writeNVRAM(int, int, u_char);
static void	getNVpref(int, u_char[]);
static void	setNVpref(int, u_char[], int, int);
static void	showOS(u_char);
static void	showLang(u_char);
static void	showKbdLang(u_char);
static void	showDateFmt(u_char);
static void	showDateSep(u_char);
static void	showVideo2(u_char);
static void	showVideo1(u_char, u_char);
static int	checkOS(u_char *, char *);
static int	checkLang(u_char *, char *);
static int	checkKbdLang(u_char *, char *);
static int	checkInt(u_char *, char *, int, int);
static int 	checkDateFmt(u_char *, char *);
static void 	checkDateSep(u_char *, char *);
static int 	checkColours(u_char *, char *);

#define SET_OS		0x001
#define SET_LANG	0x002
#define SET_KBDLANG	0x004
#define SET_HOSTID	0x008
#define SET_DATIME	0x010
#define SET_DATESEP	0x020
#define SET_BOOTDLY	0x040
#define SET_VID1	0x080
#define SET_VID2	0x100

#define ARRAY_OS	0
#define ARRAY_LANG	1
#define ARRAY_KBDLANG	2
#define ARRAY_HOSTID	3
#define ARRAY_DATIME	4
#define ARRAY_DATESEP	5
#define ARRAY_BOOTDLY	6
#define ARRAY_VID1	7
#define ARRAY_VID2	8

static const char	nvrdev[] = PATH_NVRAM;

int
main (int argc, char *argv[])
{
	int	c, set = 0, verbose = 0;
	int	fd;
	u_char	bootpref[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	u_char	check_hour = 0, check_video1 = 0, check_video2 = 0;

	fd = openNVRAM ();
	bootpref[ARRAY_VID2] = readNVRAM (fd, NVRAM_VID2);
	bootpref[ARRAY_VID1] = readNVRAM (fd, NVRAM_VID1);
	/* parse options */
	while ((c = getopt (argc, argv, "Vb:d:k:l:s:f:12e:c:nptv48oOxXiI")) != -1) {
		switch (c) {
		case 'V':
			verbose = 1;
			break;
		case 'b':
			if (checkOS (&bootpref[ARRAY_OS], optarg))
				set |= SET_OS;
			else
				usage ();
			break;
		case 'd':
			if (checkInt (&bootpref[ARRAY_BOOTDLY], optarg,
			   0, 255))
				set |= SET_BOOTDLY;
			else
				usage ();
			break;
		case 'k':
			if (checkKbdLang (&bootpref[ARRAY_KBDLANG], optarg))
				set |= SET_KBDLANG;
			else
				usage ();
			break;
		case 'l':
			if (checkLang (&bootpref[ARRAY_LANG], optarg))
				set |= SET_LANG;
			else
				usage ();
			break;
		case 's':
			if (checkInt (&bootpref[ARRAY_HOSTID], optarg,
			    0, 7))
				set |= SET_HOSTID;
			else
				usage ();
			break;
		case 'f':
			if (checkDateFmt (&bootpref[ARRAY_DATIME], optarg))
				set |= SET_DATIME;
			else
				usage ();
			break;
		case '1':
			if (check_hour & DATIME_24H) {
				usage();
			} else {
				bootpref[ARRAY_DATIME] &= ~DATIME_24H;
				set |= SET_DATIME;
				check_hour |= DATIME_24H;
			}
			break;
		case '2':
			if (check_hour & DATIME_24H) {
				usage();
			} else {
				bootpref[ARRAY_DATIME] |= DATIME_24H;
				set |= SET_DATIME;
				check_hour |= DATIME_24H;
			}
			break;
		case 'e':
			checkDateSep (&bootpref[ARRAY_DATESEP], optarg);
			set |= SET_DATESEP;
			break;
		case 'c':
			if (checkColours (&bootpref[ARRAY_VID2], optarg))
				set |= SET_VID2;
			else
				usage ();
			break;
		case 'n':
			if (check_video2 & VID2_PAL) {
				usage();
			} else {
				bootpref[ARRAY_VID2] &= ~VID2_PAL;
				set |= SET_VID2;
				check_video2 |= VID2_PAL;
			}
			break;
		case 'p':
			if (check_video2 & VID2_PAL) {
				usage();
			} else {
				bootpref[ARRAY_VID2] |= VID2_PAL;
				set |= SET_VID2;
				check_video2 |= VID2_PAL;
			}
			break;
		case 't':
			if (check_video2 & VID2_VGA) {
				usage();
			} else {
				bootpref[ARRAY_VID2] &= ~VID2_VGA;
				set |= SET_VID2;
				check_video2 |= VID2_VGA;
			}
			break;
		case 'v':
			if (check_video2 & VID2_VGA) {
				usage();
			} else {
				bootpref[ARRAY_VID2] |= VID2_VGA;
				set |= SET_VID2;
				check_video2 |= VID2_VGA;
			}
			break;
		case '4':
			if (check_video2 & VID2_80CLM) {
				usage();
			} else {
				bootpref[ARRAY_VID2] &= ~VID2_80CLM;
				set |= SET_VID2;
				check_video2 |= VID2_80CLM;
			}
			break;
		case '8':
			if (check_video2 & VID2_80CLM) {
				usage();
			} else {
				bootpref[ARRAY_VID2] |= VID2_80CLM;
				set |= SET_VID2;
				check_video2 |= VID2_80CLM;
			}
			break;
		case 'o':
			if (check_video2 & VID2_OVERSCAN) {
				usage();
			} else {
				bootpref[ARRAY_VID2] |= VID2_OVERSCAN;
				set |= SET_VID2;
				check_video2 |= VID2_OVERSCAN;
			}
			break;
		case 'O':
			if (check_video2 & VID2_OVERSCAN) {
				usage();
			} else {
				bootpref[ARRAY_VID2] &= ~VID2_OVERSCAN;
				set |= SET_VID2;
				check_video2 |= VID2_OVERSCAN;
			}
			break;
		case 'x':
			if (check_video2 & VID2_COMPAT) {
				usage();
			} else {
				bootpref[ARRAY_VID2] |= VID2_COMPAT;
				set |= SET_VID2;
				check_video2 |= VID2_COMPAT;
			}
			break;
		case 'X':
			if (check_video2 & VID2_COMPAT) {
				usage();
			} else {
				bootpref[ARRAY_VID2] &= ~VID2_COMPAT;
				set |= SET_VID2;
				check_video2 |= VID2_COMPAT;
			}
			break;
		case 'i':
			if (check_video1 & VID1_INTERLACE) {
				usage();
			} else {
				bootpref[ARRAY_VID1] |= VID1_INTERLACE;
				set |= SET_VID1;
				check_video1 |= VID1_INTERLACE;
			}
			break;
		case 'I':
			if (check_video1 & VID1_INTERLACE) {
				usage();
			} else {
				bootpref[ARRAY_VID1] &= ~VID1_INTERLACE;
				set |= SET_VID1;
				check_video1 |= VID1_INTERLACE;
			}
			break;
		default:
			usage ();
		}
	}
	if (optind != argc) {
		usage ();
	}
	if (set) {
		setNVpref (fd, bootpref, set, verbose);
	} else {
		getNVpref (fd, bootpref);
	}
	closeNVRAM (fd);
	return (EXIT_SUCCESS);
}

static void
usage (void)
{
	fprintf (stderr,
		"usage: bootpref [-V] [-b os] [-d delay] [-k kbd] [-l lang] "
		"[-s id]\n"
		"\t[-f fmt] [-1] [-2] [-e sep]\n"
		"\t[-c colours] [-n] [-p] [-t] [-v] [-4] [-8]\n"
		"\t[-o] [-O] [-x] [-X] [-i] [-I]\n");
	exit (EXIT_FAILURE);
}

static int
openNVRAM ()
{
	int fd;

	if ((fd = open (nvrdev, O_RDWR)) < 0) {
		err (EXIT_FAILURE, "%s", nvrdev);
	}
	return (fd);
}

static void
closeNVRAM (int fd)
{
	if (close (fd) < 0) {
		err (EXIT_FAILURE, "%s", nvrdev);
	}
}

static u_char
readNVRAM (int fd, int pos)
{
	u_char val;

	if (lseek(fd, (off_t)pos, SEEK_SET) != pos) {
		err(EXIT_FAILURE, "%s", nvrdev);
	}
	if (read (fd, &val, (size_t)1) != 1) {
		err(EXIT_FAILURE, "%s", nvrdev);
	}
	return (val);
}

static void
writeNVRAM (int fd, int pos, u_char val)
{
	if (lseek(fd, (off_t)pos, SEEK_SET) != pos) {
		err(EXIT_FAILURE, "%s", nvrdev);
	}
	if (write (fd, &val, (size_t)1) != 1) {
		err(EXIT_FAILURE, "%s", nvrdev);
	}
}

static void
getNVpref (int fd, u_char bootpref[])
{
	/* Boot OS */
	printf ("Boot OS is ");
	showOS (readNVRAM (fd, NVRAM_BOOTPREF));
	/* Boot Delay */
	printf ("Boot delay is %d seconds\n", readNVRAM (fd, NVRAM_BOOTDLY));
	/* TOS Language */
	printf ("Language is ");
	showLang (readNVRAM (fd, NVRAM_LANG));
	/* Keyboard Language */
	printf ("Keyboard is ");
	showKbdLang (readNVRAM (fd, NVRAM_KBDLANG));
	/* SCSI Host ID */
	printf ("SCSI host ID is ");
	if (readNVRAM (fd, NVRAM_HOSTID) & HOSTID_VALID) {
		printf ("%d\n", readNVRAM (fd, NVRAM_HOSTID) ^ HOSTID_VALID);
	} else {
		printf ("invalid");
	}
	/* Date format/separator */
	printf ("Date format is ");
	showDateFmt (readNVRAM (fd, NVRAM_DATIME));
	printf ("Date separator is ");
	showDateSep (readNVRAM (fd, NVRAM_DATESEP));
	/* Video */
	printf ("Video is (0x%02x, 0x%02x) :\n", readNVRAM (fd, NVRAM_VID2),
	    readNVRAM (fd, NVRAM_VID1));
	showVideo2 (readNVRAM (fd, NVRAM_VID2));
	showVideo1 (readNVRAM (fd, NVRAM_VID1), readNVRAM (fd, NVRAM_VID2));
}

static void
setNVpref (int fd, u_char bootpref[], int set, int verbose)
{
	/* Boot OS */
	if (set & SET_OS) {
		writeNVRAM (fd, NVRAM_BOOTPREF, bootpref[ARRAY_OS]);
		if (verbose) {
			printf ("Boot OS set to ");
			showOS (readNVRAM (fd, NVRAM_BOOTPREF));
		}
	}
	/* Boot Delay */
	if (set & SET_BOOTDLY) {
		writeNVRAM (fd, NVRAM_BOOTDLY, bootpref[ARRAY_BOOTDLY]);
		if (verbose) {
			printf ("Boot delay set to %d seconds\n", readNVRAM (fd,
			    NVRAM_BOOTDLY));
		}
	}
	/* TOS Language */
	if (set & SET_LANG) {
		writeNVRAM (fd, NVRAM_LANG, bootpref[ARRAY_LANG]);
		if (verbose) {
			printf ("Language set to ");
			showLang (readNVRAM (fd, NVRAM_LANG));
		}
	}
	/* Keyboard Language */
	if (set & SET_KBDLANG) {
		writeNVRAM (fd, NVRAM_KBDLANG, bootpref[ARRAY_KBDLANG]);
		if (verbose) {
			printf ("Keyboard set to ");
			showKbdLang (readNVRAM (fd, NVRAM_KBDLANG));
		}
	}
	/* SCSI Host ID */
	if (set & SET_HOSTID) {
		writeNVRAM (fd, NVRAM_HOSTID, bootpref[ARRAY_HOSTID] |
		    HOSTID_VALID);
		if (verbose) {
			printf ("SCSI host ID set to ");
			printf ("%d\n", readNVRAM (fd, NVRAM_HOSTID) ^
				HOSTID_VALID);
		}
	}
	/* Date format/separator */
	if (set & SET_DATIME) {
		writeNVRAM (fd, NVRAM_DATIME, bootpref[ARRAY_DATIME]);
		if (verbose) {
			printf ("Date format set to ");
			showDateFmt (readNVRAM (fd, NVRAM_DATIME));
			printf ("\n");
		}
	}
	if (set & SET_DATESEP) {
		writeNVRAM (fd, NVRAM_DATESEP, bootpref[ARRAY_DATESEP]);
		if (verbose) {
			printf ("Date separator set to ");
			showDateSep (readNVRAM (fd, NVRAM_DATESEP));
		}
	}
	/* Video */
	if ((set & SET_VID2) || (set & SET_VID1)) {
		if (set & SET_VID2) {
			writeNVRAM (fd, NVRAM_VID2, bootpref[ARRAY_VID2]);
		}
		if (set & SET_VID1) {
			writeNVRAM (fd, NVRAM_VID1, bootpref[ARRAY_VID1]);
		}
		if (verbose) {
			printf ("Video set to (0x%02x, 0x%02x) :\n",
			    readNVRAM (fd, NVRAM_VID2),
			    readNVRAM (fd, NVRAM_VID1));
			showVideo2 (readNVRAM (fd, NVRAM_VID2));
			showVideo1 (readNVRAM (fd, NVRAM_VID1),
			    readNVRAM (fd, NVRAM_VID2));
		}
	}
}

static void
showOS (u_char bootos)
{
	switch (bootos) {
	case BOOTPREF_NETBSD:
		printf ("NetBSD");
		break;
	case BOOTPREF_TOS:
		printf ("TOS");
		break;
	case BOOTPREF_MAGIC:
		printf ("MAGIC");
		break;
	case BOOTPREF_LINUX:
		printf ("Linux");
		break;
	case BOOTPREF_SYSV:
		printf ("System V");
		break;
	case BOOTPREF_NONE:
		printf ("none");
		break;
	default:
		printf ("unknown");
		break;
	}
	printf (" (0x%x).\n", bootos);
}

static void
showLang (u_char lang)
{
	switch (lang) {
	case LANG_USA:
	case LANG_GB:
		printf ("English");
		break;
	case LANG_D:
		printf ("German");
		break;
	case LANG_FR:
		printf ("French");
		break;
	case LANG_ESP:
		printf ("Spanish");
		break;
	case LANG_I:
		printf ("Italian");
		break;
	default:
		printf ("unknown");
		break;
	}
	printf (" (0x%x).\n", lang);
}

static void
showKbdLang (u_char lang)
{
	switch (lang) {
	case KBDLANG_USA:
		printf ("American");
		break;
	case KBDLANG_D:
		printf ("German");
		break;
	case KBDLANG_FR:
		printf ("French");
		break;
	case KBDLANG_GB:
		printf ("British");
		break;
	case KBDLANG_ESP:
		printf ("Spanish");
		break;
	case KBDLANG_I:
		printf ("Italian");
		break;
	case KBDLANG_CHF:
		printf ("Swiss (French)");
		break;
	case KBDLANG_CHD:
		printf ("Swiss (German)");
		break;
	default:
		printf ("unknown");
		break;
	}
	printf (" (0x%x).\n", lang);
}

static void
showDateFmt (u_char fmt)
{
	if (fmt & DATIME_24H) {
		printf ("24 hour clock, ");
	} else {
		printf ("12 hour clock, ");
	}
	switch (fmt & ~DATIME_24H) {
	case DATIME_MMDDYY:
		printf ("MMDDYY");
		break;
	case DATIME_DDMMYY:
		printf ("DDMMYY");
		break;
	case DATIME_YYMMDD:
		printf ("YYMMDD");
		break;
	case DATIME_YYDDMM:
		printf ("YYDDMM");
		break;
	default:
		printf ("unknown");
		break;
	}
	printf (" (0x%02x)\n", fmt);
}

static void
showDateSep (u_char sep)
{
	if (sep) {
		if (sep >= 0x20) {
			printf ("\"%c\" ", sep);
		}
	} else {
		printf ("\"/\" ");
	}
	printf ("(0x%02x)\n", sep);
}

static void
showVideo2 (u_char vid2)
{
	u_char colours;

	colours = vid2 & 0x07;
	printf ("\t");
	switch (colours) {
	case VID2_2COL:
		printf ("2");
		break;
	case VID2_4COL:
		printf ("4");
		break;
	case VID2_16COL:
		printf ("16");
		break;
	case VID2_256COL:
		printf ("256");
		break;
	case VID2_65535COL:
		printf ("65535");
		break;
	}
	printf (" colours, ");
	if (vid2 & VID2_80CLM) {
		printf ("80");
	} else {
		printf ("40");
	}
	printf (" column, ");
	if (vid2 & VID2_VGA) {
		printf ("VGA");
	} else {
		printf ("TV");
	}
	printf (", ");
	if (vid2 & VID2_PAL) {
		printf ("PAL\n");
	} else {
		printf ("NTSC\n");
	}
	printf ("\tOverscan ");
	if (vid2 & VID2_OVERSCAN) {
		printf ("on\n");
	} else {
		printf ("off\n");
	}
	printf ("\tST compatibility ");
	if (vid2 & VID2_COMPAT) {
		printf ("on\n");
	} else {
		printf ("off\n");
	}
}

static void
showVideo1 (u_char vid1, u_char vid2)
{
	if (vid2 & VID2_VGA) {
		printf ("\tDouble line ");
		if (vid1 & VID1_INTERLACE) {
			printf ("on");
		} else {
			printf ("off");
		}
	} else {
		printf ("\tInterlace ");
		if (vid1 & VID1_INTERLACE) {
			printf ("on");
		} else {
			printf ("off");
		}
	}
	printf ("\n");
}

static int
checkOS (u_char *val, char *str)
{
	if (!strncasecmp (str, "ne", 2)) {
		*val = BOOTPREF_NETBSD;
		return (1);
	}
	if (!strncasecmp (str, "t", 1)) {
		*val = BOOTPREF_TOS;
		return (1);
	}
	if (!strncasecmp (str, "m", 1)) {
		*val = BOOTPREF_MAGIC;
		return (1);
	}
	if (!strncasecmp (str, "l", 1)) {
		*val = BOOTPREF_LINUX;
		return (1);
	}
	if (!strncasecmp (str, "s", 1)) {
		*val = BOOTPREF_SYSV;
		return (1);
	}
	if (!strncasecmp (str, "no", 2)) {
		*val = BOOTPREF_NONE;
		return (1);
	}
	return (0);
}

static int
checkLang (u_char *val, char *str)
{
	if (!strncasecmp (str, "e", 1)) {
		*val = LANG_GB;
		return (1);
	}
	if (!strncasecmp (str, "g", 1)) {
		*val = LANG_D;
		return (1);
	}
	if (!strncasecmp (str, "f", 1)) {
		*val = LANG_FR;
		return (1);
	}
	if (!strncasecmp (str, "s", 1)) {
		*val = LANG_ESP;
		return (1);
	}
	if (!strncasecmp (str, "i", 1)) {
		*val = LANG_I;
		return (1);
	}
	return (0);
}

static int
checkKbdLang (u_char *val, char *str)
{
	if (!strncasecmp (str, "a", 1)) {
		*val = KBDLANG_USA;
		return (1);
	}
	if (!strncasecmp (str, "g", 1)) {
		*val = KBDLANG_D;
		return (1);
	}
	if (!strncasecmp (str, "f", 1)) {
		*val = KBDLANG_FR;
		return (1);
	}
	if (!strncasecmp (str, "b", 1)) {
		*val = KBDLANG_GB;
		return (1);
	}
	if (!strncasecmp (str, "sp", 2)) {
		*val = KBDLANG_ESP;
		return (1);
	}
	if (!strncasecmp (str, "i", 1)) {
		*val = KBDLANG_I;
		return (1);
	}
	if (!strncasecmp (str, "swiss f", 7) || !strncasecmp (str, "sw f", 4)) {
		*val = KBDLANG_CHF;
		return (1);
	}
	if (!strncasecmp (str, "swiss g", 7) || !strncasecmp (str, "sw g", 4)) {
		*val = KBDLANG_CHD;
		return (1);
	}
	return (0);
}

static int
checkInt (u_char *val, char *str, int min, int max)
{
	int num;
	if (1 == sscanf (str, "%d", &num) && num >= min && num <= max) {
		*val = num;
		return (1);
	}
	return (0);
}

static int
checkDateFmt (u_char *val, char *str)
{
	if (!strncasecmp (str, "m", 1)) {
		*val |= DATIME_MMDDYY;
		return (1);
	}
	if (!strncasecmp (str, "d", 1)) {
		*val |= DATIME_DDMMYY;
		return (1);
	}
	if (!strncasecmp (str, "yym", 3)) {
		*val |= DATIME_YYMMDD;
		return (1);
	}
	if (!strncasecmp (str, "yyd", 3)) {
		*val |= DATIME_YYDDMM;
		return (1);
	}
	return (0);
}

static void
checkDateSep (u_char *val, char *str)
{
	if (str[0] == '/') {
		*val = 0;
	} else {
		*val = str[0];
	}
}
	
static int
checkColours (u_char *val, char *str)
{
	*val &= ~0x07;
	if (!strncasecmp (str, "6", 1)) {
		*val |= VID2_65535COL;
		return (1);
	}
	if (!strncasecmp (str, "25", 2)) {
		*val |= VID2_256COL;
		return (1);
	}
	if (!strncasecmp (str, "1", 1)) {
		*val |= VID2_16COL;
		return (1);
	}
	if (!strncasecmp (str, "4", 1)) {
		*val |= VID2_4COL;
		return (1);
	}
	if (!strncasecmp (str, "2", 1)) {
		*val |= VID2_2COL;
		return (1);
	}
	return (0);
}
