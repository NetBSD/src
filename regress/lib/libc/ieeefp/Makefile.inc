#	$NetBSD: Makefile.inc,v 1.3 2002/01/18 22:18:36 ross Exp $
#
# do not install regression test programs
proginstall::

.if ${MACHINE_ARCH} == "alpha"
COPTS+=	-mieee
.endif
