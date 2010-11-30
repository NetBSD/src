# $NetBSD: t_resize_ffs.sh,v 1.1 2010/11/30 23:17:27 riz Exp $
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

atf_test_case grow cleanup
grow_head()
{
	atf_set "descr" "Checks successful fs growth"
	atf_set "require.user"	"root"
}
grow_body()
{
	cat >disktab <<EOF
floppy288|2.88MB 3.5in Extra High Density Floppy:\
	:ty=floppy:se#512:nt#2:rm#300:ns#36:nc#80:\
	:pa#5760:oa#0:ba#4096:fa#512:ta=4.2BSD:\
	:pb#5760:ob#0:\
	:pc#5760:oc#0:
EOF

	echo "***resize_ffs grow test"

	atf_check -o ignore -e ignore dd if=/dev/zero of=${IMG} count=5860
	atf_check -o ignore -e ignore vnconfig -v ${VND} ${IMG}
	atf_check -o ignore -e ignore disklabel -f disktab -rw ${VND} floppy288
	# resize_ffs only supports ffsv1 at the moment
	atf_check -o ignore -e ignore newfs -V1 -s 3000 /dev/r${VND}a

	# grow the fs to the full partition size (5760)
	atf_check -s exit:0 resize_ffs -y /dev/r${VND}a
	atf_check -s exit:0 -o ignore fsck -f -n /dev/r${VND}a
}
grow_cleanup()
{
	vnconfig -u ${VND}
}

atf_test_case shrink cleanup
shrink_head()
{
	atf_set "descr" "Checks successful fs shrinkage"
	atf_set "require.user"	"root"
}
shrink_body()
{
	cat >disktab <<EOF
floppy288|2.88MB 3.5in Extra High Density Floppy:\
	:ty=floppy:se#512:nt#2:rm#300:ns#36:nc#80:\
	:pa#5760:oa#0:ba#4096:fa#512:ta=4.2BSD:\
	:pb#5760:ob#0:\
	:pc#5760:oc#0:
EOF

	echo "*** resize_ffs shrinkage test"

	atf_check -o ignore -e ignore dd if=/dev/zero of=${IMG} count=5860
	atf_check -o ignore -e ignore vnconfig -v ${VND} ${IMG}
	atf_check -o ignore -e ignore disklabel -f disktab -rw ${VND} floppy288
	# resize_ffs only supports ffsv1 at the moment
	atf_check -o ignore -e ignore newfs -V1 /dev/r${VND}a

	# shrink the fs to something smaller.  Would be better to have
	# data in there, but one step at a time
	atf_check -s exit:0 resize_ffs -s 3000 -y /dev/r${VND}a
	atf_expect_fail "PR bin/44177"
	atf_check -s exit:0 -o ignore fsck -f -n /dev/r${VND}a
}
shrink_cleanup()
{
	vnconfig -u ${VND}
}

atf_init_test_cases()
{
	atf_add_test_case grow
	atf_add_test_case shrink
}
