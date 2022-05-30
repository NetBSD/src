# common test body
#    $1 = pathname of file to monitor
#    $2 = pathname of file to update/modify

nullmnt_common()
{    
	curdir=$(pwd)
	helper=$(atf_get_srcdir)/h_nullmnt

	mkdir ${curdir}/lower_dir
	mkdir ${curdir}/upper_dir
	mount -t null ${curdir}/lower_dir ${curdir}/upper_dir
	rm -f ${curdir}/lower_dir/afile
	touch ${curdir}/lower_dir/afile

	atf_check -e ignore -o ignore -s exit:0		\
		${helper} ${curdir}/${1} ${curdir}/${2}
}

nullmnt_common_cleanup()
{
	curdir=$(pwd)
	umount ${curdir}/upper_dir
	rm -rf ${curdir}/lower_dir ${curdir}/upper_dir
}

atf_test_case nullmnt_upper_lower cleanup
nullmnt_upper_lower_head()
{
	atf_set "descr" "ensure upper fs events seen on lower fs"
}
nullmnt_upper_lower_body()
{
	atf_expect_fail "PR kern/56713"
	nullmnt_common lower_dir/afile upper_dir/afile
} 
nullmnt_upper_lower_cleanup()
{
	nullmnt_common_cleanup
}

atf_test_case nullmnt_upper_upper cleanup
nullmnt_upper_upper_head()
{
	atf_set "descr" "ensure upper fs events seen on upper fs"
}
nullmnt_upper_upper_body()
{
	atf_expect_fail "PR kern/56713"
	nullmnt_common upper_dir/afile upper_dir/afile
} 
nullmnt_upper_upper_cleanup()
{
	nullmnt_common_cleanup
}
atf_test_case nullmnt_lower_upper cleanup
nullmnt_lower_upper_head()
{
	atf_set "descr" "ensure lower fs events seen on upper fs"
}
nullmnt_lower_upper_body()
{
	nullmnt_common upper_dir/afile lower_dir/afile
} 
nullmnt_lower_upper_cleanup()
{
	nullmnt_common_cleanup
}

atf_test_case nullmnt_lower_lower cleanup
nullmnt_lower_lower_head()
{
	atf_set "descr" "ensure lower fs events seen on lower fs"
}
nullmnt_lower_lower_body()
{
	nullmnt_common lower_dir/afile lower_dir/afile
} 
nullmnt_lower_lower_cleanup
{
	nullmnt_common_cleanup()
}

atf_init_test_cases()
{
	atf_add_test_case nullmnt_upper_upper
	atf_add_test_case nullmnt_upper_lower
	atf_add_test_case nullmnt_lower_upper
	atf_add_test_case nullmnt_lower_lower
}
