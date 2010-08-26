//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
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
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstring>

extern "C" {
#include "atf-c/error.h"
}

#include "atf-c++/exceptions.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/sanity.hpp"

namespace impl = atf::io;
#define IMPL_NAME "atf::io"

// ------------------------------------------------------------------------
// The "file_handle" class.
// ------------------------------------------------------------------------

impl::file_handle::file_handle(void) :
    m_handle(invalid_value())
{
}

impl::file_handle::file_handle(handle_type h) :
    m_handle(h)
{
    PRE(m_handle != invalid_value());
}

impl::file_handle::file_handle(const file_handle& fh) :
    m_handle(fh.m_handle)
{
    fh.m_handle = invalid_value();
}

impl::file_handle::~file_handle(void)
{
    if (is_valid())
        close();
}

impl::file_handle&
impl::file_handle::operator=(const file_handle& fh)
{
    m_handle = fh.m_handle;
    fh.m_handle = invalid_value();

    return *this;
}

bool
impl::file_handle::is_valid(void)
    const
{
    return m_handle != invalid_value();
}

void
impl::file_handle::close(void)
{
    PRE(is_valid());

    ::close(m_handle);

    m_handle = invalid_value();
}

impl::file_handle::handle_type
impl::file_handle::disown(void)
{
    PRE(is_valid());

    handle_type h = m_handle;
    m_handle = invalid_value();
    return h;
}

impl::file_handle::handle_type
impl::file_handle::get(void)
    const
{
    PRE(is_valid());

    return m_handle;
}

void
impl::file_handle::posix_remap(handle_type h)
{
    PRE(is_valid());

    if (m_handle == h)
        return;

    if (::dup2(m_handle, h) == -1)
        throw system_error(IMPL_NAME "::file_handle::posix_remap",
                           "dup2(2) failed", errno);

    if (::close(m_handle) == -1) {
        ::close(h);
        throw system_error(IMPL_NAME "::file_handle::posix_remap",
                           "close(2) failed", errno);
    }

    m_handle = h;
}

impl::file_handle::handle_type
impl::file_handle::invalid_value(void)
{
    return -1;
}

// ------------------------------------------------------------------------
// The "systembuf" class.
// ------------------------------------------------------------------------

impl::systembuf::systembuf(handle_type h, std::size_t bufsize) :
    m_handle(h),
    m_bufsize(bufsize),
    m_read_buf(NULL),
    m_write_buf(NULL)
{
    PRE(m_handle >= 0);
    PRE(m_bufsize > 0);

    try {
        m_read_buf = new char[bufsize];
        m_write_buf = new char[bufsize];
    } catch (...) {
        if (m_read_buf != NULL)
            delete [] m_read_buf;
        if (m_write_buf != NULL)
            delete [] m_write_buf;
        throw;
    }

    setp(m_write_buf, m_write_buf + m_bufsize);
}

impl::systembuf::~systembuf(void)
{
    sync(); // XXX Unsure if this is correct.  But seems to be needed.
    delete [] m_read_buf;
    delete [] m_write_buf;
}

impl::systembuf::int_type
impl::systembuf::underflow(void)
{
    PRE(gptr() >= egptr());

    bool ok;
    ssize_t cnt = ::read(m_handle, m_read_buf, m_bufsize);
    ok = (cnt != -1 && cnt != 0);

    if (!ok)
        return traits_type::eof();
    else {
        setg(m_read_buf, m_read_buf, m_read_buf + cnt);
        return traits_type::to_int_type(*gptr());
    }
}

impl::systembuf::int_type
impl::systembuf::overflow(int c)
{
    PRE(pptr() >= epptr());
    if (sync() == -1)
        return traits_type::eof();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        traits_type::assign(*pptr(), c);
        pbump(1);
    }
    return traits_type::not_eof(c);
}

int
impl::systembuf::sync(void)
{
    ssize_t cnt = pptr() - pbase();

    bool ok;
    ok = ::write(m_handle, pbase(), cnt) == cnt;

    if (ok)
        pbump(-cnt);
    return ok ? 0 : -1;
}

// ------------------------------------------------------------------------
// The "pipe" class.
// ------------------------------------------------------------------------

impl::pipe::pipe(void)
{
    file_handle::handle_type hs[2];

    if (::pipe(hs) == -1)
        throw system_error(IMPL_NAME "::pipe::pipe",
                           "pipe(2) failed", errno);

    m_read_end = file_handle(hs[0]);
    m_write_end = file_handle(hs[1]);
}

impl::file_handle&
impl::pipe::rend(void)
{
    return m_read_end;
}

impl::file_handle&
impl::pipe::wend(void)
{
    return m_write_end;
}

// ------------------------------------------------------------------------
// The "pistream" class.
// ------------------------------------------------------------------------

impl::pistream::pistream(impl::file_handle& fh) :
    std::istream(NULL),
    m_handle(fh),
    m_systembuf(m_handle.get())
{
    rdbuf(&m_systembuf);
}

