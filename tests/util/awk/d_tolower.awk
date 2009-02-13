# $NetBSD: d_tolower.awk,v 1.1 2009/02/13 05:19:51 jmmv Exp $

END {
	print tolower($0);
}
