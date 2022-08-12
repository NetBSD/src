#	$NetBSD: t_cgdconfig.sh,v 1.2 2022/08/12 10:48:44 riastradh Exp $
#
# Copyright (c) 2022 The NetBSD Foundation, Inc.
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

atf_test_case storedkey
storedkey_head()
{
	atf_set descr "Test key generation with storedkey"
}
storedkey_body()
{
	cat <<EOF >params
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey key AAABAJtnmp3XZspMBAFpCYnB8Hekn0 \
                     gj5cDVngslfGLSqwcy;
EOF
	atf_check -o inline:'m2eanddmykwEAWkJicHwd6SfSCPlwNWeCyV8YtKrBzI=\n' \
	    cgdconfig -t params
}

atf_test_case storedkeys
storedkeys_head()
{
	atf_set descr "Test multiple stored keys with cgd.conf"
}
storedkeys_body()
{
	cat <<EOF >wd0e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey key AAABAJtnmp3XZspMBAFpCYnB8Hekn0 \
                     gj5cDVngslfGLSqwcy;
EOF
	cat <<EOF >ld1e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey key AAABAK1pbgIayXftX0RQ3AaMK4YEd/ \
                     fowKwQbENxpu3o1k9m;
EOF
	cat <<EOF >cgd.conf
cgd0	/dev/wd0e	wd0e
cgd1	/dev/ld1e	ld1e
EOF
	cat <<EOF >expected
/dev/wd0e: m2eanddmykwEAWkJicHwd6SfSCPlwNWeCyV8YtKrBzI=
/dev/ld1e: rWluAhrJd+1fRFDcBowrhgR39+jArBBsQ3Gm7ejWT2Y=
EOF
	atf_check -o file:expected cgdconfig -T -f cgd.conf
}

atf_test_case storedkey2a
storedkey2a_head()
{
	atf_set descr "Test key generation with combined storedkeys"
}
storedkey2a_body()
{
	cat <<EOF >params
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey key AAABAJtnmp3XZspMBAFpCYnB8Hekn0 \
                     gj5cDVngslfGLSqwcy;
keygen storedkey key AAABAK1pbgIayXftX0RQ3AaMK4YEd/ \
                     fowKwQbENxpu3o1k9m;
EOF
	atf_check -o inline:'Ng70n82vvaFbRTnVj03b8aDov8slbMXySFTajzp9SFQ=\n' \
	    cgdconfig -t params
}

atf_test_case storedkey2b
storedkey2b_head()
{
	atf_set descr "Test key generation with combined storedkeys, reversed"
}
storedkey2b_body()
{
	cat <<EOF >params
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey key AAABAK1pbgIayXftX0RQ3AaMK4YEd/ \
                     fowKwQbENxpu3o1k9m;
keygen storedkey key AAABAJtnmp3XZspMBAFpCYnB8Hekn0 \
                     gj5cDVngslfGLSqwcy;
EOF
	atf_check -o inline:'Ng70n82vvaFbRTnVj03b8aDov8slbMXySFTajzp9SFQ=\n' \
	    cgdconfig -t params
}

atf_init_test_cases()
{
	atf_add_test_case storedkey
	atf_add_test_case storedkey2a
	atf_add_test_case storedkey2b
	atf_add_test_case storedkeys
}
