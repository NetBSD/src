#	$NetBSD: options.awk,v 1.4 2003/08/28 16:23:41 dsl Exp $
#
#	@(#)options.awk	10.1 (Berkeley) 6/8/95
 
/^\/\* O_[0-9A-Z_]*/ {
	opt = $2
	printf("#define %s %d\n", opt, cnt++);
	ofs = FS
	FS = "\""
	do getline
	while ($1 != "	{")
	FS = ofs
	opt_name = $2
	if (opt_name < prev_name) {
		printf "missorted %s: \"%s\" < \"%s\"\n", opt, opt_name, prev_name >"/dev/stderr"
		exit 1
	}
	prev_name = opt_name
	next;
}
END {
	printf("#define O_OPTIONCOUNT %d\n", cnt);
}
