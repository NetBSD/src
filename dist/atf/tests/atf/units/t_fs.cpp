//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
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
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
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
#include <sys/types.h>
#include <sys/stat.h>
}

#include <fstream>

#include "atf/exceptions.hpp"
#include "atf/fs.hpp"
#include "atf/macros.hpp"

static
void
create_files(void)
{
    ::mkdir("files", 0755);
    ::mkdir("files/dir", 0755);

    std::ofstream os("files/reg");
    os.close();

    // TODO: Should create all other file types (blk, chr, fifo, lnk, sock)
    // and test for them... but the underlying file system may not support
    // most of these.  Specially as we are working on /tmp, which can be
    // mounted with flags such as "nodev".  See how to deal with this
    // situation.
}

// ------------------------------------------------------------------------
// Test cases for the "path" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(path_empty);
ATF_TEST_CASE_HEAD(path_empty)
{
    set("descr", "Tests the path's empty method");
}
ATF_TEST_CASE_BODY(path_empty)
{
    using atf::fs::path;

    ATF_CHECK( path().empty());
    ATF_CHECK(!path("foo").empty());

    path p1;
    ATF_CHECK( p1.empty());
    p1 = p1 / "foo";
    ATF_CHECK(!p1.empty());
}

ATF_TEST_CASE(path_normalize);
ATF_TEST_CASE_HEAD(path_normalize)
{
    set("descr", "Tests the path's normalization");
}
ATF_TEST_CASE_BODY(path_normalize)
{
    using atf::fs::path;

    ATF_CHECK_EQUAL(path(".").str(), ".");
    ATF_CHECK_EQUAL(path("..").str(), "..");

    ATF_CHECK_EQUAL(path("foo").str(), "foo");
    ATF_CHECK_EQUAL(path("foo/bar").str(), "foo/bar");
    ATF_CHECK_EQUAL(path("foo/bar/").str(), "foo/bar");

    ATF_CHECK_EQUAL(path("/foo").str(), "/foo");
    ATF_CHECK_EQUAL(path("/foo/bar").str(), "/foo/bar");
    ATF_CHECK_EQUAL(path("/foo/bar/").str(), "/foo/bar");

    ATF_CHECK_EQUAL(path("///foo").str(), "/foo");
    ATF_CHECK_EQUAL(path("///foo///bar").str(), "/foo/bar");
    ATF_CHECK_EQUAL(path("///foo///bar///").str(), "/foo/bar");
}

ATF_TEST_CASE(path_is_absolute);
ATF_TEST_CASE_HEAD(path_is_absolute)
{
    set("descr", "Tests the path::is_absolute function");
}
ATF_TEST_CASE_BODY(path_is_absolute)
{
    using atf::fs::path;

    ATF_CHECK( path("/").is_absolute());
    ATF_CHECK( path("////").is_absolute());
    ATF_CHECK( path("////a").is_absolute());
    ATF_CHECK( path("//a//").is_absolute());
    ATF_CHECK(!path("a////").is_absolute());
    ATF_CHECK(!path("../foo").is_absolute());
}

ATF_TEST_CASE(path_is_root);
ATF_TEST_CASE_HEAD(path_is_root)
{
    set("descr", "Tests the path::is_root function");
}
ATF_TEST_CASE_BODY(path_is_root)
{
    using atf::fs::path;

    ATF_CHECK( path("/").is_root());
    ATF_CHECK( path("////").is_root());
    ATF_CHECK(!path("////a").is_root());
    ATF_CHECK(!path("//a//").is_root());
    ATF_CHECK(!path("a////").is_root());
    ATF_CHECK(!path("../foo").is_root());
}

ATF_TEST_CASE(path_branch_path);
ATF_TEST_CASE_HEAD(path_branch_path)
{
    set("descr", "Tests the path::branch_path function");
}
ATF_TEST_CASE_BODY(path_branch_path)
{
    using atf::fs::path;

    ATF_CHECK_EQUAL(path(".").branch_path().str(), ".");
    ATF_CHECK_EQUAL(path("foo").branch_path().str(), ".");
    ATF_CHECK_EQUAL(path("foo/bar").branch_path().str(), "foo");
    ATF_CHECK_EQUAL(path("/foo").branch_path().str(), "/");
    ATF_CHECK_EQUAL(path("/foo/bar").branch_path().str(), "/foo");
}

