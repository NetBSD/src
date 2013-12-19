#	$NetBSD: lkmwrap.awk,v 1.2.90.1 2013/12/19 01:19:26 matt Exp $

$2 == "R_ARM_PC24" {
	if (x[$3] != "")
		next;
	if ($3 == ".text")
		next
	printf " -Wl,--wrap,"$3;
	x[$3]=".";
}
