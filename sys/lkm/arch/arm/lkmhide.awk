#	$NetBSD: lkmhide.awk,v 1.1 2003/11/04 14:50:27 scw Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
