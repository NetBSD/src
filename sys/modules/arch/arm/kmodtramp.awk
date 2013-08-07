#	$NetBSD: kmodtramp.awk,v 1.1 2013/08/07 17:06:22 matt Exp $
#
BEGIN {
	print "#include <machine/asm.h>"
}

$2 == "R_ARM_PC24" || $2 == "R_ARM_CALL" || $2 == "R_ARM_JUMP24" {
	if (x[$3] != "")
		next;
	if (index($3, ".text") > 0)
		next;
	fn=$3
	sub("__wrap_", "", fn)
	if (fn == $3)
		next;
	print "KMODTRAMPOLINE("fn")"
	x[$3]=".";
}
