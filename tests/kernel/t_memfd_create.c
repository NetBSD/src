/*	$NetBSD: t_memfd_create.c,v 1.1 2023/07/29 12:16:34 christos Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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
__RCSID("$NetBSD: t_memfd_create.c,v 1.1 2023/07/29 12:16:34 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include <atf-c.h>

#include "h_macros.h"

char name_buf[NAME_MAX];
char write_buf[8192];
char read_buf[8192];

ATF_TC(create_null_name);
ATF_TC_HEAD(create_null_name, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks memfd_create fails with EFAULT when invalid memory"
	    " is provided");
}
ATF_TC_BODY(create_null_name, tc)
{
	int fd;

	ATF_REQUIRE_EQ_MSG(fd = memfd_create(NULL, 0), -1,
	    "Unexpected success");
	ATF_REQUIRE_ERRNO(EFAULT, true);
}

ATF_TC(create_long_name);
ATF_TC_HEAD(create_long_name, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks memfd_create fails for names longer than NAME_MAX-6");
}
ATF_TC_BODY(create_long_name, tc)
{
	int fd;

        memset(name_buf, 'A', sizeof(name_buf));
	name_buf[NAME_MAX-6] = '\0';

	ATF_REQUIRE_EQ_MSG(fd = memfd_create(name_buf, 0), -1,
	    "Unexpected success");
	ATF_REQUIRE_ERRNO(ENAMETOOLONG, true);

	name_buf[NAME_MAX-7] = '\0';

	RL(fd = memfd_create(name_buf, 0));
}

ATF_TC(read_write);
ATF_TC_HEAD(read_write, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that data can be written to/read from a memfd");
}
ATF_TC_BODY(read_write, tc)
{
	int fd;
	off_t offset;

	RL(fd = memfd_create("", 0));

	tests_makegarbage(write_buf, sizeof(write_buf));
	memset(read_buf, 0, sizeof(read_buf));

	RL(write(fd, write_buf, sizeof(write_buf)));
	offset = lseek(fd, 0, SEEK_CUR);
	ATF_REQUIRE_EQ_MSG(offset, sizeof(write_buf),
	    "File offset not set after write (%ld != %ld)", offset,
	    sizeof(write_buf));

	RZ(lseek(fd, 0, SEEK_SET));

	RL(read(fd, read_buf, sizeof(read_buf)));
	offset = lseek(fd, 0, SEEK_CUR);
	ATF_REQUIRE_EQ_MSG(offset, sizeof(read_buf),
	    "File offset not set after read (%ld != %ld)", offset,
	    sizeof(read_buf));

	for (size_t i = 0; i < sizeof(read_buf); i++)
		ATF_REQUIRE_EQ_MSG(read_buf[i], write_buf[i],
		    "Data read does not match data written");
}

ATF_TC(truncate);
ATF_TC_HEAD(truncate, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that truncation does result in data removal");
}
ATF_TC_BODY(truncate, tc)
{
	int fd;
	struct stat st;

	RL(fd = memfd_create("", 0));

	tests_makegarbage(write_buf, sizeof(write_buf));
	tests_makegarbage(read_buf, sizeof(read_buf));

	RL(write(fd, write_buf, sizeof(write_buf)));

	RL(fstat(fd, &st));
	ATF_REQUIRE_EQ_MSG(st.st_size, sizeof(write_buf),
	    "Write did not grow size to %ld (is %ld)", sizeof(write_buf),
	    st.st_size);

	RL(ftruncate(fd, sizeof(write_buf)/2));
	RL(fstat(fd, &st));
	ATF_REQUIRE_EQ_MSG(st.st_size, sizeof(write_buf)/2,
	    "Truncate did not shrink size to %ld (is %ld)",
	    sizeof(write_buf)/2, st.st_size);

	RL(ftruncate(fd, sizeof(read_buf)));
	RL(fstat(fd, &st));
	ATF_REQUIRE_EQ_MSG(st.st_size, sizeof(read_buf),
	    "Truncate did not grow size to %ld (is %ld)", sizeof(read_buf),
	    st.st_size);

	RZ(lseek(fd, 0, SEEK_SET));
	RL(read(fd, read_buf, sizeof(read_buf)));

	for (size_t i = 0; i < sizeof(read_buf)/2; i++)
		ATF_REQUIRE_EQ_MSG(read_buf[i], write_buf[i],
		    "Data read does not match data written");
	for (size_t i = sizeof(read_buf)/2; i < sizeof(read_buf); i++)
		ATF_REQUIRE_EQ_MSG(read_buf[i], 0,
		    "Data read on growed region is not zeroed");
}

ATF_TC(mmap);
ATF_TC_HEAD(mmap, tc)
{

	atf_tc_set_md_var(tc, "descr", "Check that mmap succeeds");
}
ATF_TC_BODY(mmap, tc)
{
	int fd;
	void *addr;

	RL(fd = memfd_create("", 0));
	RL(ftruncate(fd, sizeof(read_buf)));

	addr = mmap(NULL, sizeof(read_buf), PROT_READ|PROT_WRITE, MAP_SHARED,
	    fd, 0);
	ATF_REQUIRE_MSG(addr != MAP_FAILED, "Mmap failed unexpectedly (%s)",
	    strerror(errno));
}

ATF_TC(create_no_sealing);
ATF_TC_HEAD(create_no_sealing, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that seals cannot be added if MFD_ALLOW_SEALING is"
	    " not specified to memfd_create");
}
ATF_TC_BODY(create_no_sealing, tc)
{
	int fd;

	RL(fd = memfd_create("", 0));

	ATF_REQUIRE_EQ_MSG(fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE), -1,
	    "fcntl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);
}

ATF_TC(seal_seal);
ATF_TC_HEAD(seal_seal, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks adding F_SEAL_SEAL prevents adding other seals");
}
ATF_TC_BODY(seal_seal, tc)
{
	int fd;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL));

        ATF_REQUIRE_EQ_MSG(fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE), -1,
	    "fcntl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);
}

/*
 * Tests that the seals provided in except to not also prevent some
 * other operation.
 *
 * Note: fd must have a positive size.
 */
