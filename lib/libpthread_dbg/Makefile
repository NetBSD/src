#	$NetBSD: Makefile,v 1.1.2.2 2002/10/07 01:23:08 thorpej Exp $
#

WARNS=	2

.include <bsd.own.mk>

.if exists(${LIBPDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${LIBPDIR}/arch/${MACHINE_CPU}) 
ARCHSUBDIR=	${MACHINE_CPU}
.else
.BEGIN:
	@echo "no ARCHSUBDIR for ${MACHINE_ARCH}/${MACHINE_CPU}; skipping..."
.endif

.if defined(ARCHSUBDIR)

LIB=	pthread_dbg
LIBPDIR=	${.CURDIR}/../libpthread

ARCHDIR=	${LIBPDIR}/arch/${ARCHSUBDIR}
.PATH:	${ARCHDIR}

CPPFLAGS+=	-I${ARCHDIR} -I${LIBPDIR} -I${.CURDIR}

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	sh ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

SRCS=	pthread_dbg.c 

INCS=	pthread_dbg.h
INCSDIR=/usr/include

.endif

.include <bsd.lib.mk>

# DO NOT DELETE