ATF_TEST_CASE(path_leaf_name);
ATF_TEST_CASE_HEAD(path_leaf_name)
{
    set("descr", "Tests the path::leaf_name function");
}
ATF_TEST_CASE_BODY(path_leaf_name)
{
    using atf::fs::path;

    ATF_CHECK_EQUAL(path(".").leaf_name(), ".");
    ATF_CHECK_EQUAL(path("foo").leaf_name(), "foo");
    ATF_CHECK_EQUAL(path("foo/bar").leaf_name(), "bar");
    ATF_CHECK_EQUAL(path("/foo").leaf_name(), "foo");
    ATF_CHECK_EQUAL(path("/foo/bar").leaf_name(), "bar");
}

ATF_TEST_CASE(path_compare_equal);
ATF_TEST_CASE_HEAD(path_compare_equal)
{
    set("descr", "Tests the comparison for equality between paths");
}
ATF_TEST_CASE_BODY(path_compare_equal)
{
    using atf::fs::path;

    ATF_CHECK(path("/") == path("///"));
    ATF_CHECK(path("/a") == path("///a"));
    ATF_CHECK(path("/a") == path("///a///"));

    ATF_CHECK(path("a/b/c") == path("a//b//c"));
    ATF_CHECK(path("a/b/c") == path("a//b//c///"));
}

ATF_TEST_CASE(path_compare_different);
ATF_TEST_CASE_HEAD(path_compare_different)
{
    set("descr", "Tests the comparison for difference between paths");
}
ATF_TEST_CASE_BODY(path_compare_different)
{
    using atf::fs::path;

    ATF_CHECK(path("/") != path("//a/"));
    ATF_CHECK(path("/a") != path("a///"));

    ATF_CHECK(path("a/b/c") != path("a/b"));
    ATF_CHECK(path("a/b/c") != path("a//b"));
    ATF_CHECK(path("a/b/c") != path("/a/b/c"));
    ATF_CHECK(path("a/b/c") != path("/a//b//c"));
}

ATF_TEST_CASE(path_concat);
ATF_TEST_CASE_HEAD(path_concat)
{
    set("descr", "Tests the concatenation of multiple paths");
}
ATF_TEST_CASE_BODY(path_concat)
{
    using atf::fs::path;

    ATF_CHECK_EQUAL((path("foo") / "bar").str(), "foo/bar");
    ATF_CHECK_EQUAL((path("foo/") / "/bar").str(), "foo/bar");
    ATF_CHECK_EQUAL((path("foo/") / "/bar/baz").str(), "foo/bar/baz");
    ATF_CHECK_EQUAL((path("foo/") / "///bar///baz").str(), "foo/bar/baz");
}

ATF_TEST_CASE(path_to_absolute);
ATF_TEST_CASE_HEAD(path_to_absolute)
{
    set("descr", "Tests the conversion of a relative path to an absolute "
                 "one");
}
ATF_TEST_CASE_BODY(path_to_absolute)
{
    using atf::fs::file_info;
    using atf::fs::path;

    create_files();

    {
        const path p(".");
        path pa = p.to_absolute();
        ATF_CHECK(pa.is_absolute());

        file_info fi(p);
        file_info fia(pa);
        ATF_CHECK_EQUAL(fi.get_device(), fia.get_device());
        ATF_CHECK_EQUAL(fi.get_inode(), fia.get_inode());
    }

    {
        const path p("files/reg");
        path pa = p.to_absolute();
        ATF_CHECK(pa.is_absolute());

        file_info fi(p);
        file_info fia(pa);
        ATF_CHECK_EQUAL(fi.get_device(), fia.get_device());
        ATF_CHECK_EQUAL(fi.get_inode(), fia.get_inode());
    }
}

ATF_TEST_CASE(path_op_less);
ATF_TEST_CASE_HEAD(path_op_less)
{
    set("descr", "Tests that the path's less-than operator works");
}
ATF_TEST_CASE_BODY(path_op_less)
{
    using atf::fs::path;

    create_files();

    ATF_CHECK(!(path("aaa") < path("aaa")));

    ATF_CHECK(  path("aab") < path("abc"));
    ATF_CHECK(!(path("abc") < path("aab")));
}

// ------------------------------------------------------------------------
// Test cases for the "directory" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(directory_read);
ATF_TEST_CASE_HEAD(directory_read)
{
    set("descr", "Tests the directory class creation, which reads the "
                 "contents of a directory");
}
ATF_TEST_CASE_BODY(directory_read)
{
    using atf::fs::directory;
    using atf::fs::path;

    create_files();

    directory d(path("files"));
    ATF_CHECK_EQUAL(d.size(), 4);
    ATF_CHECK(d.find(".") != d.end());
    ATF_CHECK(d.find("..") != d.end());
    ATF_CHECK(d.find("dir") != d.end());
    ATF_CHECK(d.find("reg") != d.end());
}

