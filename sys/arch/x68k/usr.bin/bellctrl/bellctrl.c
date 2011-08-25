/*	$NetBSD: bellctrl.c,v 1.13 2011/08/25 17:00:55 christos Exp $	*/

/*
 * bellctrl - OPM bell controller (for NetBSD/X680x0)
 * Copyright (c)1995 ussy.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: bellctrl.c,v 1.13 2011/08/25 17:00:55 christos Exp $");

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <machine/opmbellio.h>

#define DEFAULT -1

#define nextarg(i, argv) \
	argv[i]; \
	if (i >= argc) \
		break; \

static int bell_setting;
static struct opm_voice voice;

static struct opm_voice bell_voice = DEFAULT_BELL_VOICE;

static struct bell_info values = {
	DEFAULT, DEFAULT, DEFAULT
};

/* function prototype */
static int is_number(const char *, int);
static void set_bell_vol(int);
static void set_bell_pitch(int);
static void set_bell_dur(int);
static void set_voice_param(const char *, int);
static void set_bell_param(void);
static void usage(void) __dead;

int
main(int argc, char **argv)
{
	const char *arg;
	int percent;
	int i;

	bell_setting = 0;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; ) {
		arg = argv[i++];
		if (strcmp(arg, "-b") == 0) {
			/* turn off bell */
			set_bell_vol(0);
		} else if (strcmp(arg, "b") == 0) {
			/* set bell to default */
			percent = DEFAULT;

			if (i >= argc) {
				/* set bell to default */
				set_bell_vol(percent);
				/* set pitch to default */
				set_bell_pitch(percent);
				/* set duration to default */
				set_bell_dur(percent);
				break;
			}
			arg = nextarg(i, argv);
			if (strcmp(arg, "on") == 0) {
				/*
				 * let it stay that way
				 */
				/* set bell on */
				set_bell_vol(BELL_VOLUME);
				/* set pitch to default */
				set_bell_pitch(BELL_PITCH);
				/* set duration to default */
				set_bell_dur(BELL_DURATION);
				i++;
			} else if (strcmp(arg, "off") == 0) {
				/* turn the bell off */
				percent = 0;
				set_bell_vol(percent);
				i++;
			} else if (is_number(arg, MAXBVOLUME)) {
				/*
				 * If volume is given
				 */
				/* set bell appropriately */
				percent = atoi(arg);

				set_bell_vol(percent);
				i++;

				arg = nextarg(i, argv);

				/* if pitch is given */
				if (is_number(arg, MAXBPITCH)) {
					/* set the bell */
					set_bell_pitch(atoi(arg));
					i++;

					arg = nextarg(i, argv);
					/* If duration is given	*/
					if (is_number(arg, MAXBTIME)) {
						/* set the bell */
						set_bell_dur(atoi(arg));
						i++;
					}
				}
			} else {
				/* set bell to default */
				set_bell_vol(BELL_VOLUME);
			}
		} else if (strcmp(arg, "v") == 0) {
			/*
			 * set voice parameter
			 */
			if (i >= argc) {
				arg = "default";
			} else {
				arg = nextarg(i, argv);
			}
			set_voice_param(arg, 1);
			i++;
		} else if (strcmp(arg, "-v") == 0) {
			/*
			 * set voice parameter
			 */
			if (i >= argc) {
				warnx("Missing -v argument");
				usage();
			}
			arg = nextarg(i, argv);
			set_voice_param(arg, 0);
			i++;
		} else {
			warnx("Unknown option %s", arg);
			usage();
		}
	}

	if (bell_setting)
		set_bell_param();

	exit(0);
}

static int
is_number(const char *arg, int maximum)
{
	const char *p;

	if (arg[0] == '-' && arg[1] == '1' && arg[2] == '\0')
		return 1;
	for (p = arg; isdigit((unsigned char)*p); p++)
		;
	if (*p || atoi(arg) > maximum)
		return 0;

	return 1;
}

static void
set_bell_vol(int percent)
{
	values.volume = percent;
	bell_setting++;
}

static void
set_bell_pitch(int pitch)
{
	values.pitch = pitch;
	bell_setting++;
}

static void
set_bell_dur(int duration)
{
	values.msec = duration;
	bell_setting++;
}

static void
set_voice_param(const char *path, int flag)
{
	int fd;

	if (flag) {
		memcpy(&voice, &bell_voice, sizeof(bell_voice));
	} else {
		if ((fd = open(path, 0)) >= 0) {
			if (read(fd, &voice, sizeof(voice)) != sizeof(voice))
				err(1, "cannot read voice parameter");
			close(fd);
		} else {
			err(1, "cannot open voice parameter");
		}
	}

	if ((fd = open("/dev/bell", O_RDWR)) < 0)
		err(1, "cannot open /dev/bell");
	if (ioctl(fd, BELLIOCSVOICE, &voice))
		err(1, "ioctl BELLIOCSVOICE failed");

	close(fd);
}

static void
set_bell_param(void)
{
	int fd;
	struct bell_info param;

	if ((fd = open("/dev/bell", O_RDWR)) < 0)
		err(1, "cannot open /dev/bell");
	if (ioctl(fd, BELLIOCGPARAM, &param))
		err(1, "ioctl BELLIOCGPARAM failed");

	if (values.volume == DEFAULT)
		values.volume = param.volume;
	if (values.pitch == DEFAULT)
		values.pitch = param.pitch;
	if (values.msec == DEFAULT)
		values.msec = param.msec;

	if (ioctl(fd, BELLIOCSPARAM, &values))
		err(1, "ioctl BELLIOCSPARAM failed");

	close(fd);
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s option ...\n", getprogname());
	fprintf(stderr, "	To turn bell off:\n");
	fprintf(stderr, "\t-b				b off"
	                "			   b 0\n");
	fprintf(stderr, "	To set bell volume, pitch and duration:\n");
	fprintf(stderr, "\t b [vol [pitch [dur]]]		  b on\n");
	fprintf(stderr, "	To restore default voice parameter:\n");
	fprintf(stderr, "\t v default\n");
	fprintf(stderr, "	To set voice parameter:\n");
	fprintf(stderr, "\t-v voicefile\n");
	exit(0);
}
