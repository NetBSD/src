/* $NetBSD: wsconscfg.c,v 1.12 2005/07/12 08:48:20 wiz Exp $ */

/*
 * Copyright (c) 1999
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <err.h>
#include <errno.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>

#define DEFDEV "/dev/ttyEcfg"

static void usage(void) __attribute__((__unused__));
int main(int, char **);

static void
usage(void)
{

	(void)fprintf(stderr,
	     "Usage: %s [-e emul] [-f ctldev] [-t type] index\n"
	     "       %s -d [-F] [-f ctldev] index\n"
	     "       %s -k | -m [-d] [-f ctldev] [index]\n"
	     "       %s -s [-f ctldev] index\n", getprogname(),
	     getprogname(), getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *wsdev;
	int c, delete, kbd, idx, wsfd, swtch, mux;
	struct wsdisplay_addscreendata asd;
	struct wsdisplay_delscreendata dsd;
	struct wsmux_device wmd;

	setprogname(argv[0]);
	wsdev = DEFDEV;
	delete = 0;
	kbd = 0;
	mux = 0;
	swtch = 0;
	idx = -1;
	asd.screentype = 0;
	asd.emul = 0;
	dsd.flags = 0;

	while ((c = getopt(argc, argv, "f:dkmst:e:F")) != -1) {
		switch (c) {
		case 'f':
			wsdev = optarg;
			break;
		case 'd':
			delete++;
			break;
		case 'k':
			kbd++;
			break;
		case 'm':
			mux++;
			kbd++;
			break;
		case 's':
			swtch++;
			break;
		case 't':
			asd.screentype = optarg;
			break;
		case 'e':
			asd.emul = optarg;
			break;
		case 'F':
			dsd.flags |= WSDISPLAY_DELSCR_FORCE;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if ((kbd || swtch) ? (argc > 1) : (argc != 1))
		usage();

	if (argc > 0 && sscanf(argv[0], "%d", &idx) != 1)
		errx(1, "invalid index");

	if ((wsfd = open(wsdev, O_RDWR)) == -1)
		err(EXIT_FAILURE, "Cannot open `%s'", wsdev);

	if (swtch) {
		if (ioctl(wsfd, VT_ACTIVATE, idx) == -1)
		    err(EXIT_FAILURE, "Cannot switch to %d", idx);
	} else if (kbd) {
		wmd.type = mux ? WSMUX_MUX : WSMUX_KBD;
		wmd.idx = idx;
		if (delete) {
			if (ioctl(wsfd, WSMUX_REMOVE_DEVICE, &wmd) == -1)
				err(EXIT_FAILURE, "WSMUX_REMOVE_DEVICE");
		} else {
			if (ioctl(wsfd, WSMUX_ADD_DEVICE, &wmd) == -1)
				err(EXIT_FAILURE, "WSMUX_ADD_DEVICE");
		}
	} else if (delete) {
		dsd.idx = idx;
		if (ioctl(wsfd, WSDISPLAYIO_DELSCREEN, &dsd) == -1)
			err(EXIT_FAILURE, "WSDISPLAYIO_DELSCREEN");
	} else {
		asd.idx = idx;
		if (ioctl(wsfd, WSDISPLAYIO_ADDSCREEN, &asd) == -1) {
			if (errno == EBUSY)
				errx(EXIT_FAILURE,
				    "screen %d is already configured", idx);
			else
				err(EXIT_FAILURE, "WSDISPLAYIO_ADDSCREEN");
		}
	}

	return 0;
}
