/* $NetBSD: videoctl.c,v 1.1 2010/12/26 10:37:15 jmcneill Exp $ */

/*-
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2010\
 Jared D. McNeill <jmcneill@invisible.ca>. All rights reserved.");
__RCSID("$NetBSD: videoctl.c,v 1.1 2010/12/26 10:37:15 jmcneill Exp $");

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/videoio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

static void		usage(void);
static void		video_print(const char *);
static void		video_print_all(void);
static bool		video_print_caps(const char *);
static bool		video_print_formats(const char *);
static bool		video_print_inputs(const char *);
static bool		video_print_audios(const char *);
static bool		video_print_standards(const char *);
static bool		video_print_tuners(const char *);
static bool		video_print_ctrl(uint32_t);
static void		video_set(const char *);
static bool		video_set_ctrl(uint32_t, int32_t);
static const char *	video_cid2name(uint32_t);
static uint32_t		video_name2cid(const char *);

static const char *video_dev = NULL;
static int video_fd = -1;
static bool aflag = false;
static bool wflag = false;

static const struct {
	uint32_t	id;
	const char	*name;
} videoctl_cid_names[] = {
	{ V4L2_CID_BRIGHTNESS,		"brightness" },
	{ V4L2_CID_CONTRAST,		"contrast" },
	{ V4L2_CID_SATURATION,		"saturation" },
	{ V4L2_CID_HUE,			"hue" },
	{ V4L2_CID_AUDIO_VOLUME,	"audio_volume" },
	{ V4L2_CID_AUDIO_BALANCE,	"audio_balance" },
	{ V4L2_CID_AUDIO_BASS,		"audio_bass" },
	{ V4L2_CID_AUDIO_TREBLE,	"audio_treble" },
	{ V4L2_CID_AUDIO_MUTE,		"audio_mute" },
	{ V4L2_CID_AUDIO_LOUDNESS,	"audio_loudness" },
	{ V4L2_CID_BLACK_LEVEL,		"black_level" },
	{ V4L2_CID_AUTO_WHITE_BALANCE,	"auto_white_balance" },
	{ V4L2_CID_DO_WHITE_BALANCE,	"do_white_balance" },
	{ V4L2_CID_RED_BALANCE,		"red_balance" },
	{ V4L2_CID_BLUE_BALANCE,	"blue_balance" },
	{ V4L2_CID_GAMMA,		"gamma" },
	{ V4L2_CID_WHITENESS,		"whiteness" },
	{ V4L2_CID_EXPOSURE,		"exposure" },
	{ V4L2_CID_AUTOGAIN,		"autogain" },
	{ V4L2_CID_GAIN,		"gain" },
	{ V4L2_CID_HFLIP,		"hflip" },
	{ V4L2_CID_VFLIP,		"vflip" },
	{ V4L2_CID_HCENTER,		"hcenter" },
	{ V4L2_CID_VCENTER,		"vcenter" },
	{ V4L2_CID_POWER_LINE_FREQUENCY, "power_line_frequency" },
	{ V4L2_CID_HUE_AUTO,		"hue_auto" },
	{ V4L2_CID_WHITE_BALANCE_TEMPERATURE, "white_balance_temperature" },
	{ V4L2_CID_SHARPNESS,		"sharpness" },
	{ V4L2_CID_BACKLIGHT_COMPENSATION, "backlight_compensation" },
};

int
main(int argc, char *argv[])
{
	int ch;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "ad:w")) != -1) {
		switch (ch) {
		case 'a':
			aflag = true;
			break;
		case 'd':
			video_dev = strdup(optarg);
			break;
		case 'w':
			wflag = true;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (wflag && aflag)
		usage();
		/* NOTREACHED */
	if (wflag && argc == 0)
		usage();
		/* NOTREACHED */
	if (aflag && argc > 0)
		usage();
		/* NOTREACHED */
	if (!wflag && !aflag && argc == 0)
		usage();
		/* NOTREACHED */

	if (video_dev == NULL)
		video_dev = _PATH_VIDEO0;

	video_fd = open(video_dev, wflag ? O_RDWR : O_RDONLY);
	if (video_fd == -1)
		err(EXIT_FAILURE, "couldn't open '%s'", video_dev);

	if (aflag) {
		video_print_all();
	} else if (wflag) {
		while (argc > 0) {
			video_set(argv[0]);
			--argc;
			++argv;
		}
	} else {
		while (argc > 0) {
			video_print(argv[0]);
			--argc;
			++argv;
		}
	}

	close(video_fd);

	return EXIT_SUCCESS;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-d file] name ...\n", getprogname());
	fprintf(stderr, "usage: %s [-d file] -w name=value ...\n",
	    getprogname());
	fprintf(stderr, "usage: %s [-d file] -a\n", getprogname());
	exit(EXIT_FAILURE);
}

