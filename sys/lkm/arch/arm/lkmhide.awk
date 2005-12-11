#	$NetBSD: lkmhide.awk,v 1.2 2005/12/11 12:24:46 christos Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
