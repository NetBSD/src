#	$NetBSD: kmodhide.awk,v 1.1.2.2 2013/08/28 23:59:35 rmind Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