static void
video_print_all(void)
{
	video_print_caps(NULL);
	video_print_formats(NULL);
	video_print_inputs(NULL);
	video_print_audios(NULL);
	video_print_standards(NULL);
	video_print_tuners(NULL);
	video_print_ctrl(0);
}

static bool
video_print_caps(const char *name)
{
	struct v4l2_capability cap;
	char capbuf[128];
	int error;
	bool found = false;

	if (strtok(NULL, ".") != NULL)
		return false;

	/* query capabilities */
	error = ioctl(video_fd, VIDIOC_QUERYCAP, &cap);
	if (error == -1)
		err(EXIT_FAILURE, "VIDIOC_QUERYCAP failed");

	if (!name || strcmp(name, "card") == 0) {
		printf("info.cap.card=%s\n", cap.card);
		found = true;
	}
	if (!name || strcmp(name, "driver") == 0) {
		printf("info.cap.driver=%s\n", cap.driver);
		found = true;
	}
	if (!name || strcmp(name, "bus_info") == 0) {
		printf("info.cap.bus_info=%s\n", cap.bus_info);
		found = true;
	}
	if (!name || strcmp(name, "version") == 0) {
		printf("info.cap.version=%u.%u.%u\n",
		    (cap.version >> 16) & 0xff,
		    (cap.version >> 8) & 0xff,
		    cap.version & 0xff);
		found = true;
	}
	if (!name || strcmp(name, "capabilities") == 0) {
		snprintb(capbuf, sizeof(capbuf), V4L2_CAP_BITMASK,
		    cap.capabilities);
		printf("info.cap.capabilities=%s\n", capbuf);
		found = true;
	}

	return found;
}

static bool
video_print_formats(const char *name)
{
	struct v4l2_fmtdesc fmtdesc;
	int error;

	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (name == NULL) {
		/* enumerate formats */
		for (fmtdesc.index = 0; ; fmtdesc.index++) {
			error = ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc);
			if (error)
				break;
			printf("info.format.%u=%s\n", fmtdesc.index,
			    fmtdesc.description);
		}
	} else {
		unsigned long n;

		if (strtok(NULL, ".") != NULL)
			return false;

		n = strtoul(name, NULL, 10);
		if (n == ULONG_MAX)
			return false;
		fmtdesc.index = n;
		error = ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (error)
			return false;
		printf("info.format.%u=%s\n", fmtdesc.index,
		    fmtdesc.description);
	}

	return true;
}

static bool
video_print_inputs(const char *name)
{
	struct v4l2_input input;
	int error;

	if (name == NULL) {
		/* enumerate inputs */
		for (input.index = 0; ; input.index++) {
			error = ioctl(video_fd, VIDIOC_ENUMINPUT, &input);
			if (error)
				break;
			printf("info.input.%u=%s\n", input.index, input.name);
			printf("info.input.%u.type=", input.index);
			switch (input.type) {
			case V4L2_INPUT_TYPE_TUNER:
				printf("tuner\n");
				break;
			case V4L2_INPUT_TYPE_CAMERA:
				printf("baseband\n");
				break;
			default:
				printf("unknown (%d)\n", input.type);
				break;
			}
		}
	} else {
		unsigned long n;
		char *s;

		n = strtoul(name, NULL, 10);
		if (n == ULONG_MAX)
			return false;
		input.index = n;
		error = ioctl(video_fd, VIDIOC_ENUMINPUT, &input);
		if (error)
			return false;

		s = strtok(NULL, ".");
		if (s == NULL) {
			printf("info.input.%u=%s\n", input.index, input.name);
		} else if (strcmp(s, "type") == 0) {
			if (strtok(NULL, ".") != NULL)
				return false;
			printf("info.input.%u.type=", input.index);
			switch (input.type) {
			case V4L2_INPUT_TYPE_TUNER:
				printf("tuner\n");
				break;
			case V4L2_INPUT_TYPE_CAMERA:
				printf("baseband\n");
				break;
			default:
				printf("unknown (%d)\n", input.type);
				break;
			}
		} else
			return false;
	}

	return true;
}

