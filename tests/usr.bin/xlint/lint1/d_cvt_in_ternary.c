/*	$NetBSD: d_cvt_in_ternary.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_cvt_in_ternary.c"

/* CVT node handling in ?: operator */
typedef unsigned long int size_t;
struct filecore_direntry {
	unsigned len: 32;
};

int
main(void)
{
	struct filecore_direntry dirent = { 0 };
	size_t uio_resid = 0;
	size_t bytelen = (((dirent.len) < (uio_resid))
	    ? (dirent.len)
	    : (uio_resid));
	return bytelen;
}
