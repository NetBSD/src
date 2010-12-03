# $NetBSD: t_resize_ffs.sh,v 1.4 2010/12/03 05:23:46 riz Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Jeffrey C. Rizzo.
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

IMG=diskimage
VND=vnd0

# misc routines

atf_test_case grow_ffsv1_partial_cg
grow_ffsv1_partial_cg_head()
{
	atf_set "descr" "Checks successful ffsv1 growth by less" \
			"than a cylinder group"
}
grow_ffsv1_partial_cg_body()
{
	echo "***resize_ffs grow test"

	# resize_ffs only supports ffsv1 at the moment
	atf_check -o ignore -e ignore newfs -V1 -s 4000 -F ${IMG}

	# size to grow to is chosen to cause partial cg
	atf_check -s exit:0 resize_ffs -y -s 5760 ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
}

atf_test_case grow_ffsv1_64k
grow_ffsv1_64k_head()
{
	atf_set "descr" "Checks successful ffsv1 growth with 64k blocksize"
}
grow_ffsv1_64k_body()
{
	echo "***resize_ffs grow 64k block test"

	atf_check -o ignore -e ignore newfs -V1 -b 65536 -f 8192 \
		-s 32768 -F ${IMG}

	# grow the fs to the full partition size (131072)
	atf_check -s exit:0 resize_ffs -y -s 131072 ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
	atf_check -o match:'size[[:space:]]*8192' dumpfs ${IMG}
}

atf_test_case shrink_ffsv1_64k
shrink_ffsv1_64k_head()
{
	atf_set "descr" "Checks successful ffsv1 shrinkage with 64k blocksize"
}
shrink_ffsv1_64k_body()
{
	echo "***resize_ffs shrink 64k block test"

	atf_check -o ignore -e ignore newfs -V1 -b 65536 -f 8192 -s 131072 \
		 -F ${IMG}

	# shrink it to half size
	atf_check -s exit:0 resize_ffs -y -s 65536 ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
	atf_check -o match:'size[[:space:]]*4096' dumpfs ${IMG}
}

atf_test_case shrink_ffsv1_partial_cg
shrink_ffsv1_partial_cg_head()
{
	atf_set "descr" "Checks successful shrinkage of ffsv1 by" \
			"less than a cylinder group"
}
shrink_ffsv1_partial_cg_body()
{
	echo "*** resize_ffs shrinkage partial cg test"

	atf_check -o ignore -e ignore newfs -V1 -F -s 5760 ${IMG}

	# shrink so there's a partial cg at the end
	atf_check -s exit:0 resize_ffs -s 4000 -y ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
}

atf_init_test_cases()
{
	atf_add_test_case grow_ffsv1_partial_cg
	atf_add_test_case grow_ffsv1_64k
	atf_add_test_case shrink_ffsv1_partial_cg
	atf_add_test_case shrink_ffsv1_64k
}
