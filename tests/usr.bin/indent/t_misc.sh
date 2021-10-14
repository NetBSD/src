#! /bin/sh
# $NetBSD: t_misc.sh,v 1.2 2021/10/14 18:55:41 rillig Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
# All rights reserved.
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
# $FreeBSD$

# Tests for indent that do not follow the input-profile-output scheme that is
# used in t_indent.

indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)
nl='
'

atf_test_case 'in_place'
in_place_body()
{
	cat <<-\EOF > code.c
		int decl;
	EOF
	cat <<-\EOF > code.c.exp
		int		decl;
	EOF
	cp code.c code.c.orig

	atf_check \
	    env SIMPLE_BACKUP_SUFFIX=".bak" "$indent" code.c
	atf_check -o 'file:code.c.exp' \
	    cat code.c
	atf_check -o 'file:code.c.orig' \
	    cat code.c.bak
}

atf_test_case 'verbose_profile'
verbose_profile_body()
{
	cat <<-\EOF > .indent.pro
		-/* comment */bacc
		-v
		-fc1
	EOF
	cat <<-\EOF > before.c
		int decl;
	EOF
	cat <<-\EOF > after.c.exp
		int		decl;
	EOF
	cat <<-\EOF > stdout.exp
		profile: -fc1
		profile: -bacc
		profile: -v
		profile: -fc1
		There were 1 output lines and 0 comments
		(Lines with comments)/(Lines with code):  0.000
	EOF

	# The code in args.c function set_profile suggests that options from
	# profile files are echoed to stdout during startup. But since the
	# command line options are handled after the profile files, a '-v' in
	# the command line has no effect. That's why '-bacc' is not listed
	# in stdout, but '-fc1' is. The second round of '-bacc', '-v', '-fc1'
	# is listed because when running ATF, $HOME equals $PWD.

	atf_check \
	    -o 'file:stdout.exp' \
	    "$indent" -v before.c after.c
	atf_check \
	     -o 'file:after.c.exp' \
	     cat after.c
}

atf_init_test_cases()
{
	atf_add_test_case 'in_place'
	atf_add_test_case 'verbose_profile'
}