ATF_TEST_CASE(directory_file_info);
ATF_TEST_CASE_HEAD(directory_file_info)
{
    set("descr", "Tests that the file_info objects attached to the "
                 "directory are valid");
}
ATF_TEST_CASE_BODY(directory_file_info)
{
    using atf::fs::directory;
    using atf::fs::file_info;
    using atf::fs::path;

    create_files();

    directory d(path("files"));

    {
        directory::const_iterator iter = d.find("dir");
        ATF_CHECK(iter != d.end());
        const file_info& fi = (*iter).second;
        ATF_CHECK(fi.get_path() == path("files/dir"));
        ATF_CHECK(fi.get_type() == file_info::dir_type);
    }

    {
        directory::const_iterator iter = d.find("reg");
        ATF_CHECK(iter != d.end());
        const file_info& fi = (*iter).second;
        ATF_CHECK(fi.get_path() == path("files/reg"));
        ATF_CHECK(fi.get_type() == file_info::reg_type);
    }
}

ATF_TEST_CASE(directory_names);
ATF_TEST_CASE_HEAD(directory_names)
{
    set("descr", "Tests the directory's names method");
}
ATF_TEST_CASE_BODY(directory_names)
{
    using atf::fs::directory;
    using atf::fs::path;

    create_files();

    directory d(path("files"));
    std::set< std::string > ns = d.names();
    ATF_CHECK_EQUAL(ns.size(), 4);
    ATF_CHECK(ns.find(".") != ns.end());
    ATF_CHECK(ns.find("..") != ns.end());
    ATF_CHECK(ns.find("dir") != ns.end());
    ATF_CHECK(ns.find("reg") != ns.end());
}

// ------------------------------------------------------------------------
// Test cases for the "file_info" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(file_info_stat);
ATF_TEST_CASE_HEAD(file_info_stat)
{
    set("descr", "Tests the file_info creation and its basic contents");
}
ATF_TEST_CASE_BODY(file_info_stat)
{
    using atf::fs::file_info;
    using atf::fs::path;

    create_files();

    {
        path p("files/dir");
        file_info fi(p);
        ATF_CHECK(fi.get_path() == p);
        ATF_CHECK(fi.get_type() == file_info::dir_type);
    }

    {
        path p("files/reg");
        file_info fi(p);
        ATF_CHECK(fi.get_path() == p);
        ATF_CHECK(fi.get_type() == file_info::reg_type);
    }
}

ATF_TEST_CASE(file_info_perms);
ATF_TEST_CASE_HEAD(file_info_perms)
{
    set("descr", "Tests the file_info methods to get the file's "
                 "permissions");
}
ATF_TEST_CASE_BODY(file_info_perms)
{
    using atf::fs::file_info;
    using atf::fs::path;

    path p("file");

    std::ofstream os(p.c_str());
    os.close();

#define perms(ur, uw, ux, gr, gw, gx, othr, othw, othx) \
    { \
        file_info fi(p); \
        ATF_CHECK(fi.is_owner_readable() == ur); \
        ATF_CHECK(fi.is_owner_writable() == uw); \
        ATF_CHECK(fi.is_owner_executable() == ux); \
        ATF_CHECK(fi.is_group_readable() == gr); \
        ATF_CHECK(fi.is_group_writable() == gw); \
        ATF_CHECK(fi.is_group_executable() == gx); \
        ATF_CHECK(fi.is_other_readable() == othr); \
        ATF_CHECK(fi.is_other_writable() == othw); \
        ATF_CHECK(fi.is_other_executable() == othx); \
    }

    ::chmod(p.c_str(), 0000);
    perms(false, false, false, false, false, false, false, false, false);

    ::chmod(p.c_str(), 0001);
    perms(false, false, false, false, false, false, false, false, true);

    ::chmod(p.c_str(), 0010);
    perms(false, false, false, false, false, true, false, false, false);

    ::chmod(p.c_str(), 0100);
    perms(false, false, true, false, false, false, false, false, false);

    ::chmod(p.c_str(), 0002);
    perms(false, false, false, false, false, false, false, true, false);

    ::chmod(p.c_str(), 0020);
    perms(false, false, false, false, true, false, false, false, false);

    ::chmod(p.c_str(), 0200);
    perms(false, true, false, false, false, false, false, false, false);

    ::chmod(p.c_str(), 0004);
    perms(false, false, false, false, false, false, true, false, false);

    ::chmod(p.c_str(), 0040);
    perms(false, false, false, true, false, false, false, false, false);

    ::chmod(p.c_str(), 0400);
    perms(true, false, false, false, false, false, false, false, false);

    ::chmod(p.c_str(), 0644);
    perms(true, true, false, true, false, false, true, false, false);

    ::chmod(p.c_str(), 0755);
    perms(true, true, true, true, false, true, true, false, true);

    ::chmod(p.c_str(), 0777);
    perms(true, true, true, true, true, true, true, true, true);

#undef perms
}

