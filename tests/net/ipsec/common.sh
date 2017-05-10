#	$NetBSD: common.sh,v 1.2 2017/05/10 04:46:13 ozaki-r Exp $
#
# Copyright (c) 2017 Internet Initiative Japan Inc.
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

test_flush_entries()
{
	local sock=$1

	export RUMP_SERVER=$sock

	atf_check -s exit:0 -o empty $HIJACKING setkey -F
	atf_check -s exit:0 -o empty $HIJACKING setkey -F -P
	atf_check -s exit:0 -o match:"No SAD entries." $HIJACKING setkey -D -a
	atf_check -s exit:0 -o match:"No SPD entries." $HIJACKING setkey -D -P
}

check_sa_entries()
{
	local sock=$1
	local local_addr=$2
	local remote_addr=$3

	export RUMP_SERVER=$sock

	$DEBUG && $HIJACKING setkey -D

	atf_check -s exit:0 -o match:"$local_addr $rmote_addr" \
	    $HIJACKING setkey -D
	atf_check -s exit:0 -o match:"$remote_addr $local_addr" \
	    $HIJACKING setkey -D
	# TODO: more detail checks
}
