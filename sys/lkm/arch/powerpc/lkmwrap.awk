#	$NetBSD: lkmwrap.awk,v 1.1 2003/02/19 19:04:27 matt Exp $

$2 == "R_PPC_REL24" {
	if (x[$3] != "")
		next;
	printf " --wrap "$3;
	x[$3]=".";
}
