#       $NetBSD: t_tcpip.sh,v 1.2 2011/02/14 15:14:00 pooka Exp $
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
	atf_check -s exit:0 env LD_PRELOAD=/usr/lib/librumphijack.so \
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

#
# Starts a SSH server and sets up the client to access it.
# Authentication is allowed and done using an RSA key exclusively, which
# is generated on the fly as part of the test case.
# XXX: Ideally, all the tests in this test program should be able to share
# the generated key, because creating it can be a very slow process on some
# machines.
#
# XXX2: copypasted from jmmv's sshd thingamob in the psshfs test.
# ideally code (and keys, like jmmv notes above) could be shared
#
start_sshd() {
	echo "Setting up SSH server configuration"
	sed -e "s,@SRCDIR@,$(atf_get_srcdir),g" -e "s,@WORKDIR@,$(pwd),g" \
	    $(atf_get_srcdir)/sshd_config.in >sshd_config || \
	    atf_fail "Failed to create sshd_config"
	atf_check -s ignore -o empty -e ignore \
	    cp $(atf_get_srcdir)/ssh_host_key .
	atf_check -s ignore -o empty -e ignore \
	    cp $(atf_get_srcdir)/ssh_host_key.pub .
	atf_check -s eq:0 -o empty -e empty chmod 400 ssh_host_key
	atf_check -s eq:0 -o empty -e empty chmod 444 ssh_host_key.pub

        env LD_PRELOAD=/usr/lib/librumphijack.so \
	    /usr/sbin/sshd -e -f ./sshd_config
	while [ ! -f sshd.pid ]; do
		sleep 0.01
	done
	echo "SSH server started (pid $(cat sshd.pid))"

	echo "Setting up SSH client configuration"
	atf_check -s eq:0 -o empty -e empty \
	    ssh-keygen -f ssh_user_key -t rsa -b 1024 -N "" -q
	atf_check -s eq:0 -o empty -e empty \
	    cp ssh_user_key.pub authorized_keys
	echo "127.0.0.1,localhost,::1 " \
	    "$(cat $(atf_get_srcdir)/ssh_host_key.pub)" >known_hosts || \
	    atf_fail "Failed to create known_hosts"
	atf_check -s eq:0 -o empty -e empty chmod 600 authorized_keys
	sed -e "s,@SRCDIR@,$(atf_get_srcdir),g" -e "s,@WORKDIR@,$(pwd),g" \
	    $(atf_get_srcdir)/ssh_config.in >ssh_config || \
	    atf_fail "Failed to create ssh_config"
	
	echo "sshd running"
}

atf_test_case ssh cleanup
ssh_head()
{
        atf_set "descr" "Test that hijacked ssh/sshd works"
}

ssh_body()
{

	atf_check -s exit:0 ${rumpnetsrv} ${RUMP_SERVER}
	# make sure clients die after we nuke the server
	export RUMPHIJACK_RETRY='die'

	export LD_LIBRARY_PATH=/home/pooka/src/nb5/src/lib/libssh

	start_sshd

	# create some sort of directory for us to "ls"
	mkdir testdir
	cd testdir
	jot 11 | xargs touch
	jot 11 12 | xargs mkdir
	cd ..

	atf_check -s exit:0 -o save:ssh.out				\
	    env LD_PRELOAD=/usr/lib/librumphijack.so			\
	    ssh -T -F ssh_config 127.0.0.1 ls -li $(pwd)/testdir
	atf_check -s exit:0 -o file:ssh.out ls -li $(pwd)/testdir
}

ssh_cleanup()
{
	rump.halt
	# sshd dies due to RUMPHIJACK_RETRY=1d6
}

atf_init_test_cases()
{
	atf_add_test_case http
	atf_add_test_case ssh
}
