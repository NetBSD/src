#	Id: options.awk,v 10.1 1995/06/08 19:00:01 bostic Exp (Berkeley) Date: 1995/06/08 19:00:01
 
/^\/\* O_[0-9A-Z_]*/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
END {
	printf("#define O_OPTIONCOUNT %d\n", cnt);
}
