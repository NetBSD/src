# $NetBSD: t_miscquota.sh,v 1.1.2.3 2011/02/12 21:49:08 bouyer Exp $ 
#
#  Copyright (c) 2011 Manuel Bouyer
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 
#  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
#  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

test_case_root walk_list_user quota_walk_list \
    "walk user quota list over several disk blocks" -b le 1 user

test_case_root psnapshot_user quota_snap \
    "create a persistent shapshot of quota-enabled fs, and do some writes" \
    -b le 1 user

test_case_root npsnapshot_user quota_snap \
    "create a non-persistent shapshot of quota-enabled fs, and do some writes" \
    -boL le 1 user

test_case_root psnapshot_unconf_user quota_snap \
    "create a persistent shapshot of quota-enabled fs, and do some writes and unconf" \
    -boC le 1 user

test_case_root npsnapshot_unconf_user quota_snap \
    "create a non-persistent shapshot of quota-enabled fs, and do some writes and unconf" \
    -boLC le 1 user


quota_walk_list()
{
	create_with_quotas_server $*
	local q=$4
	local expect

	case ${q} in
	user)
		expect=u
		fail=g
		;;
	group)
		expect=g
		fail=u
		;;
	*)
		atf_fail "wrong quota type"
		;;
	esac

	# create 100 users, all in the same hash list
	local i=1;
	while [ $i -lt 101 ]; do
		atf_check -s exit:0 \
		   $(atf_get_srcdir)/rump_edquota -${expect} \
		   -s10k/20 -h40M/50k -t 2W/3D $((i * 4096))
		i=$((i + 1))
	done
	# do a repquota
	atf_check -s exit:0 -o 'match:<integer>0x64000' \
	    $(atf_get_srcdir)/rump_repquota -x -${expect} /mnt
	rump_shutdown
}

quota_snap()
{
	local flag=$1; shift
	create_with_quotas $*
	local q=$3
	local expect

	case ${q} in
	user)
		expect=u
		fail=g
		;;
	group)
		expect=g
		fail=u
		;;
	*)
		atf_fail "wrong quota type"
		;;
	esac

	#start our server which takes a snapshot
	atf_check -s exit:0 -o ignore \
	    $(atf_get_srcdir)/h_quota2_tests ${flag} 4 ${IMG} ${RUMP_SERVER}
	# create a few users
	local i=1;
	while [ $i -lt 11 ]; do
		atf_check -s exit:0 \
		   $(atf_get_srcdir)/rump_edquota -${expect} \
		   -s10k/20 -h40M/50k -t 2W/3D $i
		i=$((i + 1))
	done
	# we should have 5 files (root + 4 regular files)
	atf_check -s exit:0 \
	    -o 'match:-        -  7days         5       -       -  7days' \
	    $(atf_get_srcdir)/rump_repquota -av
	#shutdown and check filesystem
	rump_shutdown
}
