#	$NetBSD: Makefile,v 1.6 2007/02/09 22:09:32 ad Exp $
#

WARNS=	2

.include <bsd.own.mk>

LIBPDIR=	${.CURDIR}/../libpthread

# Define PT_FIXEDSTACKSIZE_LG to set a fixed stacksize (same as libpthread)
#CPPFLAGS+=-DPT_FIXEDSTACKSIZE_LG=18
#CPPFLAGS+=	-DPTHREAD_SA

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

ARCHDIR=	${LIBPDIR}/arch/${ARCHSUBDIR}
.PATH:	${ARCHDIR}

CPPFLAGS+=	-I${ARCHDIR} -I${LIBPDIR} -I${.CURDIR}

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	${HOST_SH} ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

SRCS=	pthread_dbg.c 

INCS=	pthread_dbg.h
INCSDIR=/usr/include

.include <bsd.lib.mk>

.else

.include <bsd.man.mk>

.endif
