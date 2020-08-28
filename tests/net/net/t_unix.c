/*	$NetBSD: t_unix.c,v 1.24 2020/08/28 14:18:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#define _GNU_SOURCE
#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$Id: t_unix.c,v 1.24 2020/08/28 14:18:29 riastradh Exp $");
#else
#define getprogname() argv[0]
#endif

#ifdef __linux__
#define LX -1
#else
#define LX
#endif
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "test.h"

#define UID 666
#define GID 999

uid_t srvruid, clntuid;
gid_t srvrgid, clntgid;

#define OF offsetof(struct sockaddr_un, sun_path)

static void
print(const char *msg, struct sockaddr_un *addr, socklen_t len)
{
	size_t i;

	printf("%s: client socket length: %zu\n", msg, (size_t)len);
	printf("%s: client family %d\n", msg, addr->sun_family);
#ifdef BSD4_4
	printf("%s: client len %d\n", msg, addr->sun_len);
#endif
	printf("%s: socket name: ", msg);
	for (i = 0; i < len - OF; i++) {
		int ch = addr->sun_path[i];
		if (ch < ' ' || '~' < ch)
			printf("\\x%02x", ch);
		else
			printf("%c", ch);
	}
	printf("\n");
}

static int
acc(int s)
{
	char guard1;
	struct sockaddr_un sun;
	char guard2;
	socklen_t len;

	guard1 = guard2 = 's';

	memset(&sun, 0, sizeof(sun));
	len = sizeof(sun);
	if ((s = accept(s, (struct sockaddr *)&sun, &len)) == -1)
		FAIL("accept");
	if (guard1 != 's')
		FAIL("guard1 = '%c'", guard1);
	if (guard2 != 's')
		FAIL("guard2 = '%c'", guard2);
#ifdef DEBUG
	print("accept", &sun, len);
#endif
	if (len != 2)
		FAIL("len %d != 2", len);
	if (sun.sun_family != AF_UNIX)
		FAIL("sun->sun_family %d != AF_UNIX", sun.sun_family);
#ifdef BSD4_4
	if (sun.sun_len != 2)
		FAIL("sun->sun_len %d != 2", sun.sun_len);
#endif
	for (size_t i = 0; i < sizeof(sun.sun_path); i++)
		if (sun.sun_path[i])
			FAIL("sun.sun_path[%zu] %d != NULL", i,
			    sun.sun_path[i]);
	return s;
fail:
	if (s != -1)
		close(s);
	return -1;
}

static int
peercred(int s, uid_t *euid, gid_t *egid, pid_t *pid)
{
#ifdef SO_PEERCRED
	/* Linux */
# define unpcbid ucred
# define unp_euid uid
# define unp_egid gid
# define unp_pid pid
# define LOCAL_PEEREID SO_PEERCRED
# define LEVEL SOL_SOCKET
#else
# define LEVEL 0
#endif

#ifdef LOCAL_PEEREID
	/* NetBSD */
	struct unpcbid cred;
	socklen_t crl;
	crl = sizeof(cred);
	if (getsockopt(s, LEVEL, LOCAL_PEEREID, &cred, &crl) == -1)
		return -1;
	*euid = cred.unp_euid;
	*egid = cred.unp_egid;
	*pid = cred.unp_pid;
	return 0;
#else
	/* FreeBSD */
	*pid = -1;
	return getpeereid(s, euid, egid);
#endif
}

static int
check_cred(int fd, bool statit, pid_t checkpid, pid_t otherpid, const char *s)
{
	pid_t pid;
	uid_t euid, uid;
	gid_t egid, gid;

	if (statit) {
		struct stat st;
		if (fstat(fd, &st) == -1)
			FAIL("fstat (%s)", s);
		euid = st.st_uid;
		egid = st.st_gid;
		pid = checkpid;
	} else {
		if (peercred(fd, &euid, &egid, &pid) == -1)
			FAIL("peercred (%s)", s);
	}
	printf("%s(%s) euid=%jd egid=%jd pid=%jd\n",
	    statit ? "fstat" : "peercred", s,
	    (intmax_t)euid, (intmax_t)egid, (intmax_t)pid);

	if (statit) {
		if (strcmp(s, "server") == 0) {
			uid = srvruid;
			gid = srvrgid;
		} else {
			uid = clntuid;
			gid = clntgid;
		}
	} else {
		if (checkpid != otherpid && strcmp(s, "server") == 0) {
			uid = clntuid;
			gid = clntgid;
		} else {
			uid = srvruid;
			gid = srvrgid;
		}
	}
	fflush(stdout);

	CHECK_EQUAL(euid, uid, s);
	CHECK_EQUAL(egid, gid, s);
	CHECK_EQUAL(pid, checkpid, s);
	return 0;
fail:
	return -1;
}

