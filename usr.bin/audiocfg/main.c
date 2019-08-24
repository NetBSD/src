/* $NetBSD: main.c,v 1.15 2019/08/24 06:11:10 isaki Exp $ */

/*
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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "audiodev.h"
#include "drvctl.h"

__dead static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr, "usage: %s list [<index>]\n", p);
	fprintf(stderr, "       %s default <index>\n", p);
	fprintf(stderr, "       %s set  <index> [p|r] <enc> <prec> <ch> <freq>\n",
	    p);
	fprintf(stderr, "       %s test <index>\n", p);
	exit(EXIT_FAILURE);
}

const char *encoding_names[] = {
	"none",
	AudioEmulaw,
	AudioEalaw,
	"pcm16",
	"pcm8",
	AudioEadpcm,
	AudioEslinear_le,
	AudioEslinear_be,
	AudioEulinear_le,
	AudioEulinear_be,
	AudioEslinear,
	AudioEulinear,
	AudioEmpeg_l1_stream,
	AudioEmpeg_l1_packets,
	AudioEmpeg_l1_system,
	AudioEmpeg_l2_stream,
	AudioEmpeg_l2_packets,
	AudioEmpeg_l2_system,
	AudioEac3,
};
u_int encoding_max = __arraycount(encoding_names);

static void
print_audiodev(struct audiodev *adev)
{
	struct audiofmt *f;
	int j;

	assert(adev != NULL);

	printf("[%c] %s @ %s: ",
	    adev->defaultdev ? '*' : ' ',
	    adev->xname, adev->pxname);
	printf("%s", adev->audio_device.name);
	if (strlen(adev->audio_device.version) > 0)
		printf(" %s", adev->audio_device.version);
	printf("\n");
	printf("       playback: ");
	if ((adev->hwinfo.mode & AUMODE_PLAY)) {
		printf("%uch, %uHz\n",
		    adev->hwinfo.play.channels,
		    adev->hwinfo.play.sample_rate);
	} else {
		printf("unavailable\n");
	}
	printf("       record:   ");
	if ((adev->hwinfo.mode & AUMODE_RECORD)) {
		printf("%uch, %uHz\n",
		    adev->hwinfo.record.channels,
		    adev->hwinfo.record.sample_rate);
	} else {
		printf("unavailable\n");
	}

	TAILQ_FOREACH(f, &adev->formats, next) {
		printf("       ");
		if (f->fmt.priority < 0)
			printf("(  ) ");
		else if ((f->fmt.mode & (AUMODE_PLAY | AUMODE_RECORD))
		    == (AUMODE_PLAY | AUMODE_RECORD))
			printf("(PR) ");
		else if ((f->fmt.mode & AUMODE_PLAY))
			printf("(P-) ");
		else if ((f->fmt.mode & AUMODE_RECORD))
			printf("(-R) ");

		if (f->fmt.encoding < encoding_max)
			printf("%s", encoding_names[f->fmt.encoding]);
		else
			printf("unknown_encoding_%d", f->fmt.encoding);
		printf(" %d/%d, %dch, ",
		    f->fmt.validbits,
		    f->fmt.precision,
		    f->fmt.channels);
		if (f->fmt.frequency_type == 0) {
			printf("%d-%dHz",
			    f->fmt.frequency[0],
			    f->fmt.frequency[1]);
		} else {
			for (j = 0; j < (int)f->fmt.frequency_type; j++) {
				printf("%s%d",
				    (j == 0) ? "{ " : ", ",
				    f->fmt.frequency[j]);
			}
			printf(" }");
		}
		printf("\n");
	}
}

/* Always return non-null adev, or exit */
static struct audiodev *
getadev_fromstr(const char *str)
{
	struct audiodev *adev;
	unsigned int i;

	if (*str < '0' || *str > '9')
		usage();
		/* NOTREACHED */
	errno = 0;
	i = strtoul(str, NULL, 10);
	if (errno)
		usage();
		/* NOTREACHED */
	adev = audiodev_get(i);
	if (adev == NULL) {
		errx(EXIT_FAILURE, "no such device");
	}
	return adev;
}

int
main(int argc, char *argv[])
{
	struct audiodev *adev;
	unsigned int n, i;
	unsigned int j;
	const char *enc;
	unsigned int prec;
	unsigned int ch;
	unsigned int freq;
	int mode;

	if (audiodev_refresh() == -1)
		return EXIT_FAILURE;

	if (argc < 2)
		usage();
		/* NOTREACHED */

	if (strcmp(argv[1], "list") == 0 && argc == 2) {
		n = audiodev_maxunit();
		for (i = 0; i <= n; i++) {
			adev = audiodev_get(i);
			if (adev)
				print_audiodev(adev);
		}
	} else if (strcmp(argv[1], "list") == 0 && argc == 3) {
		print_audiodev(getadev_fromstr(argv[2]));
	} else if (strcmp(argv[1], "default") == 0 && argc == 3) {
		adev = getadev_fromstr(argv[2]);
		printf("setting default audio device to %s\n", adev->xname);
		if (audiodev_set_default(adev) == -1) {
			errx(EXIT_FAILURE, "couldn't set default device");
		}
	} else if (strcmp(argv[1], "set") == 0 && argc == 8) {
		/* XXX bad commandline... */
		/* audiocfg set <index> [p|r] <enc> <prec> <ch> <freq> */
		adev = getadev_fromstr(argv[2]);
		mode = 0;
		for (j = 0; j < strlen(argv[3]); j++) {
			if (argv[3][j] == 'p')
				mode |= AUMODE_PLAY;
			else if (argv[3][j] == 'r')
				mode |= AUMODE_RECORD;
			else
				usage();
		}
		if (mode == 0)
			usage();
			/* NOTREACHED */
		enc = argv[4];
		prec = strtoul(argv[5], NULL, 10);
		if (errno)
			usage();
		errno = 0;
		ch = strtoul(argv[6], NULL, 10);
		if (errno)
			usage();
			/* NOTREACHED */
		errno = 0;
		freq = strtoul(argv[7], NULL, 10);
		if (errno)
			usage();
			/* NOTREACHED */

		if (audiodev_set_param(adev, mode, enc, prec, ch, freq) == -1) {
			errx(EXIT_FAILURE, "couldn't set parameter");
		}
	} else if (strcmp(argv[1], "test") == 0 && argc == 3) {
		adev = getadev_fromstr(argv[2]);
		print_audiodev(adev);
		if (audiodev_test(adev) == -1)
			return EXIT_FAILURE;
	} else
		usage();
		/* NOTREACHED */

	return EXIT_SUCCESS;
}
