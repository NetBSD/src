#	$NetBSD: lkmwrap.awk,v 1.2 2004/01/16 00:35:48 matt Exp $
#
/^SYMBOL TABLE:/ {
	doing_symbols = 1;
	next;
}

/^RELOCATION RECORDS/ {
	doing_symbols = 0;
	doing_relocs = 1;
	next;
}

$2 == "*UND*" {
	if (doing_symbols)
		x[$4] = "+";
	next;
}

$2 == "R_PPC_REL24" {
	if (!doing_relocs)
		next;
	if (x[$3] != "+")
		next;
	printf " --wrap "$3;
	x[$3]=".";
}
