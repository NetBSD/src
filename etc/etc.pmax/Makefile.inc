# $NetBSD: Makefile.inc,v 1.1 1997/11/11 04:37:20 jonathan Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# pmax ldconfig/ld.so needs a ld.so.conf set up to search /usr/X11R6/lib.
BIN1+=	ld.so.conf
