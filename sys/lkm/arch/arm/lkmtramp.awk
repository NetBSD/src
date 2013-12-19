#	$NetBSD: lkmtramp.awk,v 1.2.90.1 2013/12/19 01:19:26 matt Exp $
#
BEGIN {
	print "#include <machine/asm.h>"
}

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == ".text")
		next;
	print "ENTRY(__wrap_"$3")"
	print "\tldr\tpc,1f"
	print "1:\t.word\t__real_"$3
	x[$3]=".";
}
