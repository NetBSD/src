RDEV=`pwd`/bigfile.img
dd if=/dev/zero of=$RDEV bs=1024 count=1024

atf_test_case newfs_fsck_case
newfs_fsck_case_head() {
	atf_set "descr" "This test case endures that the filesystem produced by newfs_exfatfs is consistent with the expectations of fsck_exfatfs."
}
newfs_fsck_case_body() {
	CSIZE=4096
	while [ $CSIZE -le 65536 ]
	do
		atf_check -s eq:0 -e empty newfs_exfatfs -w -q -c $CSIZE $RDEV
		atf_check -s eq:0 fsck_exfatfs -q -n $RDEV
		CSIZE=$(( $CSIZE * 2 ))
	done
}

atf_test_case newfs_dumpfs_case
newfs_dumpfs_case_head() {
        atf_set "descr" "This test case endures that the filesystem parameters passed to newfs_exfatfs are written into the file system and recognized by dumpexfatfs."
}
newfs_dumpfs_case_body() {
	cat >expout <<EOF
newfs_exfatfs -# 0x1 -a 8192 -c 8192 -h 16384 -S 512 -s 20480 $RDEV

EOF
	atf_check -s eq:0 -e empty newfs_exfatfs -Fwq -# 0x1 -a 8192 -c 8192 -h 16384 -S 512 -s 20480 $RDEV
	atf_check -s eq:0 -e empty -o file:expout dumpexfatfs -n $RDEV
}

atf_init_test_cases() {
	atf_add_test_case newfs_dumpfs_case
	atf_add_test_case newfs_fsck_case
}
