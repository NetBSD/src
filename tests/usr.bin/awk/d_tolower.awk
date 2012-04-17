# $NetBSD: d_tolower.awk,v 1.1.2.2 2012/04/17 00:09:16 yamt Exp $

END {
	print tolower($0);
}
