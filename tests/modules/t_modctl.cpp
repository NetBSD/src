// $NetBSD: t_modctl.cpp,v 1.1 2008/02/10 12:40:10 jmmv Exp $
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
__KERNEL_RCSID(0, "$NetBSD: t_modctl.cpp,v 1.1 2008/02/10 12:40:10 jmmv Exp $");

#include <sys/module.h>
#include <sys/sysctl.h>
}

#include <cerrno>
#include <cstring>
#include <iostream>

#include <atf.hpp>

static bool have_modular = false;

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

/*
 * Returns a boolean indicating if the k_helper module was loaded
 * successfully.
 */
static
bool
k_helper_is_present(void)
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
 * Loads the specified module from a file.
 */
static
void
load(const std::string& filename)
{

	std::cout << "Loading module " << filename << std::endl;
	if (::modctl(MODCTL_LOAD, __UNCONST(filename.c_str())) == -1) {
		int err = errno;
		std::cerr << "modctl(MODCTL_LOAD, " << filename
		    << ") failed: " << std::strerror(err)
		    << std::endl;
		ATF_FAIL("Module load failed");
	}
}

/*
 * Unloads the specified module.  If silent is true, nothing will be
 * printed and no errors will be raised if the unload was unsuccessful.
 */
static
void
unload(const char *name, bool silent = false)
{

	if (!silent) {
		std::cout << "Unloading module " << name << std::endl;
		if (::modctl(MODCTL_UNLOAD, __UNCONST(name)) == -1) {
			int err = errno;
			std::cerr << "modctl(MODCTL_UNLOAD, " << name
			    << ") failed: " << std::strerror(err)
			    << std::endl;
			ATF_FAIL("Module unload failed");
		}
	} else {
		(void)::modctl(MODCTL_UNLOAD, __UNCONST(name));
	}
}

/* --------------------------------------------------------------------- */
/* Test cases                                                            */
/* --------------------------------------------------------------------- */

ATF_TEST_CASE_WITH_CLEANUP(plain);
ATF_TEST_CASE_HEAD(plain)
{
	set("descr", "Checks that plain loading and unloading works");
	set("require.user", "root");
}
ATF_TEST_CASE_BODY(plain)
{
	require_modular();

	load(get_srcdir() + "/k_helper.o");

	std::cout << "Checking if load was successful" << std::endl;
	ATF_CHECK(k_helper_is_present());

	unload("k_helper");

	std::cout << "Checking if unload was successful" << std::endl;
	ATF_CHECK(!k_helper_is_present());
}
ATF_TEST_CASE_CLEANUP(plain)
{
	unload("k_helper", true);
}

/* --------------------------------------------------------------------- */
/* Main                                                                  */
/* --------------------------------------------------------------------- */

ATF_INIT_TEST_CASES(tcs)
{
	have_modular = check_modular();

	ATF_ADD_TEST_CASE(tcs, plain);
}
