#	$NetBSD: lkmhide.awk,v 1.1.6.2 2008/03/24 07:16:22 keiichi Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
