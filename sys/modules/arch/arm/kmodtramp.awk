#	$NetBSD: kmodtramp.awk,v 1.1.6.2 2014/05/22 11:41:06 yamt Exp $
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
