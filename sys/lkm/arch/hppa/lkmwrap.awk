#	$NetBSD: lkmwrap.awk,v 1.1.4.2 2008/03/23 02:05:02 matt Exp $

$2 == "R_PARISC_PCREL17F" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
