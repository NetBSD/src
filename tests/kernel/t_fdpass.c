/*	$NetBSD: t_fdpass.c,v 1.3 2026/10/02 01:31:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_fdpass.c,v 1.3 2026/10/02 01:31:12 riastradh Exp $");

#include <sys/param.h>		/* needed by sys/mbuf.h */

#include <sys/socket.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

/*
 * XXX We include <sys/mbuf.h> last to avoid conflict with atf-c.h over
 * m_type.
 */
#include <sys/mbuf.h>		/* MHLEN */

static unsigned
countfds(void)
{
	int fd, nfds, maxfd;

	RL(maxfd = rump_sys_fcntl(-1, F_MAXFD));
	for (fd = nfds = 0; fd <= maxfd; fd++) {
		if (rump_sys_fcntl(fd, F_GETFL) != -1)
			nfds++;
	}

	return nfds;
}

enum {
	MAXNFDS = 32,
};

static void
test_pr60832(const int fds[static 0], unsigned nfds)
{
	static char buf[4096];
	int sock[2];
	socklen_t optlen;
	int sndbuf;
	unsigned resid, clen, nfds_before, nfds_after;
	ssize_t nsent, nrcvd;
	size_t total = 0;

	/*
	 * Create a socket pair, nonblocking so that we fail promptly
	 * when buffers are full instead of hanging until timeout.
	 */
	RL(rump_sys_socketpair(AF_LOCAL, SOCK_STREAM|SOCK_NONBLOCK, 0, sock));

	/*
	 * Count the number of file descriptors in this process so we
	 * make sure we receive the right total number of them at the
	 * end.
	 */
	nfds_before = countfds();

	/*
	 * Find how much space is required for the control message.
	 *
	 * Note: We use CMSG_SPACE for struct msghdr::msg_controllen;
	 * for struct cmsghdr::cmsg_len below, we will use CMSG_LEN.
	 * Confused?  Thank the cmsg(3) API designers...
	 */
	ATF_REQUIRE(nfds <= MAXNFDS);
	clen = CMSG_SPACE(nfds * sizeof(int));

	/*
	 * Find out how much we can write to the socket (SO_SNDBUF)
	 * without blocking.
	 *
	 * XXX Why can we still write after sndbuf - sndlowat bytes?
	 */
	optlen = sizeof(sndbuf);
	RL(rump_sys_getsockopt(sock[0], SOL_SOCKET, SO_SNDBUF, &sndbuf,
		&optlen));
	ATF_REQUIRE_MSG(optlen == sizeof(sndbuf),
	    "optlen=%zu sizeof(sndbuf)=%zu",
	    (size_t)optlen, sizeof(sndbuf));

	printf("sndbuf=%d\n", sndbuf);
	ATF_REQUIRE(sndbuf > 0);
	ATF_REQUIRE((unsigned)sndbuf <= __type_max(__typeof(resid)));

	/*
	 * Fill the buffer until we have only MHLEN bytes left.  Count
	 * how many bytes we have sent total as we go.
	 *
	 * XXX Seems like this should really go until we have sndlowat
	 * bytes left.
	 */
	total = 0;
	for (resid = sndbuf;
	     resid > (size_t)MHLEN + clen;
	     resid -= (size_t)nsent) {
		const ssize_t nsend = MIN(resid - ((size_t)MHLEN + clen),
		    sizeof(buf));
		struct iovec iov[] = {
			{ .iov_base = buf, .iov_len = nsend },
		};
		const struct msghdr msg = {
			.msg_name = NULL,
			.msg_namelen = 0,
			.msg_iov = iov,
			.msg_iovlen = __arraycount(iov),
			.msg_control = NULL,
			.msg_controllen = 0,
			.msg_flags = 0,
		};

		printf("sendmsg, resid=%u nsend=%zd\n",
		    resid, nsend);
		ATF_REQUIRE(nsend > 0);
		RL(nsent = rump_sys_sendmsg(sock[0], &msg, 0));
		printf("sent %zd of %zd\n", nsent, nsend);
		ATF_REQUIRE_MSG(nsent == nsend, "nsent=%zd nsend=%zd",
		    nsent, nsend);
		total += (size_t)nsent;
	}
	ATF_REQUIRE_MSG(resid == (size_t)MHLEN + clen, "resid=%u", resid);

	/*
	 * Try to fill the remainder of the buffer, but with file
	 * descriptors as well.  The control message length should
	 * count toward sndbuf as well as the data length.
	 */
    {
	union {
		struct cmsghdr	hdr;
		unsigned char	buf[CMSG_SPACE(MAXNFDS * sizeof(int))];
	} cmsgbuf;
	const ssize_t nsend = resid - clen;
	struct iovec iov[] = {
		{ .iov_base = buf, .iov_len = nsend }
	};
	const struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = iov,
		.msg_iovlen = __arraycount(iov),
		.msg_control = &cmsgbuf,
		.msg_controllen = clen,
		.msg_flags = 0,
	};
	struct cmsghdr *const cmsg = CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_len = CMSG_LEN(nfds * sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	memcpy(CMSG_DATA(cmsg), fds, nfds * sizeof(int));

	printf("send %zd data bytes with %u control bytes\n", nsend, clen);
	RL(nsent = rump_sys_sendmsg(sock[0], &msg, 0));
	total += (size_t)nsent;
    }

	/*
	 * Receive the data and file descriptors.  In some chunk
	 * (though it may not be the last chunk, because the last chunk
	 * may be split into smaller chunks), we should receive the
	 * fds.
	 */
	for (resid = total; resid > 0; resid -= (size_t)nrcvd) {
		union {
			struct cmsghdr	hdr;
			unsigned char	buf[CMSG_SPACE(MAXNFDS * sizeof(int))];
		} cmsgbuf;
		const ssize_t nrecv = sizeof(buf);
		struct iovec iov[] = {
			{ .iov_base = buf, .iov_len = nrecv }
		};
		struct msghdr msg = {
			.msg_name = NULL,
			.msg_namelen = 0,
			.msg_iov = iov,
			.msg_iovlen = __arraycount(iov),
			.msg_control = &cmsgbuf,
			.msg_controllen = sizeof(cmsgbuf),
			.msg_flags = 0,
		};
		struct cmsghdr *cmsg;

		printf("recvmsg, resid=%u nrecv=%zu\n", resid, nrecv);
		nrcvd = rump_sys_recvmsg(sock[1], &msg, 0);
		if (nrcvd == -1) {
			int error = errno;

			atf_tc_fail_nonfatal("recvmsg: %d (%s)", error,
			    strerror(error));
			break;
		}
		printf("nrcvd=%zu\n", nrcvd);
		ATF_REQUIRE_MSG((size_t)nrcvd <= resid, "nrcvd=%zd resid=%u",
		    nrcvd, resid);
		for (cmsg = CMSG_FIRSTHDR(&msg);
		     cmsg != NULL;
		     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			unsigned i, n;
			const int *fdptr;

			if (cmsg->cmsg_level != SOL_SOCKET) {
				atf_tc_fail_nonfatal("cmsg_level=%d,"
				    " expected %d\n",
				    cmsg->cmsg_level, SOL_SOCKET);
				continue;
			}
			if (cmsg->cmsg_type != SCM_RIGHTS) {
				atf_tc_fail_nonfatal("cmsg_type=%d,"
				    " expected %d\n",
				    cmsg->cmsg_type, SCM_RIGHTS);
				continue;
			}
			if (cmsg->cmsg_len < CMSG_LEN(sizeof(int))) {
				atf_tc_fail_nonfatal("cmsg_len=%zu,"
				    " expected >=%zu\n",
				    (size_t)cmsg->cmsg_len,
				    (size_t)CMSG_LEN(sizeof(int)));
				continue;
			}
			if ((cmsg->cmsg_len - CMSG_LEN(0)) % sizeof(int)) {
				atf_tc_fail_nonfatal("cmsg_len=%zu,"
				    " expected 0 mod %zu after %zu\n",
				    (size_t)cmsg->cmsg_len,
				    (size_t)sizeof(int),
				    (size_t)CMSG_LEN(0));
				continue;
			}
			n = (cmsg->cmsg_len - CMSG_LEN(0))/sizeof(int);
			ATF_CHECK(n > 0);
			fdptr = (const int *)CMSG_DATA(cmsg);
			for (i = 0; i < n; i++)
				RL(rump_sys_close(fdptr[i]));
		}
	}

	/*
	 * For one final recvmsg, we should block.
	 */
    {
	union {
		struct cmsghdr	hdr;
		unsigned char	buf[CMSG_SPACE(MAXNFDS * sizeof(int))];
	} cmsgbuf;
	struct iovec iov[] = {
		{ .iov_base = buf, .iov_len = sizeof(buf) }
	};
	struct msghdr msg = {
		.msg_name = NULL,
		.msg_namelen = 0,
		.msg_iov = iov,
		.msg_iovlen = __arraycount(iov),
		.msg_control = &cmsgbuf,
		.msg_controllen = sizeof(cmsgbuf),
		.msg_flags = 0,
	};

	ATF_CHECK_ERRNO(EAGAIN, rump_sys_recvmsg(sock[1], &msg, 0) == -1);
    }

	/*
	 * Verify the total number of file descriptors has not changed.
	 * (We closed all the copies we sent ourself in the recvmsg
	 * loop above.)
	 */
	nfds_after = countfds();
	ATF_CHECK_MSG(nfds_before == nfds_after,
	    "nfds_before=%u nfds_after=%u", nfds_before, nfds_after);

	RL(rump_sys_close(sock[0]));
	RL(rump_sys_close(sock[1]));
}