static int
test(bool forkit, bool closeit, bool statit, size_t len)
{
	size_t slen;
	socklen_t sl;
	int srvr = -1, clnt = -1, acpt = -1;
	pid_t srvrpid, clntpid;
	struct sockaddr_un *sock_addr = NULL, *sun = NULL;
	socklen_t sock_addrlen;
	socklen_t peer_addrlen;
	struct sockaddr_un peer_addr;

	srvruid = geteuid();
	srvrgid = getegid();
	srvrpid = clntpid = getpid();
	srvr = socket(AF_UNIX, SOCK_STREAM, 0);
	if (srvr == -1)
		FAIL("socket(server)");

	slen = len + OF + 1;

	if ((sun = calloc(1, slen)) == NULL)
		FAIL("calloc");

	if (mkdir("sock", 0777) == -1)
		FAIL("mkdir");

	if (chdir("sock") == -1)
		FAIL("chdir");

	memset(sun->sun_path, 'a', len);
	sun->sun_path[len] = '\0';
	(void)unlink(sun->sun_path);

	sl = SUN_LEN(sun);
#ifdef BSD4_4
	sun->sun_len = sl;
#endif
	sun->sun_family = AF_UNIX;

	unlink(sun->sun_path);

	if (bind(srvr, (struct sockaddr *)sun, sl) == -1) {
		if (errno == EINVAL && sl >= 256) {
			close(srvr);
			return -1;
		}
		FAIL("bind");
	}
	if (chmod(sun->sun_path, 0777) == -1)
		FAIL("chmod `%s'", sun->sun_path);

	if (listen(srvr, SOMAXCONN) == -1)
		FAIL("listen");

	if (forkit) {
		switch (clntpid = fork()) {
		case 0:	/* child */
			srvrpid = getppid();
			clntpid = getpid();
			if (srvruid == 0) {
				setgid(clntgid = GID);
				setuid(clntuid = UID);
			} else {
				clntgid = srvrgid;
				clntuid = srvruid;
			}
			break;
		case -1:
			FAIL("fork");
		default:
			if (srvruid == 0) {
				clntgid = GID;
				clntuid = UID;
			}
			break;
		}
	}

	if (clntpid == getpid()) {
		clnt = socket(AF_UNIX, SOCK_STREAM, 0);
		if (clnt == -1)
			FAIL("socket(client)");

		if (connect(clnt, (const struct sockaddr *)sun, sl) == -1)
			FAIL("connect");
		check_cred(clnt, statit, srvrpid, clntpid, "client");

	}

	if (srvrpid == getpid()) {
		acpt = acc(srvr);
		peer_addrlen = sizeof(peer_addr);
		memset(&peer_addr, 0, sizeof(peer_addr));
		if (getpeername(acpt, (struct sockaddr *)&peer_addr,
		    &peer_addrlen) == -1)
			FAIL("getpeername");
		print("peer", &peer_addr, peer_addrlen);
	}

	if (clntpid == getpid()) {
		if (closeit) {
			if (close(clnt) == -1)
				FAIL("close");
			clnt = -1;
		}
	}

	if (srvrpid == getpid()) {
		check_cred(acpt, statit, clntpid, srvrpid, "server");
		if ((sock_addr = calloc(1, slen)) == NULL)
			FAIL("calloc");
		sock_addrlen = slen;
		if (getsockname(srvr, (struct sockaddr *)sock_addr,
		    &sock_addrlen) == -1)
			FAIL("getsockname");
		print("sock", sock_addr, sock_addrlen);

		if (sock_addr->sun_family != AF_UNIX)
			FAIL("sock_addr->sun_family %d != AF_UNIX",
			    sock_addr->sun_family);

		len += OF;
		if (sock_addrlen LX != len)
			FAIL("sock_addr_len %zu != %zu", (size_t)sock_addrlen,
			    len);
#ifdef BSD4_4
		if (sock_addr->sun_len != sl)
			FAIL("sock_addr.sun_len %d != %zu", sock_addr->sun_len,
			    (size_t)sl);
#endif
		for (size_t i = 0; i < slen - OF; i++)
			if (sock_addr->sun_path[i] != sun->sun_path[i])
				FAIL("sock_addr.sun_path[%zu] %d != "
				    "sun->sun_path[%zu] %d\n", i,
				    sock_addr->sun_path[i], i,
				    sun->sun_path[i]);

		if (acpt != -1)
			(void)close(acpt);
		if (srvr != -1)
			(void)close(srvr);
		free(sock_addr);
		sock_addr = NULL;
		if (forkit && waitpid(clntpid, NULL, 0) == -1)
			FAIL("waitpid");
	}
	if (clnt != -1 && !closeit)
		(void)close(clnt);

	free(sock_addr);
	free(sun);
	return 0;
fail:
	if (srvrpid == getpid()) {
		if (acpt != -1)
			(void)close(acpt);
		if (srvr != -1)
			(void)close(srvr);
	}
	if (clntpid == getpid()) {
		if (clnt != -1 && !closeit)
			(void)close(clnt);
	}
	free(sock_addr);
	free(sun);
	return -1;
}