// ------------------------------------------------------------------------
// Test cases for the "temp_dir" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(temp_dir_raii);
ATF_TEST_CASE_HEAD(temp_dir_raii)
{
    set("descr", "Tests the RAII behavior of the temp_dir class");
}
ATF_TEST_CASE_BODY(temp_dir_raii)
{
    using atf::fs::exists;
    using atf::fs::file_info;
    using atf::fs::path;
    using atf::fs::temp_dir;

    path t1;
    path t2;

    {
        path tmpl("testdir.XXXXXX");
        temp_dir td1(tmpl);
        temp_dir td2(tmpl);
        t1 = td1.get_path();
        t2 = td2.get_path();
        ATF_CHECK(t1.str().find("XXXXXX") == std::string::npos);
        ATF_CHECK(t2.str().find("XXXXXX") == std::string::npos);
        ATF_CHECK(t1 != t2);
        ATF_CHECK(!exists(tmpl));
        ATF_CHECK( exists(t1));
        ATF_CHECK( exists(t2));

        file_info fi1(t1);
        ATF_CHECK( fi1.is_owner_readable());
        ATF_CHECK( fi1.is_owner_writable());
        ATF_CHECK( fi1.is_owner_executable());
        ATF_CHECK(!fi1.is_group_readable());
        ATF_CHECK(!fi1.is_group_writable());
        ATF_CHECK(!fi1.is_group_executable());
        ATF_CHECK(!fi1.is_other_readable());
        ATF_CHECK(!fi1.is_other_writable());
        ATF_CHECK(!fi1.is_other_executable());

        file_info fi2(t2);
        ATF_CHECK( fi2.is_owner_readable());
        ATF_CHECK( fi2.is_owner_writable());
        ATF_CHECK( fi2.is_owner_executable());
        ATF_CHECK(!fi2.is_group_readable());
        ATF_CHECK(!fi2.is_group_writable());
        ATF_CHECK(!fi2.is_group_executable());
        ATF_CHECK(!fi2.is_other_readable());
        ATF_CHECK(!fi2.is_other_writable());
        ATF_CHECK(!fi2.is_other_executable());
    }

    ATF_CHECK(!exists(t1));
    ATF_CHECK(!exists(t2));
}

// ------------------------------------------------------------------------
// Test cases for the free functions.
// ------------------------------------------------------------------------

ATF_TEST_CASE(change_directory);
ATF_TEST_CASE_HEAD(change_directory)
{
    set("descr", "Tests the change_directory function");
}
ATF_TEST_CASE_BODY(change_directory)
{
    using atf::fs::change_directory;
    using atf::fs::get_current_dir;
    using atf::fs::path;

    create_files();
    const path old = get_current_dir();

    ATF_CHECK_THROW(change_directory(path("files/reg")), atf::system_error);
    ATF_CHECK(get_current_dir() == old);

    path old2 = change_directory(path("files"));
    ATF_CHECK(old2 == old);
    path old3 = change_directory(path("dir"));
    ATF_CHECK(old3 == old2 / "files");
    path old4 = change_directory(path("../.."));
    ATF_CHECK(old4 == old3 / "dir");
    ATF_CHECK(get_current_dir() == old);
}

ATF_TEST_CASE(exists);
ATF_TEST_CASE_HEAD(exists)
{
    set("descr", "Tests the exists function");
}
ATF_TEST_CASE_BODY(exists)
{
    using atf::fs::exists;
    using atf::fs::path;

    create_files();

    ATF_CHECK( exists(path("files")));
    ATF_CHECK(!exists(path("file")));
    ATF_CHECK(!exists(path("files2")));

    ATF_CHECK( exists(path("files/.")));
    ATF_CHECK( exists(path("files/..")));
    ATF_CHECK( exists(path("files/dir")));
    ATF_CHECK( exists(path("files/reg")));
    ATF_CHECK(!exists(path("files/foo")));
}

