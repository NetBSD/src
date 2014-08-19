#	$NetBSD: kmodhide.awk,v 1.1.10.2 2014/08/20 00:04:32 tls Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
