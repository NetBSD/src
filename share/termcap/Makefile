#	$NetBSD: Makefile,v 1.26 2003/05/18 08:20:34 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

.include <bsd.endian.mk>
.if ${TARGET_ENDIANNESS} == "1234"
CAP_MKDB_ENDIAN=	-l
.elif ${TARGET_ENDIANNESS} == "4321"
CAP_MKDB_ENDIAN=	-b
.else
CAP_MKDB_ENDIAN=
.endif

termcap.db: termcap.src
	${CAP_MKDB} ${CAP_MKDB_ENDIAN} -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
