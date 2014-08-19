#	$NetBSD: kmodtramp.awk,v 1.1.10.2 2014/08/20 00:04:32 tls Exp $
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
