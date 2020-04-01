# $NetBSD: mpls_common.sh,v 1.1 2020/04/01 01:49:26 christos Exp $
#
# Copyright (c) 2020 The NetBSD Foundation, Inc.
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

export PATH=/sbin:/usr/sbin:/bin:/usr/bin

RUMP_SERVER1=unix://./r1
RUMP_SERVER2=unix://./r2
RUMP_SERVER3=unix://./r3
RUMP_SERVER4=unix://./r4

RUMP_FLAGS6="-lrumpnet -lrumpnet_net -lrumpnet_netinet -lrumpnet_netinet6 \
             -lrumpnet_shmif -lrumpnet_netmpls"

dostart()
{

	ulimit -r 400
	atf_check -s exit:0 rump_server ${RUMP_FLAGS6} ${RUMP_SERVER1}
	atf_check -s exit:0 rump_server ${RUMP_FLAGS6} ${RUMP_SERVER2}
	atf_check -s exit:0 rump_server ${RUMP_FLAGS6} ${RUMP_SERVER3}
	atf_check -s exit:0 rump_server ${RUMP_FLAGS6} ${RUMP_SERVER4}
}

docleanup()
{

	RUMP_SERVER=${RUMP_SERVER1} rump.halt
	RUMP_SERVER=${RUMP_SERVER2} rump.halt
	RUMP_SERVER=${RUMP_SERVER3} rump.halt
	RUMP_SERVER=${RUMP_SERVER4} rump.halt
}
