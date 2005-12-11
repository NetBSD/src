#	$NetBSD: lkmwrap.awk,v 1.2 2005/12/11 12:24:46 christos Exp $

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