static void
test_all_seals_except(int fd, int except)
{
	int rv;
	struct stat st;
	void *addr;

	RL(fstat(fd, &st));
	ATF_REQUIRE(st.st_size > 0);

	if (except & ~F_SEAL_SEAL) {
		rv = fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL);
		if (rv == -1) {
			ATF_REQUIRE_MSG(errno != EPERM,
			    "Seal %x prevented F_ADD_SEALS", except);
			ATF_REQUIRE_MSG(errno == EPERM,
			    "F_ADD_SEALS failed unexpectedly (%s)",
			    strerror(errno));
		}
	}

	if (except & ~(F_SEAL_WRITE|F_SEAL_FUTURE_WRITE)) {
		RZ(lseek(fd, 0, SEEK_SET));
		rv = write(fd, write_buf, sizeof(write_buf));
		if (rv == -1) {
			ATF_REQUIRE_MSG(errno != EPERM,
			    "Seal %x prevented write", except);
		        ATF_REQUIRE_MSG(errno == EPERM,
			    "Write failed unexpectedly (%s)",
			    strerror(errno));
		}

	        addr = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE,
		    MAP_SHARED, fd, 0);
		ATF_REQUIRE_MSG(addr != MAP_FAILED,
		    "Mmap failed unexpectedly (%s)", strerror(errno));
	}

	if (except & ~F_SEAL_SHRINK) {
		rv = ftruncate(fd, st.st_size - 1);
		if (rv == -1) {
		        ATF_REQUIRE_MSG(errno != EPERM,
			    "Seal %x prevented truncate to shrink", except);
			ATF_REQUIRE_MSG(errno == EPERM,
			    "Truncate failed unexpectedly (%s)",
			    strerror(errno));
		}
	}

	if (except & ~F_SEAL_GROW) {
		rv = ftruncate(fd, st.st_size + 1);
		if (rv == -1) {
		        ATF_REQUIRE_MSG(errno != EPERM,
			    "Seal %x prevented truncate to shrink", except);
		        ATF_REQUIRE_MSG(errno == EPERM,
			    "Truncate failed unexpectedly (%s)",
			    strerror(errno));
		}
	}
}

ATF_TC(seal_shrink);
ATF_TC_HEAD(seal_shrink, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks F_SEAL_SHRINK prevents shrinking the file");
}
ATF_TC_BODY(seal_shrink, tc)
{
	int fd;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(write_buf)));
	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK));

	ATF_REQUIRE_EQ_MSG(ftruncate(fd, sizeof(write_buf)/2), -1,
	    "Truncate succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);

	test_all_seals_except(fd, F_SEAL_SHRINK);
}

