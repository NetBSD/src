#	$NetBSD: Makefile,v 1.2.2.3 2000/10/18 18:27:54 tv Exp $

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

.SUFFIXES: .OM.c

.c.OM.c:
	cp ${RSRCDIR}/omapip/$*.c $@

CLEANFILES+= ${SRCS:M*.OM.c}

.include <bsd.prog.mk>
