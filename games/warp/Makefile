#	$NetBSD: Makefile,v 1.5 2020/11/11 20:04:02 christos Exp $

.include <bsd.own.mk>

PROG=	warp
#DBG=-g

SRCS=	bang.c init.c intrp.c move.c object.c play.c score.c sig.c term.c \
	them.c us.c util.c version.c warp.c weapon.c

DPADD=	${LIBTERMLIB} ${LIBM}
LDADD=	-ltermlib -lm

CPPFLAGS+=-DHAVETERMLIB

CLEANFILES+=sm sm.lo smap.0 smap.1 smap.2 smap.3 smap.4 smap.5 smap.6 smap.7
CLEANFILES+=warp.6
HIDEGAME=hidegame
SETGIDGAME=yes
MAN=	warp.6

warp.6:
	${_MKTARGET_CREATE}
	cat ${.CURDIR}/warp.man > ${.TARGET}

DPSRCS+=smap.0 smap.1 smap.2 smap.3 smap.4 smap.5 smap.6 smap.7

smap.0: smp.0 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.0 > ${.TARGET}

smap.1: smp.1 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.1 > ${.TARGET}

smap.2: smp.2 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.2 > ${.TARGET}

smap.3: smp.3 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.3 > ${.TARGET}

smap.4: smp.4 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.4 > ${.TARGET}

smap.5: smp.5 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.5 > ${.TARGET}

smap.6: smp.6 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.6 > ${.TARGET}

smap.7: smp.7 sm
	${_MKTARGET_CREATE}
	./sm < ${.CURDIR}/smp.7 > ${.TARGET}

sm: sm.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

.if ${MKSHARE} != "no"
FILES=smap.0 smap.1 smap.2 smap.3 smap.4 smap.5 smap.6 smap.7
FILES+=warp.doc warp.news README
FILESDIR=/usr/share/games/warp
.endif

.include <bsd.prog.mk>
