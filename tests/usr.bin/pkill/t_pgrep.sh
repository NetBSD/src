# $NetBSD: t_pgrep.sh,v 1.2 2017/02/21 21:22:45 kre Exp $
#
# Copyright (c) 2016 The NetBSD Foundation, Inc.
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

atf_test_case pr50934
basic_shift_test_head() {
	atf_set "descr" "Test fix for PR bin/50934 (null argv[0])"
}
pr50934_body() {
	atf_require_prog pgrep
	atf_require_prog cc

	cat > t.c <<'!'
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	sleep(2);
	argv[0] = 0;
	pause();
	exit(0);
}
!

	cc -o t0123456789abcdefg-beef t.c || atf_fail "t.c compile failed"

	./t0123456789abcdefg-beef &
	PID=$!

	pgrep -l t0123456 >OUT	; # should find the process
	pgrep -l beef >>OUT	; # should find the process

	sleep 5			; # allow time for sleep in t.c and argv[0]=0

	pgrep -l t0123456 >>OUT	; # should find the process
	pgrep -l beef >>OUT	; # should find nothing

	kill -9 $PID
	cat OUT	; # just for the log

	# note that pgrep -l only ever prints p_comm which is of limited sicze
	atf_check_equal "$(cat OUT)" \
	   "$PID t0123456789abcde $PID t0123456789abcde $PID t0123456789abcde"

	return 0
}

atf_init_test_cases() {
	atf_add_test_case pr50934
}
