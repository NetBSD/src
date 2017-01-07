#	$NetBSD: Makefile,v 1.11.8.1 2017/01/07 08:56:04 pgoyette Exp $
#

WARNS?=		5

.include <bsd.own.mk>

LIBPDIR=	${.CURDIR}/../libpthread

.if defined(PTHREAD_MACHINE_ARCH) && !empty(PTHREAD_MACHINE_ARCH) && \
    exists(${LIBPDIR}/arch/${PTHREAD_MACHINE_ARCH})
ARCHSUBDIR=	${PTHREAD_MACHINE_ARCH}
.elif exists(${LIBPDIR}/arch/${MACHINE_ARCH})
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
	${TOOL_GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${PROF} \
		${GENASSYM_CPPFLAGS} < ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

SRCS=	pthread_dbg.c 

CPPFLAGS+=	-D__LIBPTHREAD_SOURCE__

INCS=	pthread_dbg.h
INCSDIR=/usr/include

MAN+=	pthread_dbg.3
MAN+=	td_open.3
MAN+=	td_map_pth2thr.3
MAN+=	td_thr_getname.3
MAN+=	td_thr_info.3
MAN+=	td_thr_iter.3

MLINKS+=	td_open.3 td_close.3

.include <bsd.lib.mk>

.else

.include <bsd.man.mk>

.endif
