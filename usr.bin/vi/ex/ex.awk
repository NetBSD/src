#	$NetBSD: ex.awk,v 1.3 2001/03/31 11:37:49 aymeric Exp $
#
#	@(#)ex.awk	10.1 (Berkeley) 6/8/95
 
/^\/\* C_[0-9A-Z_]* \*\/$/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
