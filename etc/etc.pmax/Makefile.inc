# $NetBSD: Makefile.inc,v 1.1.2.2 1997/12/22 22:35:29 perry Exp $
#
#	etc.pmax/Makefile.inc -- pmax-specific etc Makefile targets
#

# pmax ldconfig/ld.so needs a ld.so.conf set up to search /usr/X11R6/lib.
BIN1+=	etc.pmax/ld.so.conf