#ifndef TEST

ATF_TC(sockaddr_un_len_exceed);
ATF_TC_HEAD(sockaddr_un_len_exceed, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that exceeding the size of "
	    "unix domain sockets does not trash memory or kernel when "
	    "exceeding the size of the fixed sun_path");
}

ATF_TC_BODY(sockaddr_un_len_exceed, tc)
{
	ATF_REQUIRE_MSG(test(false, false, false, 254) == -1,
	    "test(false, false, false, 254): %s", strerror(errno));
}

ATF_TC(sockaddr_un_len_max);
ATF_TC_HEAD(sockaddr_un_len_max, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that we can use the maximum "
	    "unix domain socket pathlen (253): 255 - sizeof(sun_len) - "
	    "sizeof(sun_family)");
}

ATF_TC_BODY(sockaddr_un_len_max, tc)
{
	ATF_REQUIRE_MSG(test(false, false, false, 253) == 0,
	    "test(false, false, false, 253): %s", strerror(errno));
}

ATF_TC(sockaddr_un_closed);
ATF_TC_HEAD(sockaddr_un_closed, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that we can use the accepted "
	    "address of unix domain socket when closed");
}

ATF_TC_BODY(sockaddr_un_closed, tc)
{
	ATF_REQUIRE_MSG(test(false, true, false, 100) == 0,
	    "test(false, true, false, 100): %s", strerror(errno));
}

ATF_TC(sockaddr_un_local_peereid);
ATF_TC_HEAD(sockaddr_un_local_peereid, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that we get the right information"
	    " from LOCAL_PEEREID");
}

ATF_TC_BODY(sockaddr_un_local_peereid, tc)
{
	ATF_REQUIRE_MSG(test(true, true, false, 100) == 0,
	    "test(true, true, false, 100): %s", strerror(errno));
}

ATF_TC(sockaddr_un_fstat);
ATF_TC_HEAD(sockaddr_un_fstat, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that we get the right information"
	    " from fstat");
}

ATF_TC_BODY(sockaddr_un_fstat, tc)
{
	ATF_REQUIRE_MSG(test(true, true, true, 100) == 0,
	    "test(true, true, true, 100): %s", strerror(errno));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, sockaddr_un_len_exceed);
	ATF_TP_ADD_TC(tp, sockaddr_un_len_max);
	ATF_TP_ADD_TC(tp, sockaddr_un_closed);
	ATF_TP_ADD_TC(tp, sockaddr_un_local_peereid);
	ATF_TP_ADD_TC(tp, sockaddr_un_fstat);
	return atf_no_error();
}
#else
int
main(int argc, char *argv[])
{
	size_t len;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s <len>\n", getprogname());
		return EXIT_FAILURE;
	}
	test(false, false, false, atoi(argv[1]));
	test(false, true, false, atoi(argv[1]));
	test(true, false, false, atoi(argv[1]));
	test(true, true, false, atoi(argv[1]));
	test(true, true, true, atoi(argv[1]));
}
#endif