void
impl::pistream::close(void)
{
    m_handle.close();
}

impl::file_handle&
impl::pistream::handle(void)
{
    return m_handle;
}

// ------------------------------------------------------------------------
// The "postream" class.
// ------------------------------------------------------------------------

impl::postream::postream(impl::file_handle& fh) :
    std::ostream(NULL),
    m_handle(fh),
    m_systembuf(m_handle.get())
{
    rdbuf(&m_systembuf);
}

void
impl::postream::close(void)
{
    m_handle.close();
}

impl::file_handle&
impl::postream::handle(void)
{
    return m_handle;
}

// ------------------------------------------------------------------------
// The "pollable_istream" class.
// ------------------------------------------------------------------------

impl::unbuffered_istream::unbuffered_istream(impl::file_handle& fh) :
    m_fh(fh),
    m_is_good(true)
{
}

impl::file_handle&
impl::unbuffered_istream::get_fh(void)
{
    return m_fh;
}

bool
impl::unbuffered_istream::good(void)
    const
{
    return m_is_good;
}

size_t
impl::unbuffered_istream::read(void* buf, size_t buflen)
{
    if (!m_is_good)
        return 0;

    ssize_t res = ::read(m_fh.get(), buf, buflen);
    m_is_good = res > 0;
    return res > 0 ? res : 0;
}

void
impl::unbuffered_istream::close(void)
{
    m_is_good = false;
    m_fh.close();
}

// ------------------------------------------------------------------------
// The "std_muxer" class.
// ------------------------------------------------------------------------

impl::std_muxer::std_muxer(void)
{
}

impl::std_muxer::~std_muxer(void)
{
}

void
impl::std_muxer::got_stdout_line(const std::string& line)
{
}

void
impl::std_muxer::got_stderr_line(const std::string& line)
{
}

void
impl::std_muxer::got_eof(void)
{
}

void
impl::std_muxer::read(unbuffered_istream& out, unbuffered_istream& err,
                      const bool& terminate)
{
    struct pollfd fds[2];
    fds[0].fd = out.get_fh().get();
    fds[0].events = POLLIN;
    fds[1].fd = err.get_fh().get();
    fds[1].events = POLLIN;

    do {
        fds[0].revents = 0;
        fds[1].revents = 0;

        int ret;
        while ((ret = ::poll(fds, 2, 250)) <= 0) {
            if (terminate || ret == -1) {
                fds[0].events = 0;
                fds[1].events = 0;
                break;
            }
        }

        if (fds[0].revents & POLLIN) {
            std::string line;
            if (impl::getline(out, line).good())
                got_stdout_line(line);
            else
                fds[0].events &= ~POLLIN;
        } else if (fds[0].revents & POLLERR || fds[0].revents & POLLHUP)
            fds[0].events &= ~POLLIN;

        if (fds[1].revents & POLLIN) {
            std::string line;
            if (impl::getline(err, line).good())
                got_stderr_line(line);
            else
                fds[1].events &= ~POLLIN;
        } else if (fds[1].revents & POLLERR || fds[1].revents & POLLHUP)
            fds[1].events &= ~POLLIN;
    } while (fds[0].events & POLLIN || fds[1].events & POLLIN);

    got_eof();
}

// ------------------------------------------------------------------------
// Free functions.
// ------------------------------------------------------------------------

impl::unbuffered_istream&
impl::getline(unbuffered_istream& uis, std::string& str)
{
    str.clear();
    char ch;
    while (uis.read(&ch, sizeof(ch)) == sizeof(ch) && ch != '\n') {
        if (ch != '\r')
            str += ch;
    }
    return uis;
}

int
impl::cmp(const fs::path& p1, const fs::path& p2)
{
    bool equal = false;

    int fd1 = ::open(p1.c_str(), O_RDONLY);
    if (fd1 == -1)
        throw system_error(IMPL_NAME "::cmp", "open(2) failed", errno);
    file_handle f1(fd1);

    int fd2 = ::open(p2.c_str(), O_RDONLY);
    if (fd2 == -1)
        throw system_error(IMPL_NAME "::cmp", "open(2) failed", errno);
    file_handle f2(fd2);

    for (;;) {
        ssize_t r1, r2;
        char buf1[512], buf2[512];

        r1 = ::read(fd1, buf1, sizeof(buf1));
        if (r1 < 0)
            throw system_error(IMPL_NAME "::cmp", "read(2) failed for " +
                               p1.str(), errno);

        r2 = read(fd2, buf2, sizeof(buf2));
        if (r2 < 0)
            throw system_error(IMPL_NAME "::cmp", "read(2) failed for " +
                               p2.str(), errno);

        if ((r1 == 0) && (r2 == 0)) {
            equal = true;
            break;
        }

        if ((r1 != r2) || (std::memcmp(buf1, buf2, r1) != 0)) {
            break;
        }
    }

    return equal;
}
