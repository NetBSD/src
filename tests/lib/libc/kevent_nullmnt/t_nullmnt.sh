atf_test_case nullmnt cleanup
nullmnt_head()
{ 
	atf_set "descr" "ensure file events traverse null-mounts"
}    

nullmnt_body()
{    
	curdir=$(pwd)
	helper=$(atf_get_srcdir)/h_nullmnt

	mkdir ${curdir}/realdir
	mkdir ${curdir}/nulldir
	mount -t null ${curdir}/realdir ${curdir}/nulldir
	rm -f ${curdir}/realdir/afile
	touch ${curdir}/realdir/afile

	atf_expect_fail "PR kern/56713"

	atf_check -e ignore -o ignore -s exit:0 \
		${helper} ${curdir}/realdir/afile ${curdir}/nulldir/afile

} 

nullmnt_cleanup()
{
	curdir=$(pwd)
	umount ${curdir}/nulldir
	rm -rf ${curdir}/realdir ${curdir}/nulldir
}

atf_init_test_cases()
{
	atf_add_test_case nullmnt
}
