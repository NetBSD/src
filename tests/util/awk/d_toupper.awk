# $NetBSD: d_toupper.awk,v 1.1.2.2 2009/05/13 19:19:36 jym Exp $

END {
	print toupper($0);
}
