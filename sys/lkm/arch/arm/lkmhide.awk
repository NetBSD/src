#	$NetBSD: lkmhide.awk,v 1.1.4.4 2004/09/21 13:36:23 skrll Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
