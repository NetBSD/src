#	$NetBSD: Makefile.inc,v 1.1.1.1 2003/09/12 02:56:09 lukem Exp $

NOLINT=         1
NOPIC=          1
NOPROFILE=      1

libinstall::


CPPFLAGS+=	${X11FLAGS.SERVER}
