#	$NetBSD: Makefile,v 1.2.2.2 2000/07/24 15:25:00 itojun Exp $

PROG=	dhclient
MKMAN=	no
CPPFLAGS+=	-DCLIENT_PATH='"PATH=/usr/bin:/usr/sbin:/bin:/sbin"'

LDSTATIC?=	-static

RSRCDIR= ${.CURDIR}/../../../usr.sbin/dhcp
.PATH: ${RSRCDIR}/client ${RSRCDIR}/common ${RSRCDIR}/minires ${RSRCDIR}/omapip
.NOPATH: ${PROG}

SRCS=	dhclient.c clparse.c

.include "Src-minires"
MINIOBJS+= ${MINISRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${MINISRCS}

.include "Src-omapip"
OMOBJS+= ${OMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${OMSRCS}

.include "Src-common"
COMOBJS+= ${COMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${COMSRCS}

CPPFLAGS+=-DSMALL -I${RSRCDIR} -I${RSRCDIR}/includes
COPTS+=	-Os

all: ${PROG}

${MINIOBJS}:
	${CC} ${CFLAGS} ${CPPFLAGS} -DMINIRES_LIB -c ${.IMPSRC}

OMalloc.c: ${RSRCDIR}/omapip/alloc.c
	cp ${RSRCDIR}/omapip/alloc.c OMalloc.c

OMdispatch.c: ${RSRCDIR}/omapip/dispatch.c
	cp ${RSRCDIR}/omapip/dispatch.c OMdispatch.c

OMsupport.c: ${RSRCDIR}/omapip/support.c
	cp ${RSRCDIR}/omapip/support.c OMsupport.c

CLEANFILES+= OMalloc.c OMdispatch.c OMsupport.c

.include <bsd.prog.mk>
