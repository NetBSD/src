#	$NetBSD: Makefile,v 1.1.2.3 2002/01/10 19:38:38 thorpej Exp $

S?=	${.CURDIR}/../../../../

LIB= riscos
NOPIC=# defined
NOPROFILE=# defined

CPPFLAGS= -I$S/lib/libsa ${RISCOSCPPFLAGS} ${RISCOSMISCCPPFLAGS}
#CPPFLAGS+= -DDISK_DEBUG
#CPPFLAGS+= -DNO_DISKLABEL
#CPPFLAGS+= -DSAVE_MEMORY

SRCS=	srt1.c devopen.c
SRCS+=	riscoscalls.S riscoscons.c riscoserrors.c riscosfile.c

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
