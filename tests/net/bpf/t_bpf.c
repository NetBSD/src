/*	$NetBSD: t_bpf.c,v 1.6.2.1 2017/04/21 16:54:12 bouyer Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_bpf.c,v 1.6.2.1 2017/04/21 16:54:12 bouyer Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <net/if.h>
#include <net/bpf.h>
#include <net/dlt.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

/* XXX: atf-c.h has collisions with mbuf */
#undef m_type
#undef m_data
#include <atf-c.h>

#include "h_macros.h"
#include "../config/netconfig.c"

ATF_TC(bpfwriteleak);
ATF_TC_HEAD(bpfwriteleak, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks that writing to /dev/bpf "
	    "does not leak mbufs");
}

static int
getmtdata(void)
{
	struct mbstat mbstat;
	size_t mbstatlen = sizeof(mbstat);
	const int mbstat_mib[] = { CTL_KERN, KERN_MBUF, MBUF_STATS };

	RL(rump_sys___sysctl(mbstat_mib, __arraycount(mbstat_mib),
	    &mbstat, &mbstatlen, NULL, 0));
	return mbstat.m_mtypes[MT_DATA];
}

ATF_TC_BODY(bpfwriteleak, tc)
{
	char buf[28]; /* sizeof(garbage) > etherhdrlen */
	struct ifreq ifr;
	int ifnum, bpfd;

	RZ(rump_init());
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));
	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));
	RL(rump_sys_ioctl(bpfd, BIOCSFEEDBACK, &ifr));

	if (getmtdata() != 0)
		atf_tc_fail("test precondition failed: MT_DATA mbufs != 0");

	ATF_REQUIRE_ERRNO(ENETDOWN, rump_sys_write(bpfd, buf, sizeof(buf))==-1);

	ATF_REQUIRE_EQ(getmtdata(), 0);
}

#if (SIZE_MAX > UINT_MAX)
ATF_TC(bpfwritetrunc);
ATF_TC_HEAD(bpfwritetrunc, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that write to /dev/bpf "
	    "does not truncate size_t to int");
}

ATF_TC_BODY(bpfwritetrunc, tc)
{
	int bpfd;
	struct ifreq ifr;
	struct iovec *iov;
	size_t iovlen, sz;
	const size_t extra_bytes = 28;
	const size_t total = extra_bytes + UINT_MAX + 1;
	long iov_max, vm_page_size; /* round_page wants vm_page_size variable */

	memset(&ifr, 0, sizeof(ifr));

	iov_max      = sysconf(_SC_IOV_MAX);
	vm_page_size = sysconf(_SC_PAGE_SIZE);
	ATF_REQUIRE(iov_max > 1 && vm_page_size > 1);

	/*
	 * Minimize memory consumption by using many iovecs
	 * all pointing to one memory region.
	 */
	iov = calloc(iov_max, sizeof(struct iovec));
	ATF_REQUIRE(iov != NULL);

	sz = round_page((total + (iov_max - 1)) / iov_max);

	iov[0].iov_len = sz;
	iov[0].iov_base = mmap(NULL, sz, PROT_READ, MAP_ANON, -1, 0);
	ATF_REQUIRE(iov[0].iov_base != MAP_FAILED);

	iovlen = 1;
	while (sz + iov[0].iov_len <= total)
	{
		iov[iovlen].iov_len  = iov[0].iov_len;
		iov[iovlen].iov_base = iov[0].iov_base;
		sz += iov[0].iov_len;
		iovlen++;
	}

	if (sz < total)
	{
		iov[iovlen].iov_len = total - sz;
		iov[iovlen].iov_base = iov[0].iov_base;
		iovlen++;
	}

	/* Sanity checks */
	ATF_REQUIRE(iovlen >= 1 && iovlen <= (size_t)iov_max);
	ATF_REQUIRE_EQ(iov[iovlen-1].iov_len, total % iov[0].iov_len);

	RZ(rump_init());
	netcfg_rump_makeshmif("bpfwritetrunc", ifr.ifr_name);
	netcfg_rump_if(ifr.ifr_name, "10.1.1.1", "255.0.0.0");

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));
	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	ATF_CHECK_ERRNO(EMSGSIZE, rump_sys_writev(bpfd, iov, iovlen) == -1);

	munmap(iov[0].iov_base, iov[0].iov_len);
	free(iov);
}
#endif /* #if (SIZE_MAX > UINT_MAX) */

ATF_TC(bpf_ioctl_BLEN);
ATF_TC_HEAD(bpf_ioctl_BLEN, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks behaviors of BIOCGBLEN and "
	    "BIOCSBLEN");
}

