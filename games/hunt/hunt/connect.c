/*	$NetBSD: connect.c,v 1.8.12.1 2014/08/20 00:00:23 tls Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor
 *   the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: connect.c,v 1.8.12.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include "hunt_common.h"
#include "hunt_private.h"

void
do_connect(const char *name, size_t namelen, char team, int enter_status)
{
	static int32_t uid;
	int32_t mode;
	int32_t wire_status = htonl(enter_status);

	if (uid == 0)
		uid = htonl(getuid());
	(void) write(huntsocket, &uid, sizeof(uid));
	assert(namelen == WIRE_NAMELEN);
	(void) write(huntsocket, name, namelen);
	(void) write(huntsocket, &team, 1);
	(void) write(huntsocket, &wire_status, sizeof(wire_status));
	(void) strcpy(Buf, ttyname(fileno(stderr)));
	(void) write(huntsocket, Buf, WIRE_NAMELEN);
#ifdef INTERNET
	if (Send_message != NULL)
		mode = C_MESSAGE;
	else
#endif
#ifdef MONITOR
	if (Am_monitor)
		mode = C_MONITOR;
	else
#endif
		mode = C_PLAYER;
	mode = htonl(mode);
	(void) write(huntsocket, &mode, sizeof mode);
}
