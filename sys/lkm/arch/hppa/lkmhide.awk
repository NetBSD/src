#	$NetBSD: lkmhide.awk,v 1.1 2008/03/01 12:40:08 skrll Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
