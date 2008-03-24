#	$NetBSD: lkmwrap.awk,v 1.1.6.2 2008/03/24 07:16:22 keiichi Exp $

$2 == "R_PARISC_PCREL17F" {
	if (x[$3] != "")
		next;
	if ($3 == "\.text")
		next
	printf " --wrap "$3;
	x[$3]=".";
}
