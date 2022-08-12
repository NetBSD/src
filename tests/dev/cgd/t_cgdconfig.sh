#	$NetBSD: t_cgdconfig.sh,v 1.3 2022/08/12 10:49:17 riastradh Exp $
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

COUNTKEY=$(atf_get_srcdir)/h_countkey

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

atf_test_case sharedstoredkey10
sharedstoredkey10_head()
{
	atf_set descr "Test shared key generation from storedkey, 10-byte info"
}
sharedstoredkey10_body()
{
	cat <<EOF >params
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey {
        key AAABAAd3CTYsLjLfDdw/DcR7umOQtsc7tQ+cMSLshErXwrPl;
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAAAUPDx8vP09fb3+Pk=;
};
EOF
	atf_check -o inline:'PLJfJfqs1XqQQ09k0DYvKi0tCpDPGlpMXbAtVuzExb8=\n' \
	    cgdconfig -t params
}

atf_test_case sharedstoredkey80
sharedstoredkey80_head()
{
	atf_set descr "Test shared key generation from storedkey, 80-byte info"
}
sharedstoredkey80_body()
{
	cat <<EOF >params
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey {
        key AAABAAamuIxYUzYaBhBMnOs1tFzvdgAUkEZxAUoZP0DBX8JE;
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAACgLCxsrO0tba3uLm6u7y9vr/AwcLDxMXGx8jJ \
                   ysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn \
                   6Onq6+zt7u/w8fLz9PX29/j5+vv8/f7/;
};
EOF
	atf_check -o inline:'sR45jcgDJ6HI5/eMWWpJNE8BLtotTvrYoFDMTBmvqXw=\n' \
	    cgdconfig -t params
}

atf_test_case sharedstoredkeys
sharedstoredkeys_head()
{
	atf_set descr "Test multiple shared key generations from stored keys"
}
sharedstoredkeys_body()
{
	cat <<EOF >wd0e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey {
        key AAABAAd3CTYsLjLfDdw/DcR7umOQtsc7tQ+cMSLshErXwrPl;
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAAAUPDx8vP09fb3+Pk=;
};
EOF
	cat <<EOF >ld1e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen storedkey {
        key AAABAAd3CTYsLjLfDdw/DcR7umOQtsc7tQ+cMSLshErXwrPl;
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAAAQMxUtCBh7ha6mUU=;
};
EOF
	cat <<EOF >cgd.conf0
cgd0	/dev/wd0e	wd0e
cgd1	/dev/ld1e	ld1e
EOF
	cat <<EOF >expected0
/dev/wd0e: PLJfJfqs1XqQQ09k0DYvKi0tCpDPGlpMXbAtVuzExb8=
/dev/ld1e: ADxn574yb7sVdxHphNRRdObZxntMJA/ssMuUX6SXgEY=
EOF
	cat <<EOF >cgd.conf1
cgd0	/dev/ld1e	ld1e
cgd1	/dev/wd0e	wd0e
EOF
	cat <<EOF >expected1
/dev/ld1e: ADxn574yb7sVdxHphNRRdObZxntMJA/ssMuUX6SXgEY=
/dev/wd0e: PLJfJfqs1XqQQ09k0DYvKi0tCpDPGlpMXbAtVuzExb8=
EOF
	atf_check -o file:expected0 cgdconfig -T -f cgd.conf0
	atf_check -o file:expected1 cgdconfig -T -f cgd.conf1
}

atf_test_case sharedshellkeys
sharedshellkeys_head()
{
	atf_set descr "Test multiple shared key generations from shell_cmd"
}
sharedshellkeys_body()
{
	cat <<EOF >wd0e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen shell_cmd {
        cmd "${COUNTKEY} n B3cJNiwuMt8N3D8NxHu6Y5C2xzu1D5wxIuyEStfCs+U=";
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAAAUPDx8vP09fb3+Pk=;
};
EOF
	cat <<EOF >ld1e
algorithm adiantum;
iv-method encblkno1;
keylength 256;
verify_method none;
keygen shell_cmd {
        cmd "${COUNTKEY} n B3cJNiwuMt8N3D8NxHu6Y5C2xzu1D5wxIuyEStfCs+U=";
        shared "helloworld" algorithm hkdf-hmac-sha256 \
            subkey AAAAQMxUtCBh7ha6mUU=;
};
EOF
	cat <<EOF >cgd.conf0
cgd0	/dev/wd0e	wd0e
cgd1	/dev/ld1e	ld1e
EOF
	cat <<EOF >expected0
/dev/wd0e: PLJfJfqs1XqQQ09k0DYvKi0tCpDPGlpMXbAtVuzExb8=
/dev/ld1e: ADxn574yb7sVdxHphNRRdObZxntMJA/ssMuUX6SXgEY=
EOF
	cat <<EOF >cgd.conf1
cgd0	/dev/ld1e	ld1e
cgd1	/dev/wd0e	wd0e
EOF
	cat <<EOF >expected1
/dev/ld1e: ADxn574yb7sVdxHphNRRdObZxntMJA/ssMuUX6SXgEY=
/dev/wd0e: PLJfJfqs1XqQQ09k0DYvKi0tCpDPGlpMXbAtVuzExb8=
EOF
	atf_check -o file:expected0 cgdconfig -T -f cgd.conf0
	atf_check -o inline:'1\n' cat n
	atf_check -o file:expected1 cgdconfig -T -f cgd.conf1
	atf_check -o inline:'2\n' cat n
}

atf_init_test_cases()
{
	atf_add_test_case sharedshellkeys
	atf_add_test_case sharedstoredkey10
	atf_add_test_case sharedstoredkey80
	atf_add_test_case sharedstoredkeys
	atf_add_test_case storedkey
	atf_add_test_case storedkey2a
	atf_add_test_case storedkey2b
	atf_add_test_case storedkeys
}
