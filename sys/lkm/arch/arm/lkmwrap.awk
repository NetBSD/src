#	$NetBSD: lkmwrap.awk,v 1.1.4.4 2004/09/21 13:36:23 skrll Exp $

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
