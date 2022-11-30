# $NetBSD: t_extattr.sh,v 1.4 2022/11/30 07:20:36 martin Exp $
#
#  Copyright (c) 2021 The NetBSD Foundation, Inc.
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

VND=vnd0
BDEV=/dev/${VND}
CDEV=/dev/r${VND}
IMG=fsimage
MNT=mnt

atf_test_case fsck_extattr_enable cleanup
atf_test_case fsck_extattr_enable_corrupted cleanup
atf_test_case fsck_extattr_disable cleanup

cleanup()
{
	echo in cleanup
	umount -f ${MNT} > /dev/null 2>&1 || true
	vnconfig -u ${VND} > /dev/null 2>&1 || true
}

fsck_extattr_enable_head()
{
	atf_set "descr" "Checks fsck_ffs enabling extattrs"
	atf_set "require.user" "root";
}

fsck_extattr_enable_body()
{
	atf_check mkdir -p ${MNT}

	atf_check -o ignore newfs -O2 -s 4m -F ${IMG}
	atf_check vnconfig ${VND} ${IMG}

	# Verify that extattrs are disabled.
	atf_check -o ignore -e 'match:POSIX1e ACLs not supported by this fs' \
		tunefs -p enable ${CDEV}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check touch ${MNT}/file
	atf_check -s exit:1 -e ignore setextattr user name1 value1 ${MNT}/file
	atf_check umount ${MNT}

	# Enable extattrs.
	atf_check -o 'match:ENABLING EXTATTR SUPPORT' \
		fsck_ffs -c ea ${CDEV}

	# Verify that extattrs are now enabled.
	atf_check -o 'match:POSIX1e ACLs set' -e ignore \
		tunefs -p enable ${CDEV}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check touch ${MNT}/file
	atf_check setextattr user testname testvalue ${MNT}/file
	atf_check -o 'match:testvalue' getextattr user testname ${MNT}/file
	atf_check umount ${MNT}
	atf_check vnconfig -u ${VND}
}

fsck_extattr_enable_cleanup()
{
	cleanup
}

fsck_extattr_enable_corrupted_head()
{
	atf_set "descr" "Checks fsck_ffs enabling extattrs with corruption"
	atf_set "require.user" "root";
}

fsck_extattr_enable_corrupted_body()
{
	atf_check mkdir -p ${MNT}

	# Create an fs with extattrs enabled and set an extattr on the test file.
	atf_check -o ignore newfs -O2ea -b 8k -f 1k -s 4m -F ${IMG}
	atf_check vnconfig ${VND} ${IMG}

	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check touch ${MNT}/file
	atf_check setextattr user testname testvalue ${MNT}/file
	atf_check -o 'match:testvalue' getextattr user testname ${MNT}/file
	atf_check umount ${MNT}

	# Find the location and size of the extattr block.
	extb0=$(printf 'cd file\niptrs\n' | fsdb -n $CDEV | grep 'di_extb 0' |
		awk '{print $3}')
	extsize=$(printf 'cd file\n' | fsdb -n $CDEV | grep EXTSIZE | tail -1 |
		awk '{print $4}' | sed 's,.*=,,')
	atf_check [ $extb0 != 0 ]
	atf_check [ $extsize != 0 ]

	# Recreate the fs with extattrs disabled and set the extattr block
	# size/location of the new test file to the same values as the old
	# test file.  This simulates extattrs having been created in a
	# UFS2-non-ea file system before UFS2ea was invented.
	atf_check -o ignore newfs -O2 -b 8k -f 1k -s 4m -F ${IMG}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check touch ${MNT}/file
	atf_check umount ${MNT}
	printf "cd file\nchextb 0 $extb0\n" | fsdb -N $CDEV
	printf "cd file\nchextsize $extsize\n" | fsdb -N $CDEV

	# Convert to enable extattrs.
	atf_check -o 'match:CLEAR EXTATTR FIELDS' \
		  -o 'match:ENABLING EXTATTR SUPPORT' \
		  fsck_ffs -y -c ea ${CDEV}

	# Verify that the test file does not have the extattr.
	atf_check -o ignore fsck_ffs -n ${CDEV}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check -s exit:1 -e 'match:Attribute not found' \
		  getextattr user testname ${MNT}/file
	atf_check umount ${MNT}
	atf_check vnconfig -u ${VND}
}

fsck_extattr_enable_corrupted_cleanup()
{
	cleanup
}

fsck_extattr_disable_head()
{
	atf_set "descr" "Checks fsck_ffs disabling extattrs"
	atf_set "require.user" "root";
}

fsck_extattr_disable_body()
{
	atf_check mkdir -p ${MNT}

	# Create an fs with extattrs enabled and set an extattr on the test file.
	atf_check -o ignore newfs -O2ea -b 8k -f 1k -s 4m -F ${IMG}
	atf_check vnconfig ${VND} ${IMG}

	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check touch ${MNT}/file
	atf_check setextattr user testname testvalue ${MNT}/file
	atf_check -o 'match:testvalue' getextattr user testname ${MNT}/file
	atf_check umount ${MNT}

	# Convert to disable extattrs.
	atf_check -o 'match:CLEAR EXTATTR FIELDS' \
		  -o 'match:DISABLING EXTATTR SUPPORT' \
		  fsck_ffs -y -c no-ea ${CDEV}

	# Verify that the test file does not have the test extattr.
	atf_check -o ignore fsck_ffs -n ${CDEV}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check -s exit:1 -e 'match:getextattr: mnt/file: failed: Operation not supported' \
		  getextattr user testname ${MNT}/file
	atf_check umount ${MNT}

	# Convert to enable extattrs again.
	atf_check -o 'match:ENABLING EXTATTR SUPPORT' \
		  fsck_ffs -y -c ea ${CDEV}

	# Verify that the test extattr is still gone.
	atf_check -o ignore fsck_ffs -n ${CDEV}
	atf_check mount -t ffs ${BDEV} ${MNT}
	atf_check -s exit:1 -e 'match:Attribute not found' \
		  getextattr user testname ${MNT}/file
	atf_check umount ${MNT}

	atf_check vnconfig -u ${VND}
}

fsck_extattr_disable_cleanup()
{
	cleanup
}

atf_init_test_cases()
{
	atf_add_test_case fsck_extattr_enable
	atf_add_test_case fsck_extattr_enable_corrupted
	atf_add_test_case fsck_extattr_disable
}
