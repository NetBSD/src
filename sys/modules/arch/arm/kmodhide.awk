#	$NetBSD: kmodhide.awk,v 1.1.6.2 2014/05/22 11:41:06 yamt Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
