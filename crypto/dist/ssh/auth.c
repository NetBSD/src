/*	$NetBSD: auth.c,v 1.1.1.2 2001/01/14 04:50:00 itojun Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 *
 * Copyright (c) 2000 Markus Friedl. All rights reserved.
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

/* from OpenBSD: auth.c,v 1.11 2000/10/11 20:27:23 markus Exp */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: auth.c,v 1.1.1.2 2001/01/14 04:50:00 itojun Exp $");
#endif

#include "includes.h"

#include "xmalloc.h"
#include "rsa.h"
#include "ssh.h"
#include "pty.h"
#include "packet.h"
#include "buffer.h"
#include "mpaux.h"
#include "servconf.h"
#include "compat.h"
#include "channels.h"
#include "match.h"

#include "bufaux.h"
#include "ssh2.h"
#include "auth.h"
#include "session.h"

#include <login_cap.h>

/* import */
extern ServerOptions options;

/*
 * Check if the user is allowed to log in via ssh. If user is listed in
 * DenyUsers or user's primary group is listed in DenyGroups, false will
 * be returned. If AllowUsers isn't empty and user isn't listed there, or
 * if AllowGroups isn't empty and user isn't listed there, false will be
 * returned.
 * If the user's shell is not executable, false will be returned.
 * Otherwise true is returned.
 */
int
allowed_user(struct passwd *pw)
{
	struct stat st;
	struct group *grp;
	const char *shell;
	int i, match_name, match_ip;
	login_cap_t *lc;
	const char *hostname, *ipaddr;
	char *cap_hlist, *hp;

	hostname = get_canonical_hostname();
	ipaddr = get_remote_ipaddr();

	/* Shouldn't be called if pw is NULL, but better safe than sorry... */
	if (pw == NULL)
		goto denied;

	lc = login_getclass(pw->pw_class);

	/*
	 * Check the deny list.
	 */
	cap_hlist = login_getcapstr(lc, "host.deny", NULL, NULL);
	if (cap_hlist != NULL) {
		hp = strtok(cap_hlist, ",");
		while (hp != NULL) {
			match_name = match_hostname(hostname,
			    hp, strlen(hp));
			match_ip = match_hostname(ipaddr,
			    hp, strlen(hp));
			/*
			 * Only a positive match here causes a "deny".
			 */
			if (match_name > 0 || match_ip > 0) {
				free(cap_hlist);
				login_close(lc);
				goto denied;
			}
			hp = strtok(NULL, ",");
		}
		free(cap_hlist);
	}

	/*
	 * Check the allow list.  If the allow list exists, and the
	 * remote host is not in it, the user is implicitly denied.
	 */
	cap_hlist = login_getcapstr(lc, "host.allow", NULL, NULL);
	if (cap_hlist != NULL) {
		hp = strtok(cap_hlist, ",");
		if (hp == NULL) {
			/* Just in case there's an empty string... */
			free(cap_hlist);
			login_close(lc);
			goto denied;
		}
		while (hp != NULL) {
			match_name = match_hostname(hostname,
			    hp, strlen(hp));
			match_ip = match_hostname(ipaddr,
			    hp, strlen(hp));
			/*
			 * Negative match causes an immediate "deny".
			 * Positive match causes us to break out
			 * of the loop (allowing a fallthrough).
			 */
			if (match_name < 0 || match_ip < 0) {
				free(cap_hlist);
				login_close(lc);
				goto denied;
			}
			if (match_name > 0 || match_ip > 0)
				break;
		}
		free(cap_hlist);
		if (hp == NULL) {
			login_close(lc);
			goto denied;
		}
	}

	login_close(lc);

	/*
	 * Get the shell from the password data.  An empty shell field is
	 * legal, and means /bin/sh.
	 */
	shell = (pw->pw_shell[0] == '\0') ? _PATH_BSHELL : pw->pw_shell;

	/*
	 * Deny if shell does not exists or is not executable.
	 * XXX Should check to see if it is executable by the
	 * XXX requesting user.  --thorpej
	 */
	if (stat(shell, &st) != 0)
		goto denied;
	if (S_ISREG(st.st_mode) == 0 ||
	    (st.st_mode & (S_IXOTH|S_IXUSR|S_IXGRP)) == 0)
		goto denied;

	/*
	 * XXX Consider nuking {Allow,Deny}{Users,Groups}.  We have the
	 * XXX login_cap(3) mechanism which covers all other types of
	 * XXX logins, too.
	 */

	/* Return false if user is listed in DenyUsers */
	if (options.num_deny_users > 0) {
		if (pw->pw_name == NULL)
			goto denied;
		for (i = 0; i < options.num_deny_users; i++)
			if (match_pattern(pw->pw_name, options.deny_users[i]))
				goto denied;
	}
	/* Return false if AllowUsers isn't empty and user isn't listed there */
	if (options.num_allow_users > 0) {
		if (pw->pw_name == NULL)
			goto denied;
		for (i = 0; i < options.num_allow_users; i++)
			if (match_pattern(pw->pw_name, options.allow_users[i]))
				break;
		/* i < options.num_allow_users iff we break for loop */
		if (i >= options.num_allow_users)
			goto denied;
	}
	/* Get the primary group name if we need it. Return false if it fails */
	if (options.num_deny_groups > 0 || options.num_allow_groups > 0) {
		grp = getgrgid(pw->pw_gid);
		if (grp == NULL)
			goto denied;

		/* Return false if user's group is listed in DenyGroups */
		if (options.num_deny_groups > 0) {
			if (grp->gr_name == NULL)
				goto denied;
			for (i = 0; i < options.num_deny_groups; i++)
				if (match_pattern(grp->gr_name,
				    options.deny_groups[i]))
					goto denied;
		}
		/*
		 * Return false if AllowGroups isn't empty and user's group
		 * isn't listed there
		 */
		if (options.num_allow_groups > 0) {
			if (grp->gr_name == NULL)
				goto denied;
			for (i = 0; i < options.num_allow_groups; i++)
				if (match_pattern(grp->gr_name,
				    options.allow_groups[i]))
					break;
			/* i < options.num_allow_groups iff we break for
			   loop */
			if (i >= options.num_allow_groups)
				goto denied;
		}
	}

	/* We found no reason not to let this user try to log on... */
	return 1;

 denied:
	/* XXX Need to add notice()  --thorpej */
	log("Denied connection for %.200s from %.200s [%.200s] port %d.\n",
	    pw == NULL ? "<unknown>" : pw->pw_name, hostname, ipaddr,
	    get_remote_port());
	return (0);
}
