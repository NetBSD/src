#	$NetBSD: Makefile,v 1.3.4.1 2000/03/01 12:52:35 he Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	rwho

.if (${MACHINE_ARCH} == "i386")
CFLAGS=	-O	# work around bug in egcs
.endif

.include <bsd.prog.mk>
