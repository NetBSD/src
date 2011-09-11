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

#if defined(HAVE_CONFIG_H)
#include "bconfig.h"
#endif

extern "C" {
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <unistd.h>
}

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "atf-c++/detail/process.hpp"

#include "fs.hpp"
#include "user.hpp"

namespace impl = atf::atf_run;
#define IMPL_NAME "atf::atf_run"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static void cleanup_aux(const atf::fs::path&, dev_t, bool);
static void cleanup_aux_dir(const atf::fs::path&, const atf::fs::file_info&,
                            bool);
static void do_unmount(const atf::fs::path&);

// The erase parameter in this routine is to control nested mount points.
// We want to descend into a mount point to unmount anything that is
// mounted under it, but we do not want to delete any files while doing
// this traversal.  In other words, we erase files until we cross the
// first mount point, and after that point we only scan and unmount.
static
void
cleanup_aux(const atf::fs::path& p, dev_t parent_device, bool erase)
{
    atf::fs::file_info fi(p);

    if (fi.get_type() == atf::fs::file_info::dir_type)
        cleanup_aux_dir(p, fi, fi.get_device() == parent_device);

    if (fi.get_device() != parent_device)
        do_unmount(p);

    if (erase) {
        if (fi.get_type() == atf::fs::file_info::dir_type)
            atf::fs::rmdir(p);
        else
            atf::fs::remove(p);
    }
}

static
void
cleanup_aux_dir(const atf::fs::path& p, const atf::fs::file_info& fi,
                bool erase)
{
    if (erase && ((fi.get_mode() & S_IRWXU) != S_IRWXU)) {
        if (chmod(p.c_str(), fi.get_mode() | S_IRWXU) == -1)
            throw atf::system_error(IMPL_NAME "::cleanup(" +
                                    p.str() + ")", "chmod(2) failed", errno);
    }

    std::set< std::string > subdirs;
    {
        const atf::fs::directory d(p);
        subdirs = d.names();
    }

    for (std::set< std::string >::const_iterator iter = subdirs.begin();
         iter != subdirs.end(); iter++) {
        const std::string& name = *iter;
        if (name != "." && name != "..")
            cleanup_aux(p / name, fi.get_device(), erase);
    }
}

static
void
do_unmount(const atf::fs::path& in_path)
{
    // At least, FreeBSD's unmount(2) requires the path to be absolute.
    // Let's make it absolute in all cases just to be safe that this does
    // not affect other systems.
    const atf::fs::path& abs_path = in_path.is_absolute() ?
        in_path : in_path.to_absolute();

#if defined(HAVE_UNMOUNT)
    if (unmount(abs_path.c_str(), 0) == -1)
        throw atf::system_error(IMPL_NAME "::cleanup(" + in_path.str() + ")",
                                "unmount(2) failed", errno);
#else
    // We could use umount(2) instead if it was available... but
    // trying to do so under, e.g. Linux, is a nightmare because we
    // also have to update /etc/mtab to match what we did.  It is
    // satf::fser to just leave the system-specific umount(8) tool deal
    // with it, at least for now.

    const atf::fs::path prog("umount");
    atf::process::argv_array argv("umount", abs_path.c_str(), NULL);

    atf::process::status s = atf::process::exec(prog, argv,
        atf::process::stream_inherit(), atf::process::stream_inherit());
    if (!s.exited() || s.exitstatus() != EXIT_SUCCESS)
        throw std::runtime_error("Call to unmount failed");
#endif
}

// ------------------------------------------------------------------------
// The "temp_dir" class.
// ------------------------------------------------------------------------

impl::temp_dir::temp_dir(const atf::fs::path& p)
{
    atf::utils::auto_array< char > buf(new char[p.str().length() + 1]);
    std::strcpy(buf.get(), p.c_str());
    if (::mkdtemp(buf.get()) == NULL)
        throw system_error(IMPL_NAME "::temp_dir::temp_dir(" +
                           p.str() + ")", "mkdtemp(3) failed",
                           errno);

    m_path.reset(new atf::fs::path(buf.get()));
}

impl::temp_dir::~temp_dir(void)
{
    cleanup(*m_path);
}

const atf::fs::path&
impl::temp_dir::get_path(void)
    const
{
    return *m_path;
}

// ------------------------------------------------------------------------
// Free functions.
// ------------------------------------------------------------------------

atf::fs::path
impl::change_directory(const atf::fs::path& dir)
{
    atf::fs::path olddir = get_current_dir();

    if (olddir != dir) {
        if (::chdir(dir.c_str()) == -1)
            throw system_error(IMPL_NAME "::chdir(" + dir.str() + ")",
                               "chdir(2) failed", errno);
    }

    return olddir;
}

void
impl::cleanup(const atf::fs::path& p)
{
    atf::fs::file_info fi(p);
    cleanup_aux(p, fi.get_device(), true);
}

atf::fs::path
impl::get_current_dir(void)
{
    std::auto_ptr< char > cwd;
#if defined(HAVE_GETCWD_DYN)
    cwd.reset(getcwd(NULL, 0));
#else
    cwd.reset(getcwd(NULL, MAXPATHLEN));
#endif
    if (cwd.get() == NULL)
        throw atf::system_error(IMPL_NAME "::get_current_dir()",
                                "getcwd() failed", errno);

    return atf::fs::path(cwd.get());
}