ATF_TEST_CASE(get_current_dir);
ATF_TEST_CASE_HEAD(get_current_dir)
{
    set("descr", "Tests the get_current_dir function");
}
ATF_TEST_CASE_BODY(get_current_dir)
{
    using atf::fs::change_directory;
    using atf::fs::get_current_dir;
    using atf::fs::path;

    create_files();

    path curdir = get_current_dir();
    change_directory(path("."));
    ATF_CHECK(get_current_dir() == curdir);
    change_directory(path("files"));
    ATF_CHECK(get_current_dir() == curdir / "files");
    change_directory(path("dir"));
    ATF_CHECK(get_current_dir() == curdir / "files/dir");
    change_directory(path(".."));
    ATF_CHECK(get_current_dir() == curdir / "files");
    change_directory(path(".."));
    ATF_CHECK(get_current_dir() == curdir);
}

ATF_TEST_CASE(is_executable);
ATF_TEST_CASE_HEAD(is_executable)
{
    set("descr", "Tests the is_executable function");
}
ATF_TEST_CASE_BODY(is_executable)
{
    using atf::fs::is_executable;
    using atf::fs::path;

    create_files();

    ATF_CHECK( is_executable(path("files")));
    ATF_CHECK( is_executable(path("files/.")));
    ATF_CHECK( is_executable(path("files/..")));
    ATF_CHECK( is_executable(path("files/dir")));

    ATF_CHECK(!is_executable(path("non-existent")));

    ATF_CHECK(!is_executable(path("files/reg")));
    ATF_CHECK(::chmod("files/reg", 0755) != -1);
    ATF_CHECK( is_executable(path("files/reg")));
}

ATF_TEST_CASE(cleanup);
ATF_TEST_CASE_HEAD(cleanup)
{
    set("descr", "Tests the cleanup function");
}
ATF_TEST_CASE_BODY(cleanup)
{
    using atf::fs::cleanup;
    using atf::fs::get_current_dir;
    using atf::fs::exists;
    using atf::fs::path;

    create_files();

    path p("files");
    ATF_CHECK( exists(p));
    ATF_CHECK( exists(p / "dir"));
    ATF_CHECK( exists(p / "reg"));
    cleanup(p);
    ATF_CHECK(!exists(p));
}

ATF_TEST_CASE(remove);
ATF_TEST_CASE_HEAD(remove)
{
    set("descr", "Tests the remove function");
}
ATF_TEST_CASE_BODY(remove)
{
    using atf::fs::exists;
    using atf::fs::path;
    using atf::fs::remove;

    create_files();

    ATF_CHECK( exists(path("files/reg")));
    remove(path("files/reg"));
    ATF_CHECK(!exists(path("files/reg")));

    ATF_CHECK( exists(path("files/dir")));
    ATF_CHECK_THROW(remove(path("files/dir")), atf::system_error);
    ATF_CHECK( exists(path("files/dir")));
}

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add the tests for the "path" class.
    ATF_ADD_TEST_CASE(tcs, path_empty);
    ATF_ADD_TEST_CASE(tcs, path_normalize);
    ATF_ADD_TEST_CASE(tcs, path_is_absolute);
    ATF_ADD_TEST_CASE(tcs, path_is_root);
    ATF_ADD_TEST_CASE(tcs, path_branch_path);
    ATF_ADD_TEST_CASE(tcs, path_leaf_name);
    ATF_ADD_TEST_CASE(tcs, path_compare_equal);
    ATF_ADD_TEST_CASE(tcs, path_compare_different);
    ATF_ADD_TEST_CASE(tcs, path_concat);
    ATF_ADD_TEST_CASE(tcs, path_to_absolute);
    ATF_ADD_TEST_CASE(tcs, path_op_less);

    // Add the tests for the "file_info" class.
    ATF_ADD_TEST_CASE(tcs, file_info_stat);
    ATF_ADD_TEST_CASE(tcs, file_info_perms);

    // Add the tests for the "directory" class.
    ATF_ADD_TEST_CASE(tcs, directory_read);
    ATF_ADD_TEST_CASE(tcs, directory_names);
    ATF_ADD_TEST_CASE(tcs, directory_file_info);

    // Add the tests for the "temp_dir" class.
    ATF_ADD_TEST_CASE(tcs, temp_dir_raii);

    // Add the tests for the free functions.
    ATF_ADD_TEST_CASE(tcs, get_current_dir);
    ATF_ADD_TEST_CASE(tcs, exists);
    ATF_ADD_TEST_CASE(tcs, is_executable);
    ATF_ADD_TEST_CASE(tcs, change_directory);
    ATF_ADD_TEST_CASE(tcs, cleanup);
    ATF_ADD_TEST_CASE(tcs, remove);
}
