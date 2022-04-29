atf_test_case nullmnt cleanup
nullmnt_head()
{ 
	atf_set "descr" "ensure file events traverse null-mounts"
}    

nullmnt_body()
{    
	srcdir=$(atf_get_srcdir)
	helper=${srcdir}/h_nullmnt

	mkdir ${srcdir}/realdir
	mkdir ${srcdir}/nulldir
	mount -t null ${srcdir}/realdir ${srcdir}/nulldir
	rm -f ${srcdir}/realdir/afile
	touch ${srcdir}/realdir/afile

	atf_expect_fail "PR kern/56713"

	atf_check -e ignore -o ignore -s exit:0 \
		${helper} ${srcdir}/realdir/afile ${srcdir}/nulldir/afile

} 

nullmnt_cleanup()
{
	srcdir=$(atf_get_srcdir)
	umount ${srcdir}/nulldir
	rm -rf ${srcdir}/realdir ${srcdir}/nulldir
}

atf_init_test_cases()
{
	atf_add_test_case nullmnt
}
