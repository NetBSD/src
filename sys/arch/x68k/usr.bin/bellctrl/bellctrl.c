/*	$NetBSD: bellctrl.c,v 1.4 2003/05/17 09:24:15 isaki Exp $	*/

/*
 * bellctrl - OPM bell controller (for NetBSD/X680x0)
 * Copyright (c)1995 ussy.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include "../../include/opmbellio.h"
#include "../../dev/opmbellvar.h"
#include "../../dev/opmreg.h"

#define DEFAULT -1

#define nextarg(i, argv) \
	argv[i]; \
	if (i >= argc) \
		break; \

int bell_setting;
char *progName;
struct opm_voice voice;

static struct bell_info values = {
    DEFAULT, DEFAULT, DEFAULT
};

/* function prototype */
int is_number();
void set_bell_vol();
void set_bell_pitch();
void set_bell_dur();
void set_voice_param();
void set_bell_param();
int usage();
int error();

int
main(argc, argv)
int argc;
char **argv;
{
    register char *arg;
    int percent;
    int i;

    progName = argv[0];
    bell_setting = 0;

    if (argc < 2) {
	usage(NULL, NULL);
    }

    for (i = 1; i < argc; ) {
	arg = argv[i++];
	if (strcmp(arg, "-b") == 0) {
	    set_bell_vol(0);		/* turn off bell */
	} else if (strcmp(arg, "b") == 0) {
	    percent = DEFAULT;		/* set bell to default */
	    if (i >= argc) {
		set_bell_vol(percent);		/* set bell to default */
		set_bell_pitch(percent);	/* set pitch to default */
		set_bell_dur(percent);		/* set duration to default */
		break;
	    }
	    arg = nextarg(i, argv);
	    if (strcmp(arg, "on") == 0) {	/* let it stay that way */
		set_bell_vol(BELL_VOLUME);	/* set bell on */
		set_bell_pitch(BELL_PITCH);	/* set pitch to default */
		set_bell_dur(BELL_DURATION);	/* set duration to default */
		i++;
	    } else if (strcmp(arg, "off") == 0) {
		percent = 0;			/* turn the bell off */
		set_bell_vol(percent);
		i++;
	    } else if (is_number(arg, MAXBVOLUME)) {	/* If volume is given */
		percent = atoi(arg);		/* set bell appropriately */
		set_bell_vol(percent);
		i++;

		arg = nextarg(i, argv);
		if (is_number(arg, MAXBPITCH)) {	/* if pitch is given */
		    set_bell_pitch(atoi(arg));		/* set the bell */
		    i++;

		    arg = nextarg(i, argv);
		    if (is_number(arg, MAXBTIME)) {	/* If duration is given	*/
			set_bell_dur(atoi(arg));	/* set the bell */
			i++;
		    }
		}
	    } else {
		set_bell_vol(BELL_VOLUME);		/* set bell to default */
	    }
	} else if (strcmp(arg, "v") == 0) {	/* set voice parameter */
	    if (i >= argc) {
		arg = "default";
	    } else {
		arg = nextarg(i, argv);
	    }
	    set_voice_param(arg, 1);
	    i++;
	} else if (strcmp(arg, "-v") == 0) {	/* set voice parameter */
	    if (i >= argc) {
		usage("missing -v argument", NULL);
	    }
	    arg = nextarg(i, argv);
	    set_voice_param(arg, 0);
	    i++;
	} else {
	    usage("unknown option %s", arg);
	}
    }

    if (bell_setting)
	set_bell_param();

    exit(0);
}

int
is_number(arg, maximum)
char *arg;
int maximum;
{
    register char *p;

    if (arg[0] == '-' && arg[1] == '1' && arg[2] == '\0')
	return 1;
    for (p = arg; isdigit((unsigned char)*p); p++)
	;
    if (*p || atoi(arg) > maximum)
	return 0;

    return 1;
}

void
set_bell_vol(percent)
int percent;
{
    values.volume = percent;
    bell_setting++;
}

void
set_bell_pitch(pitch)
int pitch;
{
    values.pitch = pitch;
    bell_setting++;
}

void
set_bell_dur(duration)
int duration;
{
    values.msec = duration;
    bell_setting++;
}

void
set_voice_param(path, flag)
char *path;
int flag;
{
    int fd;

    if (flag) {
	memcpy(&voice, &bell_voice, sizeof(bell_voice));
    } else {
	if ((fd = open(path, 0)) >= 0) {
	    if (read(fd, &voice, sizeof(voice)) != sizeof(voice)) {
		error("cannot read voice parameter.");
	    }
	    close(fd);
	} else {
	    error("cannot open voice parameter.");
	}
    }

    if ((fd = open("/dev/bell", O_RDWR)) < 0) {
	error("cannot open /dev/bell");
    }
    if (ioctl(fd, BELLIOCSVOICE, &voice)) {
	error("ioctl BELLIOCSVOICE failed");
    }
    close(fd);
}

void
set_bell_param(void)
{
    int fd;
    struct bell_info param;

    if ((fd = open("/dev/bell", O_RDWR)) < 0) {
	error("cannot open /dev/bell");
    }
    if (ioctl(fd, BELLIOCGPARAM, &param)) {
	error("ioctl BELLIOCGPARAM failed.");
    }

    if (values.volume == DEFAULT) {
	values.volume = param.volume;
    }
    if (values.pitch == DEFAULT) {
	values.pitch = param.pitch;
    }
    if (values.msec == DEFAULT) {
	values.msec = param.msec;
    }

    if (ioctl(fd, BELLIOCSPARAM, &values)) {
	error("ioctl BELLIOCSPARAM failed.");
    }

    close(fd);
}

int
usage(fmt, arg)
char *fmt;
char *arg;
{
    if (fmt) {
	fprintf (stderr, "%s:  ", progName);
	fprintf (stderr, fmt, arg);
	fprintf (stderr, "\n\n");
    }

    fprintf(stderr, "usage:  %s option ...\n", progName);
    fprintf(stderr, "    To turn bell off:\n");
    fprintf(stderr, "\t-b                b off               b 0\n");
    fprintf(stderr, "    To set bell volume, pitch and duration:\n");
    fprintf(stderr, "\t b [vol [pitch [dur]]]          b on\n");
    fprintf(stderr, "    To restore default voice parameter:\n");
    fprintf(stderr, "\t v default\n");
    fprintf(stderr, "    To set voice parameter:\n");
    fprintf(stderr, "\t-v voicefile\n");
    exit(0);
}

int
error(message)
char *message;
{
    fprintf(stderr, "%s: %s\n", progName, message);
    exit(1);
}

