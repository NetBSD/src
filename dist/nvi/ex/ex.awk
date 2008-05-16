#	$Id: ex.awk,v 1.1.1.1 2008/05/16 18:03:50 aymeric Exp $ (Berkeley) $Date: 2008/05/16 18:03:50 $
 
/^\/\* C_[0-9A-Z_]* \*\/$/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
