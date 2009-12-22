//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

extern "C" {
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>

#include "atf-c++/io.hpp"
#include "atf-c++/macros.hpp"

#include "h_lib.hpp"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
void
systembuf_check_data(std::istream& is, std::size_t length)
{
    char ch = 'A', chr;
    std::size_t cnt = 0;
    while (is >> chr) {
        ATF_CHECK_EQUAL(ch, chr);
        if (ch == 'Z')
            ch = 'A';
        else
            ch++;
        cnt++;
    }
    ATF_CHECK_EQUAL(cnt, length);
}

static
void
systembuf_write_data(std::ostream& os, std::size_t length)
{
    char ch = 'A';
    for (std::size_t i = 0; i < length; i++) {
        os << ch;
        if (ch == 'Z')
            ch = 'A';
        else
            ch++;
    }
    os.flush();
}

static
void
systembuf_test_read(std::size_t length, std::size_t bufsize)
{
    using atf::io::systembuf;

    std::ofstream f("test_read.txt");
    systembuf_write_data(f, length);
    f.close();

    int fd = ::open("test_read.txt", O_RDONLY);
    ATF_CHECK(fd != -1);
    systembuf sb(fd, bufsize);
    std::istream is(&sb);
    systembuf_check_data(is, length);
    ::close(fd);
    ::unlink("test_read.txt");
}

static
void
systembuf_test_write(std::size_t length, std::size_t bufsize)
{
    using atf::io::systembuf;

    int fd = ::open("test_write.txt", O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    ATF_CHECK(fd != -1);
    systembuf sb(fd, bufsize);
    std::ostream os(&sb);
    systembuf_write_data(os, length);
    ::close(fd);

    std::ifstream is("test_write.txt");
    systembuf_check_data(is, length);
    is.close();
    ::unlink("test_write.txt");
}

// ------------------------------------------------------------------------
// Test cases for the "file_handle" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(file_handle_ctor);
ATF_TEST_CASE_HEAD(file_handle_ctor)
{
    set_md_var("descr", "Tests file_handle's constructors");
}
ATF_TEST_CASE_BODY(file_handle_ctor)
{
    using atf::io::file_handle;

    file_handle fh1;
    ATF_CHECK(!fh1.is_valid());

    file_handle fh2(STDOUT_FILENO);
    ATF_CHECK(fh2.is_valid());
    fh2.disown();
}

ATF_TEST_CASE(file_handle_copy);
ATF_TEST_CASE_HEAD(file_handle_copy)
{
    set_md_var("descr", "Tests file_handle's copy constructor");
}
ATF_TEST_CASE_BODY(file_handle_copy)
{
    using atf::io::file_handle;

    file_handle fh1;
    file_handle fh2(STDOUT_FILENO);

    file_handle fh3(fh2);
    ATF_CHECK(!fh2.is_valid());
    ATF_CHECK(fh3.is_valid());

    fh1 = fh3;
    ATF_CHECK(!fh3.is_valid());
    ATF_CHECK(fh1.is_valid());

    fh1.disown();
}

ATF_TEST_CASE(file_handle_get);
ATF_TEST_CASE_HEAD(file_handle_get)
{
    set_md_var("descr", "Tests the file_handle::get method");
}
ATF_TEST_CASE_BODY(file_handle_get)
{
    using atf::io::file_handle;

    file_handle fh1(STDOUT_FILENO);
    ATF_CHECK_EQUAL(fh1.get(), STDOUT_FILENO);
}

ATF_TEST_CASE(file_handle_posix_remap);
ATF_TEST_CASE_HEAD(file_handle_posix_remap)
{
    set_md_var("descr", "Tests the file_handle::posix_remap method");
}
ATF_TEST_CASE_BODY(file_handle_posix_remap)
{
    using atf::io::file_handle;

    int pfd[2];

    ATF_CHECK(::pipe(pfd) != -1);
    file_handle rend(pfd[0]);
    file_handle wend(pfd[1]);

    ATF_CHECK(rend.get() != 10);
    ATF_CHECK(wend.get() != 10);
    wend.posix_remap(10);
    ATF_CHECK_EQUAL(wend.get(), 10);
    ATF_CHECK(::write(wend.get(), "test-posix-remap", 16) != -1);
    {
        char buf[17];
        ATF_CHECK_EQUAL(::read(rend.get(), buf, sizeof(buf)), 16);
        buf[16] = '\0';
        ATF_CHECK(std::strcmp(buf, "test-posix-remap") == 0);
    }

    // Redo previous to ensure that remapping over the same descriptor
    // has no side-effects.
    ATF_CHECK_EQUAL(wend.get(), 10);
    wend.posix_remap(10);
    ATF_CHECK_EQUAL(wend.get(), 10);
    ATF_CHECK(::write(wend.get(), "test-posix-remap", 16) != -1);
    {
        char buf[17];
        ATF_CHECK_EQUAL(::read(rend.get(), buf, sizeof(buf)), 16);
        buf[16] = '\0';
        ATF_CHECK(std::strcmp(buf, "test-posix-remap") == 0);
    }
}

// ------------------------------------------------------------------------
// Test cases for the "systembuf" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(systembuf_short_read);
ATF_TEST_CASE_HEAD(systembuf_short_read)
{
    set_md_var("descr", "Tests that a short read (one that fits in the "
               "internal buffer) works when using systembuf");
}
ATF_TEST_CASE_BODY(systembuf_short_read)
{
    systembuf_test_read(64, 1024);
}

ATF_TEST_CASE(systembuf_long_read);
ATF_TEST_CASE_HEAD(systembuf_long_read)
{
    set_md_var("descr", "Tests that a long read (one that does not fit in "
               "the internal buffer) works when using systembuf");
}
ATF_TEST_CASE_BODY(systembuf_long_read)
{
    systembuf_test_read(64 * 1024, 1024);
}

ATF_TEST_CASE(systembuf_short_write);
ATF_TEST_CASE_HEAD(systembuf_short_write)
{
    set_md_var("descr", "Tests that a short write (one that fits in the "
               "internal buffer) works when using systembuf");
}
ATF_TEST_CASE_BODY(systembuf_short_write)
{
    systembuf_test_write(64, 1024);
}

ATF_TEST_CASE(systembuf_long_write);
ATF_TEST_CASE_HEAD(systembuf_long_write)
{
    set_md_var("descr", "Tests that a long write (one that does not fit "
               "in the internal buffer) works when using systembuf");
}
ATF_TEST_CASE_BODY(systembuf_long_write)
{
    systembuf_test_write(64 * 1024, 1024);
}

// ------------------------------------------------------------------------
// Test cases for the "pipe" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(pipe_read_and_write);
ATF_TEST_CASE_HEAD(pipe_read_and_write)
{
    set_md_var("descr", "Tests reading from and writing to a pipe");
}
ATF_TEST_CASE_BODY(pipe_read_and_write)
{
    using atf::io::pipe;
    using atf::io::systembuf;

    pipe p;
    systembuf rbuf(p.rend().get());
    systembuf wbuf(p.wend().get());
    std::istream rend(&rbuf);
    std::ostream wend(&wbuf);

    // XXX This assumes that the pipe's buffer is big enough to accept
    // the data written without blocking!
    wend << "1Test 1message" << std::endl;
    std::string tmp;
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "1Test");
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "1message");
}

