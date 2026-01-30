#! /bin/sh

# $NetBSD: t_netpgpverify.sh,v 1.7 2026/01/30 09:01:08 wiz Exp $

#
# Copyright (c) 2016 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Alistair Crooks (agc@NetBSD.org)
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
# AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
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

# Define test sets with atf_test_case
# There may well be only one test set - these are tests for different
# functionality, so netpgpverify has 2 sets, 1 for RSA and 1 for DSA
# each test set has a
# + *_head() function, to define the set, and
# + *_body(), to set up the supporting input and expected output files
#	and some atf_check calls, which actually carry out the tests

# any binary files should be uuencoded
# we need to give the input and expected output files like this as tests
# take place in a clean directory, so we need to be able to set them up
# from the shell script

# Test set 1 (rsa_signatures) for netpgpverify
atf_test_case netpgpverify_testset_1_rsa_signatures

netpgpverify_testset_1_rsa_signatures_head() {
	atf_set "descr" "Test set 1 (rsa_signatures) for netpgpverify"
}
netpgpverify_testset_1_rsa_signatures_body() {
	data=$(atf_get_srcdir)/data
	gzcat ${data}/NetBSD-6.0_hashes.asc.gz > NetBSD-6.0_hashes.asc
	# expected output contains full file names, so we need the input files locally
	for i in pubring.gpg b.gpg a.gpg jj.asc det det.sig
	do
		cp ${data}/${i} .
	done

	atf_check -s exit:0 -o file:${data}/expected16 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c verify b.gpg
	atf_check -s exit:0 -o file:${data}/expected18 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c verify a.gpg
#	atf_check -s exit:0 -o file:${data}/expected19 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c verify NetBSD-6.0_hashes.asc
	atf_check -s exit:0 -o file:${data}/expected20 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c cat jj.asc
	atf_check -s exit:0 -o file:${data}/expected21 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg < a.gpg
	atf_check -s exit:0 -o file:${data}/expected22 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg < jj.asc
#	atf_check -s exit:0 -o file:${data}/expected23 -e empty env TZ=US/Pacific netpgpverify < NetBSD-6.0_hashes.asc
	atf_check -s exit:0 -o file:${data}/expected24 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg < b.gpg
	#atf_check -s exit:0 -o file:${data}/expected25 -e empty netpgpverify NetBSD-6.0_hashes.gpg
	#atf_check -s exit:0 -o file:${data}/expected26 -e empty netpgpverify < NetBSD-6.0_hashes.gpg
	atf_check -s exit:0 -o file:${data}/expected27 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg < NetBSD-6.0_hashes.asc
	atf_check -s exit:0 -o file:${data}/expected28 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg NetBSD-6.0_hashes.asc
	#atf_check -s exit:0 -o file:${data}/expected29 -e empty netpgpverify NetBSD-6.0_hashes_ascii.gpg
	#atf_check -s exit:0 -o file:${data}/expected30 -e empty netpgpverify < NetBSD-6.0_hashes_ascii.gpg
	atf_check -s exit:0 -o file:${data}/expected31 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c cat b.gpg b.gpg b.gpg
	atf_check -s exit:0 -o file:${data}/expected32 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg b.gpg b.gpg b.gpg
	atf_check -s exit:0 -o file:${data}/expected33 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c cat b.gpg jj.asc b.gpg
	atf_check -s exit:0 -o file:${data}/expected34 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg det.sig
	atf_check -s exit:0 -o file:${data}/expected35 -e empty env TZ=US/Pacific netpgpverify -k pubring.gpg -c cat det.sig
	#atf_check -s exit:0 -o file:${data}/expected46 -e empty netpgpverify -k problem-pubring.gpg NetBSD-6.0_hashes.asc
}

# Test set 2 (dsa_signatures) for netpgpverify
atf_test_case netpgpverify_testset_2_dsa_signatures

netpgpverify_testset_2_dsa_signatures_head() {
	atf_set "descr" "Test set 2 (dsa_signatures) for netpgpverify"
}
netpgpverify_testset_2_dsa_signatures_body() {
	data=$(atf_get_srcdir)/data
	for i in dsa-pubring.gpg in1.gpg in1.asc in2.gpg in2.asc
	do
		cp ${data}/${i} .
	done

	atf_check -s exit:0 -o file:${data}/expected36 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg in1.gpg
	atf_check -s exit:0 -o file:${data}/expected37 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg < in1.gpg
	atf_check -s exit:0 -o file:${data}/expected38 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg in1.asc
	atf_check -s exit:0 -o file:${data}/expected39 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg < in1.asc
	atf_check -s exit:0 -o file:${data}/expected40 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg -c cat in1.gpg
	atf_check -s exit:0 -o file:${data}/expected41 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg -c cat < in1.gpg
	atf_check -s exit:0 -o file:${data}/expected42 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg -c cat in1.asc
	atf_check -s exit:0 -o file:${data}/expected43 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg -c cat < in1.asc
	atf_check -s exit:0 -o file:${data}/expected44 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg in2.gpg
	atf_check -s exit:0 -o file:${data}/expected45 -e empty env TZ=US/Pacific netpgpverify -k dsa-pubring.gpg in2.asc
}

# Test set 3 - test signatures made by GnuPG v1 and v2
atf_test_case netpgpverify_testset_3_gnupg_version_signatures

netpgpverify_testset_3_gnupg_version_signatures_head() {
	atf_set "descr" "Test set 3 (signatures by GnuPG v1 and v2) for netpgpverify"
}

netpgpverify_testset_3_gnupg_version_signatures_body() {
        atf_expect_fail "PR bin/59936 - does not support signatures generated by gnugp2"
	data=$(atf_get_srcdir)/data
	for i in message message.v1.asc message.v1.sig message.v2.asc message.v2.sig
	do
		cp ${data}/${i} .
	done
	ln -s message message.v1
	ln -s message message.v2

	# signature made by GnuPG 1.x
	atf_check -s exit:0 -o file:${data}/message.v1.asc.expected -e empty env TZ=Europe/Vienna netpgpverify -k ${data}/message.keyring message.v1.asc
	atf_check -s exit:0 -o file:${data}/message.v1.sig.expected -e empty env TZ=Europe/Vienna netpgpverify -k ${data}/message.keyring message.v1.sig
	# signature made by GnuPG 2.x
	atf_check -s exit:0 -o file:${data}/message.v2.asc.expected -e empty env TZ=Europe/Vienna netpgpverify -k ${data}/message.keyring message.v2.asc
	atf_check -s exit:0 -o file:${data}/message.v2.sig.expected -e empty env TZ=Europe/Vienna netpgpverify -k ${data}/message.keyring message.v2.sig
}

# all test sets
atf_init_test_cases() {
	atf_add_test_case netpgpverify_testset_1_rsa_signatures
	atf_add_test_case netpgpverify_testset_2_dsa_signatures
	atf_add_test_case netpgpverify_testset_3_gnupg_version_signatures
}

