#	$NetBSD: lkmtramp.awk,v 1.1.4.3 2004/09/18 14:54:08 skrll Exp $
#
BEGIN {
	print "#include <machine/asm.h>"
}

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next;
	print "ENTRY(__wrap_"$3")"
	print "\tldr\tpc,1f"
	print "1:\t.word\t__real_"$3
	x[$3]=".";
}
