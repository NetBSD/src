#	$NetBSD: lkmwrap.awk,v 1.1 2003/11/04 14:50:27 scw Exp $

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
