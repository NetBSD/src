#	$Id: options.awk,v 1.1.1.1.2.2 2008/05/18 12:29:23 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:23 $
 
/^\/\* O_[0-9A-Z_]*/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
END {
	printf("#define O_OPTIONCOUNT %d\n", cnt);
}
