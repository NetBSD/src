#	$NetBSD: options.awk,v 1.3 2001/03/31 11:37:46 aymeric Exp $
#
#	@(#)options.awk	10.1 (Berkeley) 6/8/95
 
/^\/\* O_[0-9A-Z_]*/ {
	printf("#define %s %d\n", $2, cnt++);
	next;
}
END {
	printf("#define O_OPTIONCOUNT %d\n", cnt);
}
