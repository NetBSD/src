/*	$NetBSD: devaddr.c,v 1.1 2006/06/19 15:44:36 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: devaddr.c,v 1.1 2006/06/19 15:44:36 gdamore Exp $");

#include <sys/ioctl.h>
#include <bluetooth.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
bt_devaddr(const char *name, bdaddr_t *addr)
{
	struct btreq btr;
	bdaddr_t bdaddr;
	int s;

	if (name == NULL) {
		errno = EINVAL;
		return 0;
	}

	if (addr == NULL)
		addr = &bdaddr;

	if (bt_aton(name, addr))
		return bt_devname(NULL, addr);

	memset(&btr, 0, sizeof(btr));
	strlcpy(btr.btr_name, name, HCI_DEVNAME_SIZE);

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return 0;

	if (ioctl(s, SIOCGBTINFO, &btr) < 0)
		return 0;

	close(s);

	bdaddr_copy(addr, &btr.btr_bdaddr);
	return 1;
}

int
bt_devname(char *name, const bdaddr_t *addr)
{
	struct btreq btr;
	int s;

	if (addr == NULL) {
		errno = EINVAL;
		return 0;
	}

	memset(&btr, 0, sizeof(btr));
	bdaddr_copy(&btr.btr_bdaddr, addr);

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return 0;

	if (ioctl(s, SIOCGBTINFOA, &btr) < 0)
		return 0;

	close(s);

	if (name != NULL)
		strlcpy(name, btr.btr_name, HCI_DEVNAME_SIZE);

	return 1;
}