ATF_TC_BODY(bpf_ioctl_BLEN, tc)
{
	struct ifreq ifr;
	int ifnum, bpfd;
	u_int blen = 0;

	RZ(rump_init());
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));

	RL(rump_sys_ioctl(bpfd, BIOCGBLEN, &blen));
	ATF_REQUIRE(blen != 0);
	blen = 100;
	RL(rump_sys_ioctl(bpfd, BIOCSBLEN, &blen));
	RL(rump_sys_ioctl(bpfd, BIOCGBLEN, &blen));
	ATF_REQUIRE_EQ(blen, 100);

	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCSBLEN, &blen), -1,
	    "Don't allow to change buflen after binding bpf to an interface");
}

ATF_TC(bpf_ioctl_PROMISC);
ATF_TC_HEAD(bpf_ioctl_PROMISC, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks behaviors of BIOCPROMISC");
}

ATF_TC_BODY(bpf_ioctl_PROMISC, tc)
{
	struct ifreq ifr;
	int ifnum, bpfd;

	RZ(rump_init());
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));

	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCPROMISC, NULL), -1,
	    "Don't allow to call ioctl(BIOCPROMISC) without interface");

	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	RL(rump_sys_ioctl(bpfd, BIOCPROMISC, NULL));
	/* TODO check if_flags */
}

ATF_TC(bpf_ioctl_SETIF);
ATF_TC_HEAD(bpf_ioctl_SETIF, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks behaviors of BIOCSETIF");
}

ATF_TC_BODY(bpf_ioctl_SETIF, tc)
{
	struct ifreq ifr;
	int ifnum, bpfd;

	RZ(rump_init());

	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));

	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);
	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	/* Change the listening interface */
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);
	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));
}

ATF_TC(bpf_ioctl_DLT);
ATF_TC_HEAD(bpf_ioctl_DLT, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks behaviors of BIOCGDLT and "
	    "BIOCSDLT");
}

ATF_TC_BODY(bpf_ioctl_DLT, tc)
{
	struct ifreq ifr;
	int ifnum, bpfd;
	u_int dlt;

	RZ(rump_init());
	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCGDLT, &dlt), -1,
	    "Don't allow to get a DLT without interfaces");

	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	RL(rump_sys_ioctl(bpfd, BIOCGDLT, &dlt));
	ATF_REQUIRE(dlt == DLT_EN10MB);

	dlt = DLT_NULL;
	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCSDLT, &dlt), -1,
	    "Don't allow to set a DLT that doesn't match any listening "
	    "interfaces");
}

ATF_TC(bpf_ioctl_GDLTLIST);
ATF_TC_HEAD(bpf_ioctl_GDLTLIST, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks behaviors of BIOCGDLTLIST");
}

ATF_TC_BODY(bpf_ioctl_GDLTLIST, tc)
{
	struct ifreq ifr;
	int ifnum, bpfd;
	struct bpf_dltlist dltlist;

	RZ(rump_init());
	RL(bpfd = rump_sys_open("/dev/bpf", O_RDWR));
	RZ(rump_pub_shmif_create(NULL, &ifnum));
	sprintf(ifr.ifr_name, "shmif%d", ifnum);

	dltlist.bfl_len = 0;
	dltlist.bfl_list = NULL;
	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCGDLTLIST, &dltlist), -1,
	    "Don't allow to get a DLT list without interfaces");

	RL(rump_sys_ioctl(bpfd, BIOCSETIF, &ifr));

	/* Get the size of an avaiable DLT list */
	dltlist.bfl_len = 0;
	dltlist.bfl_list = NULL;
	RL(rump_sys_ioctl(bpfd, BIOCGDLTLIST, &dltlist));
	ATF_REQUIRE(dltlist.bfl_len == 1);

	/* Get an avaiable DLT list */
	dltlist.bfl_list = calloc(sizeof(u_int), 1);
	dltlist.bfl_len = 1;
	RL(rump_sys_ioctl(bpfd, BIOCGDLTLIST, &dltlist));
	ATF_REQUIRE(dltlist.bfl_len == 1);
	ATF_REQUIRE(dltlist.bfl_list[0] == DLT_EN10MB);

	/* Get an avaiable DLT list with a less buffer (fake with bfl_len) */
	dltlist.bfl_len = 0;
	ATF_REQUIRE_EQ_MSG(rump_sys_ioctl(bpfd, BIOCGDLTLIST, &dltlist), -1,
	    "This should fail with ENOMEM");

	free(dltlist.bfl_list);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, bpfwriteleak);
#if (SIZE_MAX > UINT_MAX)
	ATF_TP_ADD_TC(tp, bpfwritetrunc);
#endif
	ATF_TP_ADD_TC(tp, bpf_ioctl_BLEN);
	ATF_TP_ADD_TC(tp, bpf_ioctl_PROMISC);
	ATF_TP_ADD_TC(tp, bpf_ioctl_SETIF);
	ATF_TP_ADD_TC(tp, bpf_ioctl_DLT);
	ATF_TP_ADD_TC(tp, bpf_ioctl_GDLTLIST);
	return atf_no_error();
}
