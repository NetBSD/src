# $NetBSD: t_id.sh,v 1.2.4.2 2008/01/09 01:59:38 matt Exp $
#
# Copyright (c) 2007 The NetBSD Foundation, Inc.
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
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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

run_id() {
	[ -f ./id ] || ln -s $(atf_get_srcdir)/h_id ./id
	./id "${@}"
}

atf_test_case default
default_head() {
	atf_set "descr" "Checks that the output without options is correct"
}
default_body() {
	echo "uid=100(test) gid=100(users) groups=100(users),0(wheel)" >expout
	atf_check "run_id" 0 expout null
	atf_check "run_id 100" 0 expout null
	atf_check "run_id test" 0 expout null

	echo "uid=0(root) gid=0(wheel) groups=0(wheel)" >expout
	atf_check "run_id 0" 0 expout null
	atf_check "run_id root" 0 expout null

	export LIBFAKE_EGID_ROOT=1 LIBFAKE_EUID_ROOT=1
	echo "uid=100(test) gid=100(users) euid=0(root) egid=0(wheel) groups=100(users),0(wheel)" >expout
	atf_check "run_id" 0 expout null
	unset LIBFAKE_EGID_ROOT LIBFAKE_EUID_ROOT

	echo 'id: nonexistent: No such user' >experr
	atf_check "run_id nonexistent" 1 null experr

	atf_check "run_id root nonexistent" 1 null stderr
	atf_check "grep ^usage: stderr" 0 ignore null
}

atf_test_case primaries
primaries_head() {
	atf_set "descr" "Checks that giving multiple primaries" \
	                "simultaneously provides an error"
}
primaries_body() {
	for p1 in -G -g -p -u; do
		for p2 in -G -g -p -u; do
			if [ ${p1} != ${p2} ]; then
				atf_check "run_id ${p1} ${p2}" 1 null stderr
				atf_check "grep ^usage: stderr" 0 ignore null
			fi
		done
	done
}

atf_test_case Gflag
Gflag_head() {
	atf_set "descr" "Checks that the -G primary flag works"
}
Gflag_body() {
	echo "100 0" >expout
	atf_check "run_id -G" 0 expout null
	atf_check "run_id -G 100" 0 expout null
	atf_check "run_id -G test" 0 expout null

	echo "users wheel" >expout
	atf_check "run_id -G -n" 0 expout null
	atf_check "run_id -G -n 100" 0 expout null
	atf_check "run_id -G -n test" 0 expout null

	echo "0" >expout
	atf_check "run_id -G 0" 0 expout null
	atf_check "run_id -G root" 0 expout null

	echo "wheel" >expout
	atf_check "run_id -G -n 0" 0 expout null
	atf_check "run_id -G -n root" 0 expout null

	echo 'id: nonexistent: No such user' >experr
	atf_check "run_id -G nonexistent" 1 null experr

	atf_check "run_id -G root nonexistent" 1 null stderr
	atf_check "grep ^usage: stderr" 0 ignore null
}

atf_test_case gflag
gflag_head() {
	atf_set "descr" "Checks that the -g primary flag works"
}
gflag_body() {
	echo "100" >expout
	atf_check "run_id -g" 0 expout null
	atf_check "run_id -g 100" 0 expout null
	atf_check "run_id -g test" 0 expout null

	echo "users" >expout
	atf_check "run_id -g -n" 0 expout null
	atf_check "run_id -g -n 100" 0 expout null
	atf_check "run_id -g -n test" 0 expout null

	echo "0" >expout
	atf_check "run_id -g 0" 0 expout null
	atf_check "run_id -g root" 0 expout null

	echo "wheel" >expout
	atf_check "run_id -g -n 0" 0 expout null
	atf_check "run_id -g -n root" 0 expout null

	echo "100" >expout
	atf_check "run_id -g -r" 0 expout null

	echo "users" >expout
	atf_check "run_id -g -r -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -g -r 100" 0 expout null
	atf_check "run_id -g -r test" 0 expout null

	echo "users" >expout
	atf_check "run_id -g -r -n 100" 0 expout null
	atf_check "run_id -g -r -n test" 0 expout null

	export LIBFAKE_EGID_ROOT=1 LIBFAKE_EUID_ROOT=1

	echo "0" >expout
	atf_check "run_id -g" 0 expout null

	echo "wheel" >expout
	atf_check "run_id -g -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -g -r" 0 expout null

	echo "users" >expout
	atf_check "run_id -g -r -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -g -r 100" 0 expout null
	atf_check "run_id -g -r test" 0 expout null

	echo "users" >expout
	atf_check "run_id -g -r -n 100" 0 expout null
	atf_check "run_id -g -r -n test" 0 expout null

	unset LIBFAKE_EGID_ROOT LIBFAKE_EUID_ROOT

	echo 'id: nonexistent: No such user' >experr
	atf_check "run_id -g nonexistent" 1 null experr

	atf_check "run_id -g root nonexistent" 1 null stderr
	atf_check "grep ^usage: stderr" 0 ignore null
}

