/*	$NetBSD: auth2-krb5.c,v 1.3 2014/10/19 16:30:58 christos Exp $	*/
/*
 * Copyright (c) 2003 Markus Friedl.  All rights reserved.
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

#include "includes.h"
__RCSID("$NetBSD: auth2-krb5.c,v 1.3 2014/10/19 16:30:58 christos Exp $");

#include <krb5.h>
#include <stdio.h>

#include "ssh2.h"
#include "xmalloc.h"
#include "packet.h"
#include "log.h"
#include "key.h"
#include "buffer.h"
#include "hostfile.h"
#include "auth.h"
#ifdef GSSAPI
#include "ssh-gss.h"
#endif
#include "monitor_wrap.h"
#include "misc.h"
#include "servconf.h"

/* import */
extern ServerOptions options;

static int
userauth_kerberos(Authctxt *authctxt)
{
	krb5_data tkt, reply;
	u_int dlen;
	char *client = NULL;
	int authenticated = 0;

	tkt.data = packet_get_string(&dlen);
	tkt.length = dlen;
	packet_check_eom();

	if (PRIVSEP(auth_krb5(authctxt, &tkt, &client, &reply))) {
		authenticated = 1;
		if (reply.length)
			free(reply.data);
	}
	if (client)
		free(client);
	free(tkt.data);
	return (authenticated);
}

Authmethod method_kerberos = {
	"kerberos-2@ssh.com",
	userauth_kerberos,
	&options.kerberos_authentication
};
