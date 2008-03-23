#	$NetBSD: lkmhide.awk,v 1.1.4.2 2008/03/23 02:05:02 matt Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
