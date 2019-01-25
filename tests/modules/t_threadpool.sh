# $NetBSD: t_threadpool.sh,v 1.1 2019/01/25 18:34:45 christos Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jason R. Thorpe.
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

# Pick an arbitrary priority that is not likely to be used.
tp_pri=5

# The kernel test jig includes a 1 second delay in the job.  We need to
# wait longer for it to complete.
job_delay=2

read_sysctl() {
	echo "${1} = ${2}" >expout
	atf_check -s eq:0 -o file:expout -e empty sysctl ${1}
}

write_sysctl() {
	atf_check -s eq:0 -o ignore -e empty sysctl -w "${1}=${2}"
}

write_sysctl_fail() {
	echo "${3}" >experr
	atf_check -s eq:1 -o ignore -e file:experr sysctl -w "${1}=${2}"
}

atf_test_case unbound cleanup
unbound_head() {
	atf_set "descr" "Test unbound thread pools"
	atf_set "require.user" "root"
}
unbound_body() {
	modload $(atf_get_srcdir)/threadpool_tester/threadpool_tester.kmod
	if [ $? -ne 0 ]; then
		atf_skip "cannot load threadpool_tester.kmod"
	fi

	# Ensure that the state is clean.
	read_sysctl kern.threadpool_tester.test_value 0

	# Create an unbound pool.
	write_sysctl kern.threadpool_tester.get_unbound $tp_pri

	# Do it again.  We expect this to fail, but the test jig will
	# do some additional threadpool object lifecycle validation.
	# (It will not hold the additional reference.)
	write_sysctl_fail kern.threadpool_tester.get_unbound $tp_pri \
	    "sysctl: kern.threadpool_tester.get_unbound: File exists"

	# Schedule the test jig job on the pool.
	# Wait for a short period of time and then check that the job
	# successfully ran.
	write_sysctl kern.threadpool_tester.run_unbound $tp_pri
	sleep $job_delay
	read_sysctl kern.threadpool_tester.test_value 1

	# ...and again.
	write_sysctl kern.threadpool_tester.run_unbound $tp_pri
	sleep $job_delay
	read_sysctl kern.threadpool_tester.test_value 2

	# Now destroy the threadpool.
	write_sysctl kern.threadpool_tester.put_unbound $tp_pri
}
unbound_cleanup() {
	modunload threadpool_tester >/dev/null 2>&1
}

atf_test_case percpu cleanup
percpu_head() {
	atf_set "descr" "Test percpu thread pools"
	atf_set "require.user" "root"
}
percpu_body() {
	modload $(atf_get_srcdir)/threadpool_tester/threadpool_tester.kmod
	if [ $? -ne 0 ]; then
		atf_skip "cannot load threadpool_tester.kmod"
	fi

	# Ensure that the state is clean.
	read_sysctl kern.threadpool_tester.test_value 0

	# Create an percpu pool.
	write_sysctl kern.threadpool_tester.get_percpu $tp_pri

	# Do it again.  We expect this to fail, but the test jig will
	# do some additional threadpool object lifecycle validation.
	# (It will not hold the additional reference.)
	write_sysctl_fail kern.threadpool_tester.get_percpu $tp_pri \
	    "sysctl: kern.threadpool_tester.get_percpu: File exists"

	# Schedule the test jig job on the pool.
	# Wait for a short period of time and then check that the job
	# successfully ran.
	write_sysctl kern.threadpool_tester.run_percpu $tp_pri
	sleep $job_delay
	read_sysctl kern.threadpool_tester.test_value 1

	# ...and again.
	write_sysctl kern.threadpool_tester.run_percpu $tp_pri
	sleep $job_delay
	read_sysctl kern.threadpool_tester.test_value 2

	# Now destroy the threadpool.
	write_sysctl kern.threadpool_tester.put_percpu $tp_pri
}
percpu_cleanup() {
	modunload threadpool_tester >/dev/null 2>&1
}

atf_init_test_cases()
{
	atf_add_test_case unbound
	atf_add_test_case percpu
}
