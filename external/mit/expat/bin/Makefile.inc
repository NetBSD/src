#	$NetBSD: Makefile.inc,v 1.1.2.2 2012/03/02 18:27:56 riz Exp $

BINDIR?=	/usr/bin

CPPFLAGS+=	-I${EXPATSRCDIR}/lib

LDADD+=	-L${EXPATOBJDIR.expat} -lexpat
DPADD+=	${EXPATLIB.expat}

.include "../Makefile.inc"
