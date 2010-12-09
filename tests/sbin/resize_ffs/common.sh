
# Common settings and functions for the various resize_ffs tests.
# 

# called from atf_init_test_cases
setupvars()
{
	IMG=fsimage
	TDBASE64=$(atf_get_srcdir)/testdata.tar.gz.base64
	GOODMD5=$(atf_get_srcdir)/testdata.md5
	MNTPT=mnt
	# set BYTESWAP to opposite-endian.
	if [ $(sysctl -n hw.byteorder) = "1234" ]; then
		BYTESWAP=be
	else
		BYTESWAP=le
	fi
}

# make sure to remove test_redo before committing
test_redo ()
{
        local fpi=$((${2} * 4))
        local i
        if [ $fpi -gt 16384 ]; then
                i="-i 16384"
        fi
	sudo umount ${MNTPT}
	newfs -O1 $i -b $1 -f $2 -s $3 -F ${IMG}
	sudo rump_ffs ${IMG} ${MNTPT}
}

# test_case() taken from the tests/ipf/h_common.sh
# Used to declare the atf goop for a test.
test_case()
{
	local name="${1}"; shift
	local check_function="${1}"; shift

	atf_test_case "${name}" cleanup
	eval "${name}_body() { \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		umount -f ${MNTPT}  ; \
	}"
}

# Used to declare the atf goop for a test expected to fail.
test_case_xfail()
{
	local name="${1}"; shift
	local reason="${1}"; shift
	local check_function="${1}"; shift

	atf_test_case "${name}" cleanup
	eval "${name}_body() { \
		atf_expect_fail "${reason}" ; \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		umount -f ${MNTPT}  ; \
	}"
}

# copy_data requires the mount already done;  makes one copy of the test data
copy_data ()
{
	uudecode -p ${TDBASE64} | (cd ${MNTPT}; tar xzf - -s/testdata/TD$1/)
}

copy_multiple ()
{
	local i
	for i in $(jot $1); do
		copy_data $i
	done
}

# remove_data removes one directory worth of test data; the purpose
# is to ensure data exists near the end of the fs under test.
remove_data ()
{
	rm -rf ${MNTPT}/TD$1
}

remove_multiple ()
{
	local i
	for i in $(jot $1); do
		remove_data $i
	done
}

mount_test_fs_image ()
{
	rump_ffs ${IMG} ${MNTPT}
}

unmount_test_fs_image ()
{
	umount -f ${MNTPT}
}


# verify that the data in a particular directory is still OK
# generated md5 file doesn't need explicit cleanup thanks to ATF
check_data ()
{
	(cd ${MNTPT}/TD$1 && md5 *) > TD$1.md5
	atf_check diff -u ${GOODMD5} TD$1.md5
}

# supply begin and end arguments
check_data_range ()
{
	local i
	for i in $(jot $(($2-$1+1)) $1); do
		check_data $i
	done
}