atf_test_case pflag
pflag_head() {
	atf_set "descr" "Checks that the -p primary flag works"
}
pflag_body() {
	cat >expout <<EOF
uid	test
groups	users wheel
EOF
	atf_check "run_id -p" 0 expout null
	atf_check "run_id -p 100" 0 expout null
	atf_check "run_id -p test" 0 expout null

	cat >expout <<EOF
uid	root
groups	wheel
EOF
	atf_check "run_id -p 0" 0 expout null
	atf_check "run_id -p root" 0 expout null

	export LIBFAKE_EGID_ROOT=1 LIBFAKE_EUID_ROOT=1
	cat >expout <<EOF
uid	test
euid	root
rgid	users
groups	users wheel
EOF
	atf_check "run_id -p" 0 expout null
	unset LIBFAKE_EGID_ROOT LIBFAKE_EUID_ROOT

	echo 'id: nonexistent: No such user' >experr
	atf_check "run_id -p nonexistent" 1 null experr

	atf_check "run_id -p root nonexistent" 1 null stderr
	atf_check "grep ^usage: stderr" 0 ignore null
}

atf_test_case uflag
uflag_head() {
	atf_set "descr" "Checks that the -u primary flag works"
}
uflag_body() {
	echo "100" >expout
	atf_check "run_id -u" 0 expout null
	atf_check "run_id -u 100" 0 expout null
	atf_check "run_id -u test" 0 expout null

	echo "test" >expout
	atf_check "run_id -u -n" 0 expout null
	atf_check "run_id -u -n 100" 0 expout null
	atf_check "run_id -u -n test" 0 expout null

	echo "0" >expout
	atf_check "run_id -u 0" 0 expout null
	atf_check "run_id -u root" 0 expout null

	echo "root" >expout
	atf_check "run_id -u -n 0" 0 expout null
	atf_check "run_id -u -n root" 0 expout null

	echo "100" >expout
	atf_check "run_id -u -r" 0 expout null

	echo "test" >expout
	atf_check "run_id -u -r -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -u -r 100" 0 expout null
	atf_check "run_id -u -r test" 0 expout null

	echo "test" >expout
	atf_check "run_id -u -r -n 100" 0 expout null
	atf_check "run_id -u -r -n test" 0 expout null

	export LIBFAKE_EGID_ROOT=1 LIBFAKE_EUID_ROOT=1

	echo "0" >expout
	atf_check "run_id -u" 0 expout null

	echo "root" >expout
	atf_check "run_id -u -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -u -r" 0 expout null

	echo "test" >expout
	atf_check "run_id -u -r -n" 0 expout null

	echo "100" >expout
	atf_check "run_id -u -r 100" 0 expout null
	atf_check "run_id -u -r test" 0 expout null

	echo "test" >expout
	atf_check "run_id -u -r -n 100" 0 expout null
	atf_check "run_id -u -r -n test" 0 expout null

	unset LIBFAKE_EGID_ROOT LIBFAKE_EUID_ROOT

	echo 'id: nonexistent: No such user' >experr
	atf_check "run_id -u nonexistent" 1 null experr

	atf_check "run_id -u root nonexistent" 1 null stderr
	atf_check "grep ^usage: stderr" 0 ignore null
}

atf_init_test_cases()
{
	atf_add_test_case default
	atf_add_test_case primaries
	atf_add_test_case Gflag
	atf_add_test_case gflag
	atf_add_test_case pflag
	atf_add_test_case uflag
}
