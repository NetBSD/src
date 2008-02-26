/*-
 * Copyright (c) 2003-2007 Tim Kientzle
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "test.h"
__FBSDID("$FreeBSD$");

/*
 * Create a file on disk and set the atime to a known value.
 */
static void
test_create(const char *filename)
{
	int fd;
	struct timeval times[2];

	fd = open(filename, O_CREAT | O_WRONLY, 0644);
	assert(fd >= 0);
	/*
	 * Note: Have to write at least one byte to the file.
	 * cpio doesn't bother reading the file if it's zero length,
	 * so the atime never gets changed in that case, which
	 * makes the tests below rather pointless.
	 */
	assertEqualInt(1, write(fd, "a", 1));
	memset(times, 0, sizeof(times));
	times[0].tv_sec = 1; /* atime = 1.000000002 */
	times[0].tv_usec = 2;
	times[1].tv_sec = 3; /* mtime = 3.000000004 */
	times[1].tv_usec = 4;
	assertEqualInt(0, futimes(fd, times));
	close(fd);
}

DEFINE_TEST(test_option_a)
{
	struct stat st;
	int r;

	/* Sanity check; verify that test_create really works. */
	test_create("f0");
	assertEqualInt(0, stat("f0", &st));
	failure("test_create function is supposed to create a file with atime == 1; if this doesn't work, test_option_a is entirely invalid.");
	assertEqualInt(st.st_atime, 1);

	/* Copy the file without -a; should change the atime. */
	test_create("f1");
	r = systemf("echo f1 | %s -pd --quiet copy-no-a > copy-no-a.out 2>copy-no-a.err", testprog);
	assertEqualInt(r, 0);
	assertEmptyFile("copy-no-a.err");
	assertEmptyFile("copy-no-a.out");
	assertEqualInt(0, stat("f1", &st));
	failure("Copying file without -a should have changed atime.  Ignore this if your system does not record atimes.");
	assert(st.st_atime != 1);

	/* Archive the file without -a; should change the atime. */
	test_create("f2");
	r = systemf("echo f2 | %s -o --quiet > archive-no-a.out 2>archive-no-a.err", testprog);
	assertEqualInt(r, 0);
	assertEmptyFile("copy-no-a.err");
	assertEqualInt(0, stat("f2", &st));
	failure("Archiving file without -a should have changed atime.  Ignore this if your system does not record atimes.");
	assert(st.st_atime != 1);

	/* Copy the file with -a; should not change the atime. */
	test_create("f3");
	r = systemf("echo f3 | %s -pad --quiet copy-a > copy-a.out 2>copy-a.err", testprog);
	assertEqualInt(r, 0);
	assertEmptyFile("copy-a.err");
	assertEmptyFile("copy-a.out");
	assertEqualInt(0, stat("f3", &st));
	failure("Copying file with -a should not have changed atime.");
	assertEqualInt(st.st_atime, 1);

	/* Archive the file without -a; should change the atime. */
	test_create("f4");
	r = systemf("echo f4 | %s -oa --quiet > archive-a.out 2>archive-a.err", testprog);
	assertEqualInt(r, 0);
	assertEmptyFile("copy-a.err");
	assertEqualInt(0, stat("f4", &st));
	failure("Archiving file with -a should not have changed atime.");
	assertEqualInt(st.st_atime, 1);

}
