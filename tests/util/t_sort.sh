# $NetBSD: t_sort.sh,v 1.4 2010/07/18 22:58:14 jmmv Exp $
#
# Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
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

atf_test_case basic
basic_head()
{
	atf_set "descr" "Basic functionality test"
	atf_set "use.fs" "true"
}

basic_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
x a n g
x a n h
y c o e
z b m f
EOF

	atf_check -s exit:0 -o file:expout sort in
}

atf_test_case rflag
rflag_head()
{
	atf_set "descr" "Tests the -r flag"
	atf_set "use.fs" "true"
}

rflag_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	atf_check -s exit:0 -o file:expout sort -r in
}

atf_test_case kflag_one_field
kflag_one_field_head()
{
	atf_set "descr" "Tests the -k flag with one field"
	atf_set "use.fs" "true"
}

kflag_one_field_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
x a n g
x a n h
z b m f
y c o e
EOF

	atf_check -s exit:0 -o file:expout sort -k2.1 in
}

atf_test_case kflag_two_fields
kflag_two_fields_head()
{
	atf_set "descr" "Tests the -k flag with two fields"
	atf_set "use.fs" "true"
}

kflag_two_fields_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
x a n h
x a n g
z b m f
y c o e
EOF
	atf_check -s exit:0 -o file:expout sort -k2.1,2.0 in
}

atf_test_case nflag
nflag_head()
{
	atf_set "descr" "Tests the -n flag"
	atf_set "use.fs" "true"
}

nflag_body()
{
	cat >in <<EOF
1
123
2
EOF

	cat >expout <<EOF
1
2
123
EOF

	atf_check -s exit:0 -o file:expout sort -n in
}

atf_test_case nflag_rflag
nflag_rflag_head()
{
	atf_set "descr" "Tests the -n and -r flag combination"
	atf_set "use.fs" "true"
}

nflag_rflag_body()
{
	cat >in <<EOF
1
123
2
EOF

	cat >expout <<EOF
123
2
1
EOF

	atf_check -s exit:0 -o file:expout sort -rn in
}

atf_test_case uflag
uflag_head()
{
	atf_set "descr" "Tests the -u flag"
	atf_set "use.fs" "true"
}

uflag_body()
{
	cat >in <<EOF
a
aa
aaa
aa
EOF

	cat >expout <<EOF
a
aa
aaa
EOF

	atf_check -s exit:0 -o file:expout sort -u in
}

atf_test_case uflag_rflag
uflag_rflag_head()
{
	atf_set "descr" "Tests the -u and -r flag combination"
	atf_set "use.fs" "true"
}

uflag_rflag_body()
{
	cat >in <<EOF
a
aa
aaa
aa
EOF

	cat >expout <<EOF
aaa
aa
a
EOF

	atf_check -s exit:0 -o file:expout sort -ru in
}

atf_test_case plus_one
plus_one_head()
{
	atf_set "descr" "Tests +- addressing: +1 should become -k2.1"
	atf_set "use.fs" "true"
}
plus_one_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
x a n g
x a n h
z b m f
y c o e
EOF

	atf_check -s exit:0 -o file:expout sort +1 in
}

atf_test_case plus_one_minus_two
plus_one_minus_two_head()
{
	atf_set "descr" "Tests +- addressing: +1 -2 should become -k2.1,2.0"
	atf_set "use.fs" "true"
}
plus_one_minus_two_body()
{
	cat >in <<EOF
z b m f
y c o e
x a n h
x a n g
EOF

	cat >expout <<EOF
x a n h
x a n g
z b m f
y c o e
EOF

	atf_check -s exit:0 -o file:expout sort +1 -2 in
}

atf_test_case plus_zero
plus_zero_head()
{
	atf_set "descr" "Tests +- addressing: '-- +0' raised a '-k1.1: No" \
	    "such file or directory' error"
	atf_set "use.fs" "true"
}
plus_zero_body()
{
	echo 'good contents' >./+0

	atf_check -s exit:0 -o file:+0 sort -- +0
}

atf_test_case plus_as_path
plus_as_path_head()
{
	atf_set "descr" "Tests +- addressing: 'file +0' raised a '-k1.1: No" \
	    "such file or directory' error"
	atf_set "use.fs" "true"
}
plus_as_path_body()
{
	echo 'good contents' >./+0
	echo 'more contents' >in
	cat ./+0 in >expout

	atf_check -s exit:0 -o file:expout sort in +0
}

atf_test_case plus_bad_tempfile
plus_bad_tempfile_head()
{
	atf_set "descr" "Tests +- addressing: intermediate wrong behavior" \
	    "that raised a '+0: No such file or directory' error"
	atf_set "use.fs" "true"
}
plus_bad_tempfile_body()
{
	echo 'good contents' >in
	atf_check -s exit:0 -o file:in sort -T /tmp +0 in
}

atf_test_case plus_rflag_invalid
plus_rflag_invalid_head()
{
	atf_set "descr" "Tests +- addressing: invalid record delimiter"
	atf_set "use.fs" "true"
}
plus_rflag_invalid_body()
{
	(
	    echo 'z b m f'
	    echo 'y c o e'
	    echo 'x a n h'
	    echo 'x a n g'
	) | tr '\n' '+' >in

	atf_check -s exit:0 -o inline:'x a n g+x a n h+z b m f+y c o e+' \
	    sort -R + -k2 in
}

atf_test_case plus_tflag
plus_tflag()
{
	atf_set "descr" "Tests +- addressing: using -T caused a 'No such file" \
	    "or directory' error"
	atf_set "use.fs" "true"
}
plus_tflag()
{
	mkdir ./+
	yes | sed 200000q | sort -T + >/dev/null || atf_fail "program failed"
}

atf_init_test_cases()
{
	atf_add_test_case basic
	atf_add_test_case rflag
	atf_add_test_case kflag_one_field
	atf_add_test_case kflag_two_fields
	atf_add_test_case nflag
	atf_add_test_case nflag_rflag
	atf_add_test_case uflag
	atf_add_test_case uflag_rflag
	atf_add_test_case plus_one
	atf_add_test_case plus_one_minus_two
	atf_add_test_case plus_zero
	atf_add_test_case plus_as_path
	atf_add_test_case plus_bad_tempfile
	atf_add_test_case plus_rflag_invalid
	atf_add_test_case plus_tflag
}
