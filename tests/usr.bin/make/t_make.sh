# $NetBSD: t_make.sh,v 1.3 2014/08/23 14:50:24 christos Exp $
#
# Copyright (c) 2008, 2010, 2014 The NetBSD Foundation, Inc.
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

# Executes make and compares the output to a golden file.
run_and_check()
{
	local atfname="${1}"; shift
	local makename="${1}"; shift

	local srcdir="$(atf_get_srcdir)"
	local in="${srcdir}/d_${name}.mk"
	local out="${srcdir}/d_${name}.out"

	if [ "x${name}" = "xposix" ]; then
		# Include $(INPUTFILE) for d_posix.mk, so it can re-run make
		# on the same makefile.  Make sets $(MAKEFILE), but it is
		# not in POSIX, so it can't be used as such.  It can't be
		# set explicitly because make always sets it itself and
		# the test shouldn't use anything not provided for by in
		# the POSIX standard.
		args="INPUTFILE='${in}'"
		atf_expect_fail 'PR/49086 [$(<)], PR/49092 [output order]'
		atf_check -o file:"${out}" -x \
		    "make -kf'${in}' ${args} 2>&1 | sed -e 's,${srcdir}/d_,,'"
	else
		local testdir="$(atf_get_srcdir)/unit-tests"

		atf_check -s exit:0 -o ignore -e ignore \
		make -f "${testdir}/Makefile" "${makename}.out"
		atf_check -o file:"${testdir}/${makename}.exp" \
		    cat "${makename}.out"
	fi
}

# Defines a test case for make(1), parsing a given file and comparing the
# output to prerecorded results.
test_case()
{
	local atfname="${1}"; shift	# e.g. foo_bar
	local makename="${1}"; shift	# e.g. foo-bar
	local descr="${1}"; shift

	atf_test_case "${atfname}"
	eval "${atfname}_head() { \
		if [ -n '${descr}' ]; then \
		    atf_set descr '${descr}'; \
		fi \
	}"
	eval "${atfname}_body() { \
		run_and_check '${atfname}' '${makename}'; \
	}"
}

atf_init_test_cases()
{
	local filename basename atfname descr

	#exec >/tmp/apb 2>&1
	#set -x
	for filename in "$(atf_get_srcdir)"/unit-tests/*.mk ; do
	    basename="${filename##*/}"
	    basename="${basename%.mk}"
	    atfname="$(echo "${basename}" | tr "x-" "x_")"
	    descr='' # XXX
            test_case "${atfname}" "${basename}" "${descr}"
	    atf_add_test_case "${atfname}"
	done
	#set +x
}
