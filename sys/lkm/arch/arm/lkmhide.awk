#	$NetBSD: lkmhide.awk,v 1.1.4.2 2004/08/03 10:53:58 skrll Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