ATF_TEST_CASE(pipe_remap_read);
ATF_TEST_CASE_HEAD(pipe_remap_read)
{
    set_md_var("descr", "Tests the pipe::remap_read method");
}
ATF_TEST_CASE_BODY(pipe_remap_read)
{
    using atf::io::pipe;
    using atf::io::systembuf;

    pipe p;
    systembuf wbuf(p.wend().get());
    std::ostream wend(&wbuf);
    p.rend().posix_remap(STDIN_FILENO);

    // XXX This assumes that the pipe's buffer is big enough to accept
    // the data written without blocking!
    wend << "2Test 2message" << std::endl;
    std::string tmp;
    std::cin >> tmp;
    ATF_CHECK_EQUAL(tmp, "2Test");
    std::cin >> tmp;
    ATF_CHECK_EQUAL(tmp, "2message");
}

ATF_TEST_CASE(pipe_remap_write);
ATF_TEST_CASE_HEAD(pipe_remap_write)
{
    set_md_var("descr", "Tests the pipe::remap_write method");
}
ATF_TEST_CASE_BODY(pipe_remap_write)
{
    using atf::io::pipe;
    using atf::io::systembuf;

    pipe p;
    if (p.rend().get() == STDOUT_FILENO) {
        if (p.rend().get() > p.wend().get())
            p.rend().posix_remap(p.rend().get() + 1);
        else
            p.rend().posix_remap(p.wend().get() + 1);
    }
    systembuf rbuf(p.rend().get());
    std::istream rend(&rbuf);
    p.wend().posix_remap(STDOUT_FILENO);

    // XXX This assumes that the pipe's buffer is big enough to accept
    // the data written without blocking!
    std::cout << "3Test 3message" << std::endl;
    std::string tmp;
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "3Test");
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "3message");
}

