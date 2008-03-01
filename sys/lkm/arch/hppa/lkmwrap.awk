#	$NetBSD: lkmwrap.awk,v 1.1 2008/03/01 12:40:08 skrll Exp $

$2 == "R_PARISC_PCREL17F" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
