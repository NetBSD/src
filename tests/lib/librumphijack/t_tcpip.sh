#       $NetBSD: t_tcpip.sh,v 1.1 2011/02/06 18:44:30 pooka Exp $
#
# Copyright (c) 2011 The NetBSD Foundation, Inc.
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

rumpnetsrv='rump_server -lrumpnet -lrumpnet_net -lrumpnet_netinet'
export RUMP_SERVER=unix://csock

atf_test_case http cleanup
http_head()
{
        atf_set "descr" "Start hijacked httpd and get webpage from it"
}

http_body()
{

	atf_check -s exit:0 ${rumpnetsrv} ${RUMP_SERVER}
	# make sure clients die after we nuke the server
	export RUMPHIJACK_RETRY='die'

	# start bozo in daemon mode
	atf_check -s exit:0 -e ignore env LD_PRELOAD=/usr/lib/librumphijack.so \
	    /usr/libexec/httpd -b -s $(atf_get_srcdir)

	atf_check -s exit:0 -o file:"$(atf_get_srcdir)/netstat.expout" \
	    rump.netstat -a

	# get the webpage
	atf_check -s exit:0 env LD_PRELOAD=/usr/lib/librumphijack.so 	\
	    $(atf_get_srcdir)/h_netget 127.0.0.1 80 webfile

	# check that we got what we wanted
	atf_check -o match:'HTTP/1.0 200 OK' cat webfile
	atf_check -o match:'Content-Length: 95' cat webfile
	atf_check -o file:"$(atf_get_srcdir)/index.html" \
	    sed -n '1,/^$/!p' webfile
}

http_cleanup()
{
	rump.halt
}

atf_init_test_cases()
{
	atf_add_test_case http
}
