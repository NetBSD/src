# $NetBSD: t_findcc.sh,v 1.3 2021/08/20 06:36:10 rillig Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Roland Illig.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

n='
'

# A plain program name is searched in the PATH.  Since in this case, the
# environment is empty, nothing is found.
#
atf_test_case base_not_found
base_not_found_body() {
	atf_check -o "inline:(not found)$n" \
	    env -i \
		"$(atf_get_srcdir)"/h_findcc 'echo'
}

# A plain program name is searched in the PATH and, in this example, it is
# found in '/bin'.
#
atf_test_case base_found
base_found_body() {
	atf_check -o "inline:/bin/echo$n" \
	    env -i PATH='/bin:/nonexistent' \
		"$(atf_get_srcdir)"/h_findcc 'echo'
}

# A plain program name is searched in the PATH and, in this example, it is
# found in '/bin', which comes second in the PATH.
#
atf_test_case base_found_second
base_found_second_body() {
	atf_check -o "inline:/bin/echo$n" \
	    env -i PATH='/nonexistent:/bin' \
		"$(atf_get_srcdir)"/h_findcc 'echo'
}

# A plain program name is searched in the PATH and, in this example, it is
# found in './bin', a relative path in the PATH, which is rather unusual in
# practice.
#
atf_test_case base_found_reldir
base_found_reldir_body() {
	mkdir bin
	echo '#! /bin/sh' > 'bin/reldir-echo'
	chmod +x 'bin/reldir-echo'

	atf_check -o "inline:bin/reldir-echo$n" \
	    env -i PATH='/nonexistent:bin' \
		"$(atf_get_srcdir)"/h_findcc 'reldir-echo'
}

# The C compiler can be specified as a program with one or more arguments.
# If the program name is a plain name without any slash, the argument is
# discarded.
#
# XXX: Discarding the arguments feels unintended.
#
atf_test_case base_arg_found
base_arg_found_body() {
	atf_check -o "inline:/bin/echo$n" \
	    env -i PATH='/bin:/nonexistent' \
		"$(atf_get_srcdir)"/h_findcc 'echo arg'
}


# If the program name contains a slash, no matter where, the program is not
# searched in the PATH.  This is the same behavior as in /bin/sh.
#
atf_test_case rel_not_found
rel_not_found_body() {
	atf_check -o "inline:(not found)$n" \
	    env -i PATH='/' \
		"$(atf_get_srcdir)"/h_findcc 'bin/echo'
}

# If the program name contains a slash, no matter where, the program is not
# searched in the PATH.  This is the same behavior as in /bin/sh.
#
atf_test_case rel_found
rel_found_body() {
	mkdir bin
	echo '#! /bin/sh' > bin/echo
	chmod +x bin/echo

	atf_check -o "inline:bin/echo$n" \
	    env -i PATH='/' \
		"$(atf_get_srcdir)"/h_findcc 'bin/echo'
}

# If the program name contains a slash in the middle and has additional
# arguments, the arguments are discarded.
#
# XXX: Discarding the arguments feels unintended.
#
atf_test_case rel_arg_found
rel_arg_found_body() {
	mkdir bin
	echo '#! /bin/sh' > bin/echo
	chmod +x bin/echo

	atf_check -o "inline:bin/echo$n" \
	    env -i PATH='/' \
		"$(atf_get_srcdir)"/h_findcc 'bin/echo arg'
}


atf_test_case abs_not_found
abs_not_found_body() {
	atf_check -o "inline:(not found)$n" \
	    env -i \
		"$(atf_get_srcdir)"/h_findcc "$PWD/nonexistent/echo"
}

atf_test_case abs_found
abs_found_body() {
	mkdir bin
	echo '#! /bin/sh' > bin/echo
	chmod +x bin/echo

	atf_check -o "inline:$PWD/bin/echo$n" \
	    env -i \
		"$(atf_get_srcdir)"/h_findcc "$PWD/bin/echo"
}

# If the program name is an absolute pathname, the arguments are discarded.
#
# XXX: Discarding the arguments feels unintended.
#
atf_test_case abs_arg_found
abs_arg_found_body() {
	mkdir bin
	echo '#! /bin/sh' > bin/echo
	chmod +x bin/echo

	atf_check -o "inline:$PWD/bin/echo$n" \
	    env -i \
		"$(atf_get_srcdir)"/h_findcc "$PWD/bin/echo arg"
}


atf_init_test_cases() {
	atf_add_test_case base_not_found
	atf_add_test_case base_found
	atf_add_test_case base_found_second
	atf_add_test_case base_found_reldir
	atf_add_test_case base_arg_found

	atf_add_test_case rel_not_found
	atf_add_test_case rel_found
	atf_add_test_case rel_arg_found

	atf_add_test_case abs_not_found
	atf_add_test_case abs_found
	atf_add_test_case abs_arg_found
}
