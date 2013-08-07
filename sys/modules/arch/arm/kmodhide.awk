#	$NetBSD: kmodhide.awk,v 1.1 2013/08/07 17:06:22 matt Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
