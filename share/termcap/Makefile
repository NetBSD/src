#	$NetBSD: Makefile,v 1.23.2.2 2002/05/30 23:59:37 tv Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

MAN=	termcap.5

CLEANFILES= termcap.db
realall: termcap.db
FILES=termcap.db termcap.src map3270
FILESNAME_termcap.src=termcap
FILESDIR=${BINDIR}/misc

.include <bsd.endian.mk>
.if ${TARGET_ENDIANNESS} == "1234"
TARGET_ENDIANNESS=	-l
.elif ${TARGET_ENDIANNESS} == "4321"
TARGET_ENDIANNESS=	-b
.else
TARGET_ENDIANNESS=
.endif

termcap.db: termcap.src
	${CAP_MKDB} ${TARGET_ENDIANNESS} -f termcap ${.ALLSRC}

.include <bsd.prog.mk>