// ------------------------------------------------------------------------
// Test cases for the "pistream" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(pistream);
ATF_TEST_CASE_HEAD(pistream)
{
    set_md_var("descr", "Tests the pistream class");
}
ATF_TEST_CASE_BODY(pistream)
{
    using atf::io::pipe;
    using atf::io::pistream;
    using atf::io::systembuf;

    pipe p;
    int fh = p.rend().get();
    pistream rend(p.rend());
    ATF_CHECK_EQUAL(fh, rend.handle().get());
    systembuf wbuf(p.wend().get());
    std::ostream wend(&wbuf);

    // XXX This assumes that the pipe's buffer is big enough to accept
    // the data written without blocking!
    wend << "1Test 1message" << std::endl;
    std::string tmp;
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "1Test");
    rend >> tmp;
    ATF_CHECK_EQUAL(tmp, "1message");
}

// ------------------------------------------------------------------------
// Test cases for the "postream" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(postream);
ATF_TEST_CASE_HEAD(postream)
{
    set_md_var("descr", "Tests the postream class");
}
ATF_TEST_CASE_BODY(postream)
{
    using atf::io::pipe;
    using atf::io::postream;
    using atf::io::systembuf;

    pipe p;
    int fh = p.wend().get();
    postream wend(p.wend());
    ATF_CHECK_EQUAL(fh, wend.handle().get());

    // The following block is to ensure that the read end is closed
    // before the write one.  Otherwise we get a SIGPIPE, at least
    // under FreeBSD 6.2.
    {
        systembuf rbuf(p.rend().get());
        std::istream rend(&rbuf);

        // XXX This assumes that the pipe's buffer is big enough to accept
        // the data written without blocking!
        wend << "1Test 1message" << std::endl;
        std::string tmp;
        rend >> tmp;
        ATF_CHECK_EQUAL(tmp, "1Test");
        rend >> tmp;
        ATF_CHECK_EQUAL(tmp, "1message");
    }
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atf-c++/io.hpp", "d_include_io_hpp.cpp");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add the tests for the "file_handle" class.
    ATF_ADD_TEST_CASE(tcs, file_handle_ctor);
    ATF_ADD_TEST_CASE(tcs, file_handle_copy);
    ATF_ADD_TEST_CASE(tcs, file_handle_get);
    ATF_ADD_TEST_CASE(tcs, file_handle_posix_remap);

    // Add the tests for the "systembuf" class.
    ATF_ADD_TEST_CASE(tcs, systembuf_short_read);
    ATF_ADD_TEST_CASE(tcs, systembuf_long_read);
    ATF_ADD_TEST_CASE(tcs, systembuf_short_write);
    ATF_ADD_TEST_CASE(tcs, systembuf_long_write);

    // Add the tests for the "pipe" class.
    ATF_ADD_TEST_CASE(tcs, pipe_read_and_write);
    ATF_ADD_TEST_CASE(tcs, pipe_remap_read);
    ATF_ADD_TEST_CASE(tcs, pipe_remap_write);

    // Add the tests for the "pistream" class.
    ATF_ADD_TEST_CASE(tcs, pistream);

    // Add the tests for the "postream" class.
    ATF_ADD_TEST_CASE(tcs, postream);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