ATF_TC(seal_grow);
ATF_TC_HEAD(seal_grow, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks F_SEAL_SHRINK prevents growing the file");
}
ATF_TC_BODY(seal_grow, tc)
{
	int fd;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(write_buf)/2));
	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_GROW));

	ATF_REQUIRE_EQ_MSG(ftruncate(fd, sizeof(write_buf)), -1,
	    "Truncate succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);

	test_all_seals_except(fd, F_SEAL_GROW);
}

ATF_TC(seal_write);
ATF_TC_HEAD(seal_write, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks F_SEAL_WRITE prevents writing");
}
ATF_TC_BODY(seal_write, tc)
{
	int fd;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(write_buf)/2));
	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE));

	ATF_REQUIRE_EQ_MSG(write(fd, write_buf, sizeof(write_buf)), -1,
	    "Write succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);

	test_all_seals_except(fd, F_SEAL_WRITE);
}

ATF_TC(seal_write_mmap);
ATF_TC_HEAD(seal_write_mmap, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that F_SEAL_WRITE cannot be added with open mmaps");
}
ATF_TC_BODY(seal_write_mmap, tc)
{
	int fd;
	void *addr;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(read_buf)));

	addr = mmap(NULL, sizeof(read_buf), PROT_READ|PROT_WRITE, MAP_SHARED,
	    fd, 0);
	ATF_REQUIRE_MSG(addr != MAP_FAILED, "Mmap failed unexpectedly (%s)",
	    strerror(errno));

	ATF_REQUIRE_EQ_MSG(fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE), -1,
	    "fcntl succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EBUSY, true);
}

ATF_TC(seal_future_write);
ATF_TC_HEAD(seal_future_write, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks F_SEAL_FUTURE_WRITE prevents writing");
}
ATF_TC_BODY(seal_future_write, tc)
{
	int fd;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(write_buf)/2));
	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_FUTURE_WRITE));

	ATF_REQUIRE_EQ_MSG(write(fd, write_buf, sizeof(write_buf)), -1,
	    "Write succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);

	test_all_seals_except(fd, F_SEAL_FUTURE_WRITE);
}

ATF_TC(seal_future_write_mmap);
ATF_TC_HEAD(seal_future_write_mmap, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that F_SEAL_WRITE can be added with open mmaps but"
	    " prevents creating new ones");
}
ATF_TC_BODY(seal_future_write_mmap, tc)
{
	int fd;
	void *addr;

	RL(fd = memfd_create("", MFD_ALLOW_SEALING));
	RL(ftruncate(fd, sizeof(read_buf)));
	addr = mmap(NULL, sizeof(read_buf), PROT_READ|PROT_WRITE, MAP_SHARED,
	    fd, 0);
	ATF_REQUIRE_MSG(addr != MAP_FAILED, "Mmap failed unexpectedly (%s)",
	    strerror(errno));

	RL(fcntl(fd, F_ADD_SEALS, F_SEAL_FUTURE_WRITE));

	ATF_REQUIRE_EQ_MSG(mmap(NULL, sizeof(read_buf), PROT_READ|PROT_WRITE,
	    MAP_SHARED, fd, 0), MAP_FAILED, "Mmap succeeded unexpectedly");
	ATF_REQUIRE_ERRNO(EPERM, true);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, create_null_name);
	ATF_TP_ADD_TC(tp, create_long_name);
	ATF_TP_ADD_TC(tp, read_write);
	ATF_TP_ADD_TC(tp, truncate);
	ATF_TP_ADD_TC(tp, mmap);
	ATF_TP_ADD_TC(tp, create_no_sealing);
	ATF_TP_ADD_TC(tp, seal_seal);
	ATF_TP_ADD_TC(tp, seal_shrink);
	ATF_TP_ADD_TC(tp, seal_grow);
	ATF_TP_ADD_TC(tp, seal_write);
	ATF_TP_ADD_TC(tp, seal_write_mmap);
	ATF_TP_ADD_TC(tp, seal_future_write);
	ATF_TP_ADD_TC(tp, seal_future_write_mmap);

	return atf_no_error();
}