static bool
video_print_audios(const char *name)
{
	struct v4l2_audio audio;
	int error;

	if (name == NULL) {
		/* enumerate audio */
		for (audio.index = 0; ; audio.index++) {
			error = ioctl(video_fd, VIDIOC_ENUMAUDIO, &audio);
			if (error)
				break;
			printf("info.audio.%u=%s\n", audio.index, audio.name);
			printf("info.audio.%u.stereo=%d\n", audio.index,
			    audio.capability & V4L2_AUDCAP_STEREO ? 1 : 0);
			printf("info.audio.%u.avl=%d\n", audio.index,
			    audio.capability & V4L2_AUDCAP_AVL ? 1 : 0);
		}
	} else {
		unsigned long n;
		char *s;

		n = strtoul(name, NULL, 10);
		if (n == ULONG_MAX)
			return false;
		audio.index = n;
		error = ioctl(video_fd, VIDIOC_ENUMAUDIO, &audio);
		if (error)
			return false;

		s = strtok(NULL, ".");
		if (s == NULL) {
			printf("info.audio.%u=%s\n", audio.index, audio.name);
		} else if (strcmp(s, "stereo") == 0) {
			if (strtok(NULL, ".") != NULL)
				return false;
			printf("info.audio.%u.stereo=%d\n", audio.index,
			    audio.capability & V4L2_AUDCAP_STEREO ? 1 : 0);
		} else if (strcmp(s, "avl") == 0) {
			if (strtok(NULL, ".") != NULL)
				return false;
			printf("info.audio.%u.avl=%d\n", audio.index,
			    audio.capability & V4L2_AUDCAP_AVL ? 1 : 0);
		} else
			return false;
	}

	return true;
}

static bool
video_print_standards(const char *name)
{
	struct v4l2_standard std;
	int error;

	if (name == NULL) {
		/* enumerate standards */
		for (std.index = 0; ; std.index++) {
			error = ioctl(video_fd, VIDIOC_ENUMSTD, &std);
			if (error)
				break;
			printf("info.standard.%u=%s\n", std.index, std.name);
		}
	} else {
		unsigned long n;

		if (strtok(NULL, ".") != NULL)
			return false;

		n = strtoul(name, NULL, 10);
		if (n == ULONG_MAX)
			return false;
		std.index = n;
		error = ioctl(video_fd, VIDIOC_ENUMSTD, &std);
		if (error)
			return false;
		printf("info.standard.%u=%s\n", std.index, std.name);
	}

	return true;
}

static bool
video_print_tuners(const char *name)
{
	struct v4l2_tuner tuner;
	int error;

	if (name == NULL) {
		/* enumerate tuners */
		for (tuner.index = 0; ; tuner.index++) {
			error = ioctl(video_fd, VIDIOC_G_TUNER, &tuner);
			if (error)
				break;
			printf("info.tuner.%u=%s\n", tuner.index, tuner.name);
		}
	} else {
		unsigned long n;

		if (strtok(NULL, ".") != NULL)
			return false;

		n = strtoul(name, NULL, 10);
		if (n == ULONG_MAX)
			return false;
		tuner.index = n;
		error = ioctl(video_fd, VIDIOC_G_TUNER, &tuner);
		if (error)
			return false;
		printf("info.tuner.%u=%s\n", tuner.index, tuner.name);
	}

	return true;
}

static void
video_print(const char *name)
{
	char *buf, *s, *s2 = NULL;
	bool found = false;

	buf = strdup(name);
	s = strtok(buf, ".");
	if (s == NULL)
		return;

	if (strcmp(s, "info") == 0) {
		s = strtok(NULL, ".");
		if (s)
			s2 = strtok(NULL, ".");
		if (s == NULL || strcmp(s, "cap") == 0) {
			found = video_print_caps(s2);
		}
		if (s == NULL || strcmp(s, "format") == 0) {
			found = video_print_formats(s2);
		}
		if (s == NULL || strcmp(s, "input") == 0) {
			found = video_print_inputs(s2);
		}
		if (s == NULL || strcmp(s, "audio") == 0) {
			found = video_print_audios(s2);
		}
		if (s == NULL || strcmp(s, "standard") == 0) {
			found = video_print_standards(s2);
		}
		if (s == NULL || strcmp(s, "tuner") == 0) {
			found = video_print_tuners(s2);
		}
	} else if (strcmp(s, "ctrl") == 0) {
		s = strtok(NULL, ".");
		if (s)
			s2 = strtok(NULL, ".");

		if (s == NULL)
			found = video_print_ctrl(0);
		else if (s && !s2)
			found = video_print_ctrl(video_name2cid(s));
	}

	free(buf);
	if (!found)
		fprintf(stderr, "%s: field %s does not exist\n",
		    getprogname(), name);
}

