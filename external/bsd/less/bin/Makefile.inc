# $NetBSD: Makefile.inc,v 1.2 2023/10/06 05:57:16 simonb Exp $

.include "../Makefile.inc"

BINDIR=		/usr/bin

CPPFLAGS+=	-I${LESS_DISTDIR} \
		-I${LESS_SRCDIR}/include \
		-DBINDIR=\"${BINDIR}\" \
		-DSYSDIR=\"/etc\"
