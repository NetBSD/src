/*	$NetBSD: btkey.c,v 1.2.4.1 2009/12/18 06:01:38 snj Exp $	*/

/*-
 * Copyright (c) 2007 Iain Hibbert
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
 * 3. The name of the author may not be used to endorse or promote products
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
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2007 Iain Hibbert.  All rights reserved.");
__RCSID("$NetBSD: btkey.c,v 1.2.4.1 2009/12/18 06:01:38 snj Exp $");

#include <bluetooth.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btkey.h"

static void usage(void);
static bool scan_key(const char *);

bdaddr_t laddr;
bdaddr_t raddr;
uint8_t key[HCI_KEY_SIZE];

int
main(int ac, char *av[])
{
	struct hostent *he;
	int ch;
	bool cf, cd, lf, ld, rf, rd, wf, wd, nk;

	memset(&laddr, 0, sizeof(laddr));
	memset(&raddr, 0, sizeof(raddr));
	memset(key, 0, sizeof(key));
	cf = cd = lf = ld = rf = rd = wf = wd = nk = false;

	while ((ch = getopt(ac, av, "a:cCd:k:lLrRwW")) != EOF) {
		switch (ch) {
		case 'a':	/* remote device address */
			if (!bt_aton(optarg, &raddr)) {
				he = bt_gethostbyname(optarg);
				if (he == NULL)
					errx(EXIT_FAILURE, "%s: %s",
					    optarg, hstrerror(h_errno));

				bdaddr_copy(&raddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'c':	/* clear from file */
			cf = true;
			break;

		case 'C':	/* clear from device */
			cd = true;
			break;

		case 'd':	/* local device address */
			if (!bt_devaddr(optarg, &laddr)
			    && !bt_aton(optarg, &laddr)) {
				he = bt_gethostbyname(optarg);
				if (he == NULL)
					errx(EXIT_FAILURE, "%s: %s",
					    optarg, hstrerror(h_errno));

				bdaddr_copy(&laddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'k':	/* new link key */
			if (!scan_key(optarg))
				errx(EXIT_FAILURE, "invalid key '%s'", optarg);

			nk = true;
			break;

		case 'l':	/* list from file */
			lf = true;
			break;

		case 'L':	/* list from device */
			ld = true;
			break;

		case 'r':	/* read from file */
			rf = true;
			break;

		case 'R':	/* read from device */
			rd = true;
			break;

		case 'w':	/* write to file */
			wf = true;
			break;

		case 'W':	/* write to device */
			wd = true;
			break;

		default:
			usage();
		}
	}

	ac -= optind;
	av += optind;

	/*
	 * validate options
	 */
	if ((lf || ld) && (rf || rd || wf || wd || cf || cd || nk))
		errx(EXIT_FAILURE, "list is exclusive of other options");

	if (((rf && rd) || (rf && nk) || (rd && nk)) && (wf || wd))
		errx(EXIT_FAILURE, "too many key sources");

	if (((bdaddr_any(&laddr) || bdaddr_any(&raddr)) && !(lf || ld))
	    || ((lf || ld) && (bdaddr_any(&laddr) || !bdaddr_any(&raddr)))
	    || ac > 0)
		usage();

	/*
	 * do what we gotta do and be done
	 */
	if (!bdaddr_any(&laddr))
		print_addr("device", &laddr);

	if (!bdaddr_any(&raddr))
		print_addr("bdaddr", &raddr);

	if (lf && !list_file())
		err(EXIT_FAILURE, "list file");

	if (ld && !list_device())
		err(EXIT_FAILURE, "list device");

	if (nk)
		print_key("new key", key);

	if (rf) {
		if (!read_file())
			err(EXIT_FAILURE, "file key");

		print_key("file key", key);
	}

	if (rd) {
		if (!read_device())
			err(EXIT_FAILURE, "device key");

		print_key("device key", key);
	}

	if (wf || wd || cf || cd)
		printf("\n");

	if (wf) {
		if (!write_file())
			err(EXIT_FAILURE, "write to file");

		printf("written to file\n");
	}

	if (wd) {
		if (!write_device())
			err(EXIT_FAILURE, "write to device");

		printf("written to device\n");
	}

	if (cf) {
		if (!clear_file())
			err(EXIT_FAILURE, "clear from file");

		printf("cleared from file\n");
	}

	if (cd) {
		if (!clear_device())
			err(EXIT_FAILURE, "clear from device");

		printf("cleared from device\n");
	}

	exit(EXIT_SUCCESS);
}

static void
usage(void)
{

	fprintf(stderr,
		"Usage: %s [-cCrRwW] [-k key] -a address -d device\n"
		"       %s -lL -d device\n"
		"\n", getprogname(), getprogname());

	fprintf(stderr,
		"Where:\n"
		"\t-a address  remote device address\n"
		"\t-c          clear from file\n"
		"\t-C          clear from device\n"
		"\t-d device   local device address\n"
		"\t-k key      user specified link_key\n"
		"\t-l          list file keys\n"
		"\t-L          list device keys\n"
		"\t-r          read from file\n"
		"\t-R          read from device\n"
		"\t-w          write to file\n"
		"\t-W          write to device\n"
		"\n");

	exit(EXIT_FAILURE);
}

static bool
scan_key(const char *arg)
{
	static const char digits[] = "0123456789abcdef";
	const char *p;
	int i, j;

	memset(key, 0, sizeof(key));

	for (i = 0 ; i < HCI_KEY_SIZE ; i++) {
		for (j = 0 ; j < 2 ; j++) {
			if (*arg == '\0')
				return true;

			for (p = digits ; *p != tolower((int)*arg) ; p++)
				if (*p == '\0')
					return false;

			arg++;
			key[i] = (key[i] << 4) + (p - digits);
		}
	}

	if (*arg != '\0')
		return false;

	return true;
}

void
print_key(const char *type, const uint8_t *src)
{
	int i;

	printf("%10s: ", type);
	for (i = 0 ; i < HCI_KEY_SIZE ; i++)
		printf("%2.2x", src[i]);

	printf("\n");
}


void
print_addr(const char *type, const bdaddr_t *addr)
{
	char name[HCI_DEVNAME_SIZE];
	struct hostent *he;

	printf("%10s: %s", type, bt_ntoa(addr, NULL));

	if (bt_devname(name, addr))
		printf(" (%s)", name);
	else if ((he = bt_gethostbyaddr((const char *)addr,
	    sizeof(bdaddr_t), AF_BLUETOOTH)) != NULL)
		printf(" (%s)", he->h_name);

	printf("\n");
}
