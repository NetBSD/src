#	$NetBSD: t_ipsec_ah_keys.sh,v 1.4 2023/06/19 08:28:09 knakahara Exp $
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

SOCK_LOCAL=unix://ipsec_local

DEBUG=${DEBUG:-false}

test_ah_valid_keys_common()
{
	local aalgo=$1
	local key=
	local tmpfile=./tmp
	local len=

	rump_server_crypto_start $SOCK_LOCAL netipsec

	export RUMP_SERVER=$SOCK_LOCAL

	for len in $(get_valid_keylengths $aalgo); do
		key=$(generate_key $len)
		cat > $tmpfile <<-EOF
		add 10.0.0.1 10.0.0.2 ah 10000 -A $aalgo $key;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		atf_check -s exit:0 -o match:'10.0.0.1 10.0.0.2' \
		    $HIJACKING setkey -D
		# TODO: more detail checks

		cat > $tmpfile <<-EOF
		delete 10.0.0.1 10.0.0.2 ah 10000;
		EOF
		$DEBUG && cat $tmpfile
		atf_check -s exit:0 -o empty $HIJACKING setkey -c < $tmpfile
		atf_check -s exit:0 -o match:'No SAD entries.' \
		    $HIJACKING setkey -D
	done

	rm -f $tmpfile
}

add_test_valid_keys()
{
	local aalgo=$1
	local _aalgo=$(echo $aalgo | sed 's/-//g')
	local name= desc=

	name="ipsec_ah_${_aalgo}_valid_keys"
	desc="Tests AH ($aalgo) valid keys"

	atf_test_case ${name} cleanup
	eval "
	    ${name}_head() {
	        atf_set descr \"$desc\"
	        atf_set require.progs rump_server setkey
	    }
	    ${name}_body() {
	        test_ah_valid_keys_common $aalgo
	    }
	    ${name}_cleanup() {
	        \$DEBUG && dump
	        cleanup
	    }
	"
	atf_add_test_case ${name}
}

test_ah_invalid_keys_common()
{
	local aalgo=$1
	local key=
	local tmpfile=./tmp
	local len=

	rump_server_crypto_start $SOCK_LOCAL netipsec

	export RUMP_SERVER=$SOCK_LOCAL

	for len in $(get_invalid_keylengths $aalgo); do
		key=$(generate_key $len)
		cat > $tmpfile <<-EOF
		add 10.0.0.1 10.0.0.2 ah 10000 -A $aalgo $key;
		EOF
		$DEBUG && cat $tmpfile
		if [ $aalgo = null ]; then
			# null doesn't accept any keys
			atf_check -s exit:0 \
			    -o match:'syntax error' -e ignore \
			    $HIJACKING setkey -c < $tmpfile
		else
			atf_check -s exit:0 \
			    -o match:'Invalid (key length|argument)' -e ignore \
			    $HIJACKING setkey -c < $tmpfile
		fi
		atf_check -s exit:0 -o match:'No SAD entries.' \
		    $HIJACKING setkey -D
	done

	rm -f $tmpfile
}

add_test_invalid_keys()
{
	local aalgo=$1
	local _aalgo=$(echo $aalgo | sed 's/-//g')
	local name= desc=

	name="ipsec_ah_${_aalgo}_invalid_keys"
	desc="Tests AH ($aalgo) invalid keys"

	atf_test_case ${name} cleanup
	eval "								\
	    ${name}_head() {						\
	        atf_set \"descr\" \"$desc\";				\
	        atf_set \"require.progs\" \"rump_server\" \"setkey\";	\
	    };								\
	    ${name}_body() {						\
	        test_ah_invalid_keys_common $aalgo;			\
	    };								\
	    ${name}_cleanup() {						\
	        $DEBUG && dump;						\
	        cleanup;						\
	    }								\
	"
	atf_add_test_case ${name}
}

atf_init_test_cases()
{

	for aalgo in $AH_AUTHENTICATION_ALGORITHMS; do
		add_test_valid_keys $aalgo
		add_test_invalid_keys $aalgo
	done
}
