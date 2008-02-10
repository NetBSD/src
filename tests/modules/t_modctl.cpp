// $NetBSD: t_modctl.cpp,v 1.2 2008/02/10 16:02:24 jmmv Exp $
//
// Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: t_modctl.cpp,v 1.2 2008/02/10 16:02:24 jmmv Exp $");

#include <sys/module.h>
#include <sys/sysctl.h>
}

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>

#include <atf.hpp>

static bool have_modular = false;

enum presence_check { both_checks, stat_check, sysctl_check };

/* --------------------------------------------------------------------- */
/* Auxiliary functions                                                   */
/* --------------------------------------------------------------------- */

/*
 * Checks if the kernel has 'options MODULAR' built into it and returns
 * a boolean indicating this condition.  This function must be called
 * during the test program's initialization and the result be stored
 * globally for further (efficient) usage of require_modular().
 */
static
bool
check_modular(void)
{
	bool res;
	struct iovec iov;

	iov.iov_base = NULL;
	iov.iov_len = 0;

	if (::modctl(MODCTL_STAT, &iov) == 0)
		res = true;
	else
		res = (errno != ENOSYS);

	return res;
}

/*
 * Makes sure that the kernel has 'options MODULAR' built into it and
 * skips the test otherwise.  Cannot be called unless check_modular()
 * has been executed before.
 */
static
void
require_modular(void)
{

	if (!have_modular)
		ATF_SKIP("Kernel does not have 'options MODULAR'.");
}

static
bool
get_modstat_info(const char *name, modstat_t *msdest)
{
	bool found;
	size_t len;
	struct iovec iov;
	modstat_t *ms;

	for (len = 4096; ;) {
		iov.iov_base = ::malloc(len);
		iov.iov_len = len;
		if (::modctl(MODCTL_STAT, &iov) != 0) {
			int err = errno;
			std::cerr << "modctl(MODCTL_STAT) failed: "
			    << std::strerror(err)
			    << std::endl;
			ATF_FAIL("Failed to query module status");
		}
		if (len >= iov.iov_len)
			break;
		::free(iov.iov_base);
		len = iov.iov_len;
	}

	found = false;
	len = iov.iov_len / sizeof(modstat_t);
	for (ms = (modstat_t *)iov.iov_base; len != 0 && !found;
	    ms++, len--) {
		if (std::strcmp(ms->ms_name, name) == 0) {
			if (msdest != NULL)
				*msdest = *ms;
			found = true;
		}
	}

	::free(iov.iov_base);

	return found;
}

/*
 * Returns a boolean indicating if the k_helper module was loaded
 * successfully.  This implementation uses modctl(2)'s MODCTL_STAT
 * subcommand to do the check.
 */
static
bool
k_helper_is_present_stat(void)
{

	return get_modstat_info("k_helper", NULL);
}

/*
 * Returns a boolean indicating if the k_helper module was loaded
 * successfully.  This implementation uses the module's sysctl
 * installed node to do the check.
 */
static
bool
k_helper_is_present_sysctl(void)
{
	size_t present, presentsize;
	int ret;

	presentsize = sizeof(present);
	ret = ::sysctlbyname("vendor.k_helper.present", &present,
	    &presentsize, NULL, 0);
	if (ret == -1 && errno != ENOENT) {
		int err = errno;
		std::cerr << "sysctlbyname(2) failed: " << std::strerror(err)
		    << std::endl;
		ATF_FAIL("Failed to query module status");
	}
	return ret == 0;
}

/*
 * Returns a boolean indicating if the k_helper module was loaded
 * successfully.  The 'how' parameter specifies the implementation to
 * use to do the check.
 */
static
bool
k_helper_is_present(presence_check how)
{
	bool found;

	switch (how) {
	case both_checks:
		found = k_helper_is_present_stat();
		ATF_CHECK(k_helper_is_present_sysctl() == found);
		break;

	case stat_check:
		found = k_helper_is_present_stat();
		break;

	case sysctl_check:
		found = k_helper_is_present_sysctl();
		break;

	default:
		assert(false);
	}

	return found;
}

/*
 * Loads the specified module from a file.  If fatal is set and an error
 * occurs when loading the module, an error message is printed and the
 * test case is aborted.
 */
static
int
load(const std::string& filename, bool fatal = true)
{
	int err;

	std::cout << "Loading module " << filename << std::endl;
	err = 0;
	if (::modctl(MODCTL_LOAD, __UNCONST(filename.c_str())) == -1) {
		err = errno;
		std::cerr << "modctl(MODCTL_LOAD, " << filename
		    << ") failed: " << std::strerror(err)
		    << std::endl;
		if (fatal)
			ATF_FAIL("Module load failed");
	}
	return err;
}

