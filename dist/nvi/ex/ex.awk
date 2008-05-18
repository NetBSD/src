#	$Id: ex.awk,v 1.1.1.1.2.2 2008/05/18 12:29:26 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:26 $
 
/^\/\* C_[0-9A-Z_]* \*\/$/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