static bool
video_print_ctrl(uint32_t ctrl_id)
{
	struct v4l2_control ctrl;
	const char *ctrlname;
	bool found = false;
	int error;

	for (ctrl.id = V4L2_CID_BASE; ctrl.id != V4L2_CID_LASTP1; ctrl.id++) {
		if (ctrl_id != 0 && ctrl_id != ctrl.id)
			continue;
		error = ioctl(video_fd, VIDIOC_G_CTRL, &ctrl);
		if (error)
			continue;
		ctrlname = video_cid2name(ctrl.id);
		if (ctrlname)
			printf("ctrl.%s=%d\n", ctrlname, ctrl.value);
		else
			printf("ctrl.%08x=%d\n", ctrl.id, ctrl.value);
		found = true;
	}

	return found;
}

static void
video_set(const char *name)
{
	char *buf, *key, *value;
	bool found = false;
	long n;

	if (strchr(name, '=') == NULL) {
		fprintf(stderr, "%s: No '=' in %s\n", getprogname(), name);
		exit(EXIT_FAILURE);
	}

	buf = strdup(name);
	key = strtok(buf, "=");
	if (key == NULL)
		usage();
		/* NOTREACHED */
	value = strtok(NULL, "");
	if (value == NULL)
		usage();
		/* NOTREACHED */

	if (strncmp(key, "info.", strlen("info.")) == 0) {
		fprintf(stderr, "'info' subtree read-only\n");
		found = true;
		goto done;
	}
	if (strncmp(key, "ctrl.", strlen("ctrl.")) == 0) {
		char *ctrlname = key + strlen("ctrl.");
		uint32_t ctrl_id = video_name2cid(ctrlname);

		n = strtol(value, NULL, 0);
		if (n == LONG_MIN || n == LONG_MAX)
			goto done;
		found = video_set_ctrl(ctrl_id, n);
	}

done:
	free(buf);
	if (!found)
		fprintf(stderr, "%s: field %s does not exist\n",
		    getprogname(), name);
}

static bool
video_set_ctrl(uint32_t ctrl_id, int32_t value)
{
	struct v4l2_control ctrl;
	const char *ctrlname;
	int32_t ovalue;
	int error;

	ctrlname = video_cid2name(ctrl_id);

	ctrl.id = ctrl_id;
	error = ioctl(video_fd, VIDIOC_G_CTRL, &ctrl);
	if (error)
		return false;
	ovalue = ctrl.value;
	ctrl.value = value;
	error = ioctl(video_fd, VIDIOC_S_CTRL, &ctrl);
	if (error)
		err(EXIT_FAILURE, "VIDIOC_S_CTRL failed for '%s'", ctrlname);
	error = ioctl(video_fd, VIDIOC_G_CTRL, &ctrl);
	if (error)
		err(EXIT_FAILURE, "VIDIOC_G_CTRL failed for '%s'", ctrlname);

	if (ctrlname)
		printf("ctrl.%s: %d -> %d\n", ctrlname, ovalue, ctrl.value);
	else
		printf("ctrl.%08x: %d -> %d\n", ctrl.id, ovalue, ctrl.value);

	return true;
}

static const char *
video_cid2name(uint32_t id)
{
	unsigned int i;

	for (i = 0; i < __arraycount(videoctl_cid_names); i++)
		if (videoctl_cid_names[i].id == id)
			return videoctl_cid_names[i].name;

	return NULL;
}

static uint32_t
video_name2cid(const char *name)
{
	unsigned int i;

	for (i = 0; i < __arraycount(videoctl_cid_names); i++)
		if (strcmp(name, videoctl_cid_names[i].name) == 0)
			return videoctl_cid_names[i].id;

	return (uint32_t)-1;
}
