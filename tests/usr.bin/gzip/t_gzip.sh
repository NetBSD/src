# $NetBSD: t_gzip.sh,v 1.5 2026/01/10 05:11:23 mrg Exp $
#
# Copyright (c) 2008 The NetBSD Foundation, Inc.
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

atf_test_case concatenated
concatenated_head()
{
	atf_set "descr" "Checks concatenated gzipped data"
}
concatenated_body()
{
	echo -n "aaaa" | gzip > tmp.gz
	echo -n "bbbb" | gzip >> tmp.gz

	atf_check -o inline:"aaaabbbb" gzip -d tmp.gz -c
}

atf_test_case pipe
pipe_head()
{
	atf_set "descr" "Checks input from pipe"
}
pipe_body()
{
	atf_check -x "dd if=/dev/zero count=102400 2>/dev/null \
| gzip -c | dd bs=1 2>/dev/null | gzip -tc"
}

atf_test_case truncated
truncated_head()
{
	atf_set "descr" "Checks that gzip fails on truncated data"
}
truncated_body()
{
	cat >truncated.gz.uue <<EOF
begin-base64 644 truncated.gz
H4sIAAAAAAAAA0tMSk7hAgCspIpYCg==
====
EOF
	uudecode -m truncated.gz.uue

	atf_check -s ne:0 -e ignore gzip -d truncated.gz
}

atf_test_case crcerror
crcerror_head()
{
	atf_set "descr" "Checks that gzip fails on crc error"
}
crcerror_body()
{
	cat >crcerror.gz.uue <<EOF
begin-base64 644 crcerror.gz
H4sIAAAAAAAAA0tMSk7hAgCspFhYBQAAAA==
====
EOF
	uudecode -m crcerror.gz.uue

	atf_check -s ne:0 -e ignore gzip -d crcerror.gz
}

atf_test_case good
good_head()
{
	atf_set "descr" "Checks decompressing correct file"
}
good_body()
{
	cat >good.gz.uue <<EOF
begin-base64 644 good.gz
H4sICC8G8UAAA2FiY2QAS0xKTuECAKykilgFAAAA
====
EOF
	uudecode -m good.gz.uue

	atf_check gzip -d good.gz
}

atf_test_case lzip
lzip_head()
{
	atf_set "descr" "Checks lzip compression levels (PR/58223)"
	atf_set "require.progs" "lzip"
}
lzip_body()
{
	n=net_tests.tar
	tar -C /usr/tests/net -cf $n .
	for i in $(jot 10 0 9); do
		f=$n.$i.lz
		lzip -$ic < $n > $f
		gunzip -t $f > /dev/null
	done
}

atf_test_case unbzip2
unbzip2_head()
{
	atf_set "descr" "Test bzip2 decompression"
}

unbzip2_body()
{
	cat >testbzip2.txt.bz2.uue <<EOF
begin-base64 664 testbzip2.txt.bz2
QlpoOTFBWSZTWRkTvvsAAARbgAAQQAAQAAQAP2bcECAAMUwmmgNMQiMg9QGmIGw9
kDXEgtnxXXYTzQ5wplvpTEfF3JFOFCQGRO++wA==
====
EOF
	uudecode -m testbzip2.txt.bz2.uue
	atf_check gzip -d testbzip2.txt.bz2
}

atf_test_case unlz
unlz_head()
{
	atf_set "descr" "Test lzip decompression"
}

unlz_body()
{
	cat >testlzip2.txt.lz.uue <<EOF
begin-base64 664 testlzip.txt.lz
TFpJUAEMACoaCSdkHIeKT8pM9PgdK8W6hMTdH5N/yGpYYx/txcwX2b2V8RX3n//0
/3gA5C0yJCQAAAAAAAAARwAAAAAAAAA=
====
EOF
	uudecode -m testlzip2.txt.lz.uue
	atf_check gzip -d testlzip.txt.lz
}

atf_test_case unpack
unpack_head()
{
	atf_set "descr" "Test pack decompression"
}

unpack_body()
{
	cat >testpack.txt.z.uue <<EOF
begin-base64 664 testpack.txt.z
Hx4AAAIlCQACAwAAAwQIBm5vIGdsCmVzaXBydCcuYWJjZGhtSVRma3V3eACFBhyG
HIMRIw4khGFhxAw44wSQ4MEAMgIMxgUUACQQcjAYSIBi+RxIUMIAoYcYhhJCcYeB
BJGgkgQOQcYNIhGFhxAw44YHYwKK+XIvlyL5ci+XIvlyL5ci+XIvlyFfLkXy5F8u
RfLkXy5F8uRfLkXy5CvlyL5ci+XIvlyL5ci+XIvlyL5chXy5F8uRfLkXy5F8uRfL
kXy5F8uQr5ci+XIvlyL5ci+XIvlyL5ci+XIV8uRfLkXy5F8uRfLkXy5F8uRfLkK+
XIvlyL5ci+XIvlyL5ci+XIvlyFA4
====
EOF
	uudecode -m testpack.txt.z.uue
	atf_check gzip -d testpack.txt.z
}

atf_test_case unxz
unxz_head()
{
	atf_set "descr" "Test xz decompression"
}

unxz_body()
{
	cat >testxz.txt.xz.uue <<EOF
====
begin-base64 664 testxz.txt.xz
/Td6WFoAAATm1rRGAgAhARYAAAB0L+WjAQAhVGhpcyBpcyBhIHRlc3QgY29tcHJl
c3NlZCB4eiBmaWxlCgAAAPXae+PtU7eLAAE6IrYqT9AftvN9AQAAAAAEWVo=
====
EOF
	uudecode -m testxz.txt.xz.uue
	atf_check gzip -d testxz.txt.xz
}

atf_test_case ungzip
ungzip_head()
{
	atf_set "descr" "Test gzip decompression"
}

ungzip_body()
{
	cat >testgzip.txt.gz.uue <<EOF
begin-base64 664 testgzip.txt.gz
H4sICInRYWkAA3Rlc3QtZ3ppcC50eHQAC8nILFYAokSFktTiEoXk/NyCotTi4tQU
hfSqzAKFtMycVC4AE9bS3CQAAAA=
====
EOF
	uudecode -m testgzip.txt.gz.uue
	atf_check gzip -d testgzip.txt.gz
}

atf_init_test_cases()
{
	atf_add_test_case concatenated
	atf_add_test_case pipe
	atf_add_test_case truncated
	atf_add_test_case crcerror
	atf_add_test_case good
	atf_add_test_case lzip
	atf_add_test_case unbzip2
	atf_add_test_case unlz
	atf_add_test_case unpack
	atf_add_test_case unxz
	atf_add_test_case ungzip
}
