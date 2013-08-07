#	$NetBSD: kmodwrap.awk,v 1.1 2013/08/07 17:06:22 matt Exp $

$2 == "R_ARM_PC24" || $2 == "R_ARM_CALL" || $2 == "R_ARM_JUMP24" {
	if (x[$3] != "")
		next;
	if (index($3, ".text") > 0)
		next
	printf " --wrap="$3;
	x[$3]=".";
}
