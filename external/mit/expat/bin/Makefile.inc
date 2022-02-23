#	$NetBSD: Makefile.inc,v 1.2 2022/02/23 19:07:35 martin Exp $

BINDIR?=	/usr/bin

CPPFLAGS+=	-I ${EXPATSRCDIR}/lib -I ${EXPATSRCDIR}/../lib/libexpat

LDADD+=	-L${EXPATOBJDIR.expat} -lexpat
DPADD+=	${EXPATLIB.expat}

.include "../Makefile.inc"
