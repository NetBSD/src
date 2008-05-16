#	$Id: options.awk,v 1.1.1.1 2008/05/16 18:03:20 aymeric Exp $ (Berkeley) $Date: 2008/05/16 18:03:20 $
 
/^\/\* O_[0-9A-Z_]*/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
END {
	printf("#define O_OPTIONCOUNT %d\n", cnt);
}
