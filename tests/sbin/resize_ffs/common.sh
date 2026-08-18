
# Common settings and functions for the various resize_ffs tests.
#

# called from atf_init_test_cases
setupvars()
{
	IMG=fsimage
	TDBASE64=$(atf_get_srcdir)/testdata.tar.gz.base64
	GOODMD5=$(atf_get_srcdir)/testdata.md5
	export RUMP_SERVER=unix://csock
	# need PLAINMNT for the umount op
	RUMPMNT=/rump/mnt
	PLAINMNT=/mnt
	# set BYTESWAP to opposite-endian.
	if [ $(sysctl -n hw.byteorder) = "1234" ]; then
		BYTESWAP=be
	else
		BYTESWAP=le
	fi
}

# test_case() taken from the tests/ipf/h_common.sh
# Used to declare the atf goop for a test.
test_case()
{
	local name="${1}"; shift
	local check_function="${1}"; shift

	atf_test_case "${name}" cleanup
	eval "${name}_head() { \
		atf_set "require.user" "unprivileged" ; \
		atf_set "require.progs" "rump_server" ; \
	}"
	eval "${name}_body() { \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		umount -f mnt  ; \
		: reset error ; \
	}"
}

# Used to declare the atf goop for a test expected to fail.
test_case_xfail()
{
	local name="${1}"; shift
	local reason="${1}"; shift
	local check_function="${1}"; shift

	atf_test_case "${name}" cleanup
	eval "${name}_head() { \
		atf_set "descr" "xfail test case"; \
	}"
	eval "${name}_body() { \
		atf_expect_fail "${reason}" ; \
		${check_function} " "${@}" "; \
	}"
	eval "${name}_cleanup() { \
		unset LD_PRELOAD ; \
		rump.halt >/dev/null 2>&1  ; \
		: reset error ; \
	}"
}

rump_mount()
{
	atf_check -s exit:0 rump_server -lrumpvfs -lrumpfs_ffs -lrumpdev_disk \
		-d key=/img,hostpath=${IMG},size=host ${RUMP_SERVER}
	export LD_PRELOAD=/usr/lib/librumphijack.so
	mkdir -p ${RUMPMNT}
	atf_check -s exit:0 -e ignore env RUMPHIJACK='path=/rump,blanket=/img' \
		mount_ffs /img ${RUMPMNT}
}

rump_umount()
{
	# specify the path without "/rump" for the lookup to succeed
	atf_check -s exit:0 env RUMPHIJACK='path=/rump,blanket=/img:/mnt,vfs=all' \
		umount ${PLAINMNT}
	unset LD_PRELOAD
	atf_check -s exit:0 rump.halt
}

# copy_data requires the mount already done;  makes one copy of the test data
copy_data ()
{
	# exec a shell here because LD_PRELOAD only takes effect across
	# an exec, and "cd" is a builtin.
	uudecode -p ${TDBASE64} | \
		/bin/sh -c "cd ${RUMPMNT}; tar xzf - -s/testdata/TD$1/"
}

copy_multiple ()
{
	local i
	for i in $(seq $1); do
		copy_data $i
	done
}

# remove_data removes one directory worth of test data; the purpose
# is to ensure data exists near the end of the fs under test.
remove_data ()
{
	rm -rf ${RUMPMNT}/TD$1
}

remove_multiple ()
{
	local i
	for i in $(seq $1); do
		remove_data $i
	done
}

# verify that the data in a particular directory is still OK
# generated md5 file doesn't need explicit cleanup thanks to ATF
check_data ()
{
	# need to exec for "cd" to be correct
	/bin/sh -c "cd ${RUMPMNT}/TD$1 && md5 *" > TD$1.md5
	atf_check diff -u ${GOODMD5} TD$1.md5
}

# supply begin and end arguments
check_data_range ()
{
	local i
	for i in $(seq $1 $2); do
		check_data $i
	done
}


resize_ffs()
{
	echo "in resize_ffs:" ${@}
	local bs=$1
	local fragsz=$2
	local osize=$3
	local nsize=$4
	local fslevel=$5
	local numdata=$6
	local swap=$7
	local avail=$( df -m . | awk '{if (int($4) > 0) print $4}' )
	# convert MB size to blocks
	avail=$(( $avail \* 2 \* 1024 ))
	if [ $avail -lt $osize ] || [ $avail -lt $nsize ]; then
		atf_skip "not enough free space in working directory"
	fi
	echo "bs is ${bs} numdata is ${numdata}"
	echo "****resizing fs with blocksize ${bs}"

	# we want no more than 16K/inode to allow test files to copy.
	local fpi=$((fragsz * 4))
	local i
	if [ $fpi -gt 16384 ]; then
		i="-i 16384"
	fi
	if [ x$swap != x ]; then
		newfs -B ${BYTESWAP} -O${fslevel} -b ${bs} -f ${fragsz} \
			-s ${osize} ${i} -F ${IMG}
	else
		newfs -O${fslevel} -b ${bs} -f ${fragsz} -s ${osize} ${i} \
			-F ${IMG}
	fi

	rump_mount

	copy_multiple ${numdata}

	if [ ${nsize} -lt ${osize} ]; then
	    # how much data to remove so fs can be shrunk
	    local remove=$((numdata-numdata*nsize/osize))
	    local dataleft=$((numdata-remove))
	    echo remove is $remove dataleft is $dataleft
	    remove_multiple ${remove}
	fi

	rump_umount
	# Check that resize needed
	atf_check -s exit:0 -o ignore resize_ffs -c -y -s ${nsize} ${IMG}
	atf_check -s exit:0 -o ignore resize_ffs -y -s ${nsize} ${IMG}
	atf_check -s exit:0 -o ignore fsck_ffs -f -n -F ${IMG}
	rump_mount
	if [ ${nsize} -lt ${osize} ]; then
	    check_data_range $((remove + 1)) ${numdata}
	else
	    # checking everything because we don't delete on grow
	    check_data_range 1 ${numdata}
	fi
	rump_umount
	# Check that no resize needed
	atf_check -s exit:1 -o ignore resize_ffs -c -y -s ${nsize} ${IMG}
}
