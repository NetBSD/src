#	$NetBSD: lkmtramp.awk,v 1.1 2003/02/19 19:04:27 matt Exp $
#
BEGIN {
	print "#include <machine/asm.h>"
}

$2 == "R_PPC_REL24" {
	if (x[$3] != "")
		next;
	print "ENTRY(__wrap_"$3")"
	print "\tlis\t0,__real_"$3"@h"
	print "\tori\t0,0,__real_"$3"@l"
	print "\tmtctr\t0"
	print "\tbctr"
	x[$3]=".";
}
