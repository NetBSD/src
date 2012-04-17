# $NetBSD: d_toupper.awk,v 1.1.2.2 2012/04/17 00:09:16 yamt Exp $

END {
	print toupper($0);
}
