#	$NetBSD: Makefile.inc,v 1.1 2011/01/02 03:51:21 pgoyette Exp $
#

.if ${MACHINE_ARCH} == "alpha"
COPTS+=	-mieee
.endif
