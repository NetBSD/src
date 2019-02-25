/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018, 2019 Andrew Turner
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/kcov.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <atf-c.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static int
open_kcov(void)
{
	int fd;

	fd = open("/dev/kcov", O_RDWR);
	if (fd == -1)
		atf_tc_skip("Failed to open /dev/kcov");

	return fd;
}

ATF_TC_WITHOUT_HEAD(kcov_bufsize);
ATF_TC_BODY(kcov_bufsize, tc)
{
	int fd;
	kcov_int_t size;
	fd = open_kcov();

	size = 0;
	ATF_CHECK(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == -1);
	size = 2;
	ATF_CHECK(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_mmap);
ATF_TC_BODY(kcov_mmap, tc)
{
	void *data;
	int fd;
	kcov_int_t size = 2 * PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) == MAP_FAILED);

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0);

	ATF_REQUIRE((data = mmap(NULL, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0)) != MAP_FAILED);

	munmap(data, 2 * PAGE_SIZE);

	close(fd);
}

/* This shouldn't panic */
ATF_TC_WITHOUT_HEAD(kcov_mmap_no_munmap);
ATF_TC_BODY(kcov_mmap_no_munmap, tc)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_mmap_no_munmap_no_close);
ATF_TC_BODY(kcov_mmap_no_munmap_no_close, tc)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);
}

static sem_t sem1, sem2;

static void *
kcov_mmap_enable_thread(void *data)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();
	*(int *)data = fd;

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	ATF_CHECK(mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
	    fd, 0) != MAP_FAILED);
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == 0);

	sem_post(&sem1);
	sem_wait(&sem2);

	return NULL;
}

ATF_TC_WITHOUT_HEAD(kcov_mmap_enable_thread_close);
ATF_TC_BODY(kcov_mmap_enable_thread_close, tc)
{
	pthread_t thread;
	int fd;

	sem_init(&sem1, 0, 0);
	sem_init(&sem2, 0, 0);
	pthread_create(&thread, NULL,
	    kcov_mmap_enable_thread, &fd);
	sem_wait(&sem1);
	close(fd);
	sem_post(&sem2);
	pthread_join(thread, NULL);
}

ATF_TC_WITHOUT_HEAD(kcov_enable);
ATF_TC_BODY(kcov_enable, tc)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == -1);

	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);

	/* We need to enable before disable */
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == -1);

	/* Check enabling works only with a valid trace method */
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == -1);

	/* Disable should only be called once */
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == -1);

	/* Re-enabling should also work */
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == 0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_DISABLE) == 0);

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_enable_no_disable);
ATF_TC_BODY(kcov_enable_no_disable, tc)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();
	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == 0);
	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_enable_no_disable_no_close);
ATF_TC_BODY(kcov_enable_no_disable_no_close, tc)
{
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();
	ATF_REQUIRE(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) ==0);
	ATF_CHECK(ioctl(fd, KCOV_IOC_ENABLE) == 0);
}

static void *
common_head(int *fdp)
{
	void *data;
	int fd;
	kcov_int_t size = PAGE_SIZE / KCOV_ENTRY_SIZE;

	fd = open_kcov();

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_SETBUFSIZE, &size) == 0,
	    "Unable to set the kcov buffer size");

	data = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	ATF_REQUIRE_MSG(data != MAP_FAILED, "Unable to mmap the kcov buffer");

	*fdp = fd;
	return data;
}

static void
common_tail(int fd, kcov_int_t *data)
{

	ATF_REQUIRE_MSG(munmap(__UNVOLATILE(data), PAGE_SIZE) == 0,
	    "Unable to unmap the kcov buffer");

	close(fd);
}

ATF_TC_WITHOUT_HEAD(kcov_basic);
ATF_TC_BODY(kcov_basic, tc)
{
	kcov_int_t *buf;
	int fd;

	buf = common_head(&fd);
	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE) == 0,
	    "Unable to enable kcov ");

	KCOV_STORE(buf[0], 0);

	sleep(0);
	ATF_REQUIRE_MSG(KCOV_LOAD(buf[0]) != 0, "No records found");

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);
}

static void *
thread_test_helper(void *ptr)
{
	kcov_int_t *buf = ptr;

	KCOV_STORE(buf[0], 0);
	sleep(0);
	ATF_REQUIRE_MSG(KCOV_LOAD(buf[0]) == 0,
	    "Records changed in blocked thread");

	return NULL;
}

ATF_TC_WITHOUT_HEAD(kcov_thread);
ATF_TC_BODY(kcov_thread, tc)
{
	pthread_t thread;
	kcov_int_t *buf;
	int fd;

	buf = common_head(&fd);

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_ENABLE) == 0,
	    "Unable to enable kcov ");

	pthread_create(&thread, NULL, thread_test_helper, __UNVOLATILE(buf));
	pthread_join(thread, NULL);

	ATF_REQUIRE_MSG(ioctl(fd, KCOV_IOC_DISABLE) == 0,
	    "Unable to disable kcov");

	common_tail(fd, buf);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, kcov_bufsize);
	ATF_TP_ADD_TC(tp, kcov_mmap);
	ATF_TP_ADD_TC(tp, kcov_mmap_no_munmap);
	ATF_TP_ADD_TC(tp, kcov_mmap_no_munmap_no_close);
	ATF_TP_ADD_TC(tp, kcov_enable);
	ATF_TP_ADD_TC(tp, kcov_enable_no_disable);
	ATF_TP_ADD_TC(tp, kcov_enable_no_disable_no_close);
	ATF_TP_ADD_TC(tp, kcov_mmap_enable_thread_close);
	ATF_TP_ADD_TC(tp, kcov_basic);
	ATF_TP_ADD_TC(tp, kcov_thread);
	return atf_no_error();
}
