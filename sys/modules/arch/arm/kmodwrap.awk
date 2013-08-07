#	$NetBSD: kmodwrap.awk,v 1.2 2013/08/07 17:29:15 matt Exp $

$2 == "*UND*" {
	undef[$4]=".";
}

$2 == "R_ARM_PC24" || $2 == "R_ARM_CALL" || $2 == "R_ARM_JUMP24" {
	if (x[$3] != "" || undef[$3] != ".")
		next;
	printf " --wrap="$3;
	x[$3]=".";
}
