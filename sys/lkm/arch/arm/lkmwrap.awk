#	$NetBSD: lkmwrap.awk,v 1.1.4.2 2004/08/03 10:53:58 skrll Exp $

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