/*
 * Unloads the specified module.  If silent is true, nothing will be
 * printed and no errors will be raised if the unload was unsuccessful.
 */
static
int
unload(const char *name, bool fatal = true)
{
	int err;

	std::cout << "Unloading module " << name << std::endl;
	err = 0;
	if (::modctl(MODCTL_UNLOAD, __UNCONST(name)) == -1) {
		err = errno;
		std::cerr << "modctl(MODCTL_UNLOAD, " << name
		    << ") failed: " << std::strerror(err)
		    << std::endl;
		if (fatal)
			ATF_FAIL("Module unload failed");
	}
	return err;
}

/*
 * A silent version of unload, to be called as part of the cleanup
 * process only.
 */
static
int
unload_cleanup(const char *name)
{

	(void)::modctl(MODCTL_UNLOAD, __UNCONST(name));
}

/* --------------------------------------------------------------------- */
/* Test cases                                                            */
/* --------------------------------------------------------------------- */

ATF_TEST_CASE_WITH_CLEANUP(cmd_load);
ATF_TEST_CASE_HEAD(cmd_load)
{
	set("descr", "Tests for the MODCTL_LOAD command");
	set("require.user", "root");
}
ATF_TEST_CASE_BODY(cmd_load)
{
	require_modular();

	ATF_CHECK(load("", false) == ENOENT);
	ATF_CHECK(load("non-existent.o", false) == ENOENT);

	std::string longname(MAXPATHLEN, 'a');
	ATF_CHECK(load(longname.c_str(), false) == ENAMETOOLONG);

	ATF_CHECK(!k_helper_is_present(stat_check));
	load(get_srcdir() + "/k_helper.o");
	std::cout << "Checking if load was successful" << std::endl;
	ATF_CHECK(k_helper_is_present(stat_check));
}
ATF_TEST_CASE_CLEANUP(cmd_load)
{
	unload_cleanup("k_helper");
}

ATF_TEST_CASE_WITH_CLEANUP(cmd_stat);
ATF_TEST_CASE_HEAD(cmd_stat)
{
	set("descr", "Tests for the MODCTL_STAT command");
	set("require.user", "root");
}
ATF_TEST_CASE_BODY(cmd_stat)
{
	require_modular();

	ATF_CHECK(!k_helper_is_present(both_checks));

	load(get_srcdir() + "/k_helper.o");
	ATF_CHECK(k_helper_is_present(both_checks));
	{
		modstat_t ms;
		ATF_CHECK(get_modstat_info("k_helper", &ms));

		ATF_CHECK(ms.ms_class == MODULE_CLASS_MISC);
		ATF_CHECK(ms.ms_source == MODULE_SOURCE_FILESYS);
		ATF_CHECK(ms.ms_refcnt == 0);
	}
	unload("k_helper");

	ATF_CHECK(!k_helper_is_present(both_checks));
}
ATF_TEST_CASE_CLEANUP(cmd_stat)
{
	unload_cleanup("k_helper");
}

ATF_TEST_CASE_WITH_CLEANUP(cmd_unload);
ATF_TEST_CASE_HEAD(cmd_unload)
{
	set("descr", "Tests for the MODCTL_UNLOAD command");
	set("require.user", "root");
}
ATF_TEST_CASE_BODY(cmd_unload)
{
	require_modular();

	load(get_srcdir() + "/k_helper.o");

	ATF_CHECK(unload("", false) == ENOENT);
	ATF_CHECK(unload("non-existent.o", false) == ENOENT);
	ATF_CHECK(unload("k_helper.o", false) == ENOENT);

	ATF_CHECK(k_helper_is_present(stat_check));
	unload("k_helper");
	std::cout << "Checking if unload was successful" << std::endl;
	ATF_CHECK(!k_helper_is_present(stat_check));
}
ATF_TEST_CASE_CLEANUP(cmd_unload)
{
	unload_cleanup("k_helper");
}

/* --------------------------------------------------------------------- */
/* Main                                                                  */
/* --------------------------------------------------------------------- */

ATF_INIT_TEST_CASES(tcs)
{
	have_modular = check_modular();

	ATF_ADD_TEST_CASE(tcs, cmd_load);
	ATF_ADD_TEST_CASE(tcs, cmd_stat);
	ATF_ADD_TEST_CASE(tcs, cmd_unload);
}
