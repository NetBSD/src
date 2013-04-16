# $NetBSD: t_umountstress.sh,v 1.1 2013/04/16 22:05:44 mlelstv Exp $
#
# Copyright (c) 2013 The NetBSD Foundation, Inc.
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

TMPMP=umount-stress_mount
TMPIM=umount-stress.im

VND=vnd0
BVND=/dev/${VND}
CVND=/dev/r${VND}
MPART=a

atf_test_case umountstress cleanup
umountstress_head()
{
	atf_set "descr" "Checks stressing unmounting a busy filesystem"
	atf_set "require.user" "root"
}
umountstress_body()
{
	cat >disktab <<EOF
floppy288|2.88MB 3.5in Extra High Density Floppy:\
	:ty=floppy:se#512:nt#2:rm#300:ns#36:nc#80:\
	:pa#5760:oa#0:ba#4096:fa#512:ta=4.2BSD:\
	:pb#5760:ob#0:\
	:pc#5760:oc#0:
EOF

	echo "*** Creating a dummy directory tree at" \
	     "${TMPMP} mounted on ${TMPIM}"

	atf_check -o ignore -e ignore mkdir ${TMPMP}
	atf_check -o ignore -e ignore touch ${TMPMP}/under_the_mount
	atf_check -o ignore -e ignore dd if=/dev/zero of=${TMPIM} count=5860
	atf_check -o ignore -e ignore vnconfig -v ${VND} ${TMPIM}
	atf_check -o ignore -e ignore disklabel -f disktab -rw ${VND} floppy288
	atf_check -o ignore -e ignore newfs -i 500 -b 8192 -f 1024 ${CVND}${MPART}
	atf_check -o ignore -e ignore mount -o async ${BVND}${MPART} ${TMPMP}

	echo "*** Testing unmount"

	touch ${TMPMP}/hold
	exec 3< ${TMPMP}/hold

	(
		for l in 0 1 2 3 4 5 6 7 8 9 10; do
		for i in 0 1 2 3; do
		for j in 0 1 2 3 4 5 6 7 8 9; do
		for k in 0 1 2 3 4 5 6 7 8 9; do
			dd msgfmt=quiet if=/dev/zero count=1 of=test$i$j$k
			if [ $? -ne 0 ]; then
				atf_fail "File operation failed"
			fi
		done
		done
		done
		done
	) &
	busypid=$!

	while kill 2>/dev/null -0 $busypid; do
		if err=$(umount ${TMPMP} 2>&1); then
			kill $busypid
			wait
			atf_fail "Unmount succeeded while busy"
		else
			case $err in
			*:\ Device\ busy)
				;;
			*)
				kill $busypid
				wait
				atf_fail "Unmount failed: $err"
				;;
			esac
		fi
	done
	wait
}
umountstress_cleanup()
{
	echo "*** Cleaning up ${TMPMP}, ${TMPIM}."
	umount -f "${TMPMP}"
	vnconfig -u "${VND}"
}

atf_init_test_cases()
{
	atf_add_test_case umountstress
}
