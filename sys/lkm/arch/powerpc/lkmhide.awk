#	$NetBSD: lkmhide.awk,v 1.1 2003/04/23 18:34:21 matt Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
