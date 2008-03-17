#	$NetBSD: lkmhide.awk,v 1.1.2.2 2008/03/17 09:15:40 yamt Exp $

substr($NF, 1, 7) == "__wrap_" {
	print " --localize-symbol "$NF;
}
