#	$NetBSD: Makefile.inc,v 1.1.4.2 2012/04/17 00:04:38 yamt Exp $

BINDIR?=	/usr/bin

CPPFLAGS+=	-I${EXPATSRCDIR}/lib

LDADD+=	-L${EXPATOBJDIR.expat} -lexpat
DPADD+=	${EXPATLIB.expat}

.include "../Makefile.inc"
