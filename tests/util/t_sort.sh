# $NetBSD: t_sort.sh,v 1.2 2010/06/12 13:15:01 pooka Exp $
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

# the implementation of "sort" to test
: ${TEST_SORT:="sort"}


atf_test_case basic
basic_head()
{
	atf_set "descr" "basic functionality test"
	atf_set "use.fs" "true"
}

basic_body()
{
	echo 'z b m f' > in
	echo 'y c o e' >> in
	echo 'x a n h' >> in
	echo 'x a n g' >> in

	echo "----- test: sort -----"
	$TEST_SORT in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "xangxanhycoezbmf" ]; then
	    atf_fail "output is wrong"
	fi

	echo "----- test: sort -r -----"
	$TEST_SORT -r in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "zbmfycoexanhxang" ]; then
	    atf_fail "output is wrong"
	fi

	echo "----- test: sort -k2.1 -----"
	$TEST_SORT -k2.1 in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "xangxanhzbmfycoe" ]; then
	    atf_fail "output is wrong"
	fi

	echo "----- test: sort -k2.1,2.0 -----"
	$TEST_SORT -k2.1,2.0 in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "xanhxangzbmfycoe" ]; then
	    atf_fail "output is wrong"
	fi

	rm -f in

	echo '1' > in
	echo '123' >> in
	echo '2' >> in

	echo "----- test: sort -n -----"
	$TEST_SORT -n in > out || atf_fail "program failed"
	if [ "$(tr < out '\n' ' ')" != "1 2 123 " ]; then
	    atf_fail "output is wrong"
	fi

	echo "----- test: sort -rn -----"
	$TEST_SORT -rn in > out || atf_fail "program failed"
	if [ "$(tr < out '\n' ' ')" != "123 2 1 " ]; then
	    atf_fail "output is wrong"
	fi

	rm -f in

	echo 'a' > in
	echo 'aa' >> in
	echo 'aaa' >> in
	echo 'aa' >> in

	echo "----- test: sort -u -----"
	$TEST_SORT -u in > out || atf_fail "program failed"
	if [ "$(tr < out '\n' ' ')" != "a aa aaa " ]; then
	    atf_fail "output is wrong"
	fi

	echo "----- test: sort -ru -----"
	$TEST_SORT -ru in > out || atf_fail "program failed"
	if [ "$(tr < out '\n' ' ')" != "aaa aa a " ]; then
	    atf_fail "output is wrong"
	fi

	rm -f in
}

atf_test_case plusopts
plusopts_head()
{
	atf_set "descr" "Checks translations of +n [-n] options"
	atf_set "use.fs" "true"
}
plusopts_body()
{
	# +1 should become -k2.1
	# +1 -2 should become -k2.1,2.0
	echo 'z b m f' > in
	echo 'y c o e' >> in
	echo 'x a n h' >> in
	echo 'x a n g' >> in
	echo "----- test: sort +1 in -----"
	$TEST_SORT +1 in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "xangxanhzbmfycoe" ]; then
	    atf_fail "output is wrong"
	fi
	echo "----- test: sort +1 -2 in -----"
	$TEST_SORT +1 -2 in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' \n')" != "xanhxangzbmfycoe" ]; then
	    atf_fail "output is wrong"
	fi
	rm -f in

	# old wrong behavior: sort: -k1.1: No such file or directory
	echo "----- test: sort -- +0 -----"
	echo 'good contents' > ./+0
	$TEST_SORT -- +0 > out || atf_fail "program failed"
	diff -Nru -- ./+0 out || atf_fail "output is wrong"
	rm -f ./+0

	# old wrong behavior: sort: -k1.1: No such file or directory
	echo "----- test: sort in +0 -----"
	echo 'good contents' > ./+0
	echo 'more contents' > in
	cat ./+0 in > good
	$TEST_SORT in +0 > out || atf_fail "program failed"
	diff -Nru -- good out || atf_fail "output is wrong"
	rm -f ./+0 in good

	# old behavior: success
	# intermediate wrong behavior: sort: +0: No such file or directory
	echo "----- test: sort -T /tmp +0 in -----"
	echo 'good contents' > in
	$TEST_SORT -T /tmp +0 in > out || atf_fail "program failed"
	diff -Nru -- in out || atf_fail "output is wrong"
	rm -f in

	# old behavior: sort: invalid record delimiter -k1.1
	echo "----- test: sort -R + in -----"
	(
	    echo 'z b m f'
	    echo 'y c o e'
	    echo 'x a n h'
	    echo 'x a n g'
	) | tr '\n' '+' > in
	$TEST_SORT -R + -k2 in > out || atf_fail "program failed"
	if [ "$(tr < out -d ' +')" != "xangxanhzbmfycoe" ]; then
	    atf_fail "output is wrong"
	fi
	rm -f in

	# old behavior: sort: ftmp: mkstemp("-k1.1"): No such file or directory
	echo "----- test: sort -T + $WORDS -----"
	mkdir ./+
	yes | sed 200000q | $TEST_SORT -T + > /dev/null \
	    || atf_fail "program failed"
	rm -rf ./+
}

atf_init_test_cases()
{
	atf_add_test_case basic
	atf_add_test_case plusopts
}