ATF_TC(pr60832);
ATF_TC_HEAD(pr60832, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test sendmsg fails on partial data");
}
ATF_TC_BODY(pr60832, tc)
{
	int fds[MAXNFDS];
	unsigned i;

	REQUIRE_LIBC(setlinebuf(stdout), EOF);
	printf("MHLEN=%d\n", MHLEN);

	/*
	 * Initialize rump before we try using it.
	 */
	RL(rump_init());

	/*
	 * Create some spare file descriptors.
	 */
	for (i = 0; i < __arraycount(fds); i++)
		RL(fds[i] = rump_sys_socket(AF_LOCAL, SOCK_STREAM, 0));

	/*
	 * Run the test with different numbers of file descriptors.
	 */
	for (i = 0; i < __arraycount(fds); i++) {
		const unsigned nfds = i + 1;

		printf("test %u fd%s\n", nfds, nfds == 1 ? "" : "s");

		if (CMSG_SPACE(nfds * sizeof(int)) <
		    CMSG_SPACE(nfds * sizeof(struct file *))) {
			atf_tc_expect_fail("PR kern/60832:"
			    " AF_LOCAL stream: sendmsg() with SCM_RIGHTS"
			    " silently drops data and descriptors"
			    " but reports success");
		}

		test_pr60832(fds, nfds);

		if (CMSG_SPACE(nfds * sizeof(int)) <
		    CMSG_SPACE(nfds * sizeof(struct file *)))
			atf_tc_expect_pass();
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, pr60832);

	return atf_no_error();
}
