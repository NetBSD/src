#	$NetBSD: lkmhide.awk,v 1.1.4.3 2004/09/18 14:54:08 skrll Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
