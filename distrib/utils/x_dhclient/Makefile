#	$NetBSD: Makefile,v 1.7 2001/04/06 04:09:55 thorpej Exp $

PROG=	dhclient
MKMAN=	no
CPPFLAGS+=	-DCLIENT_PATH='"PATH=/usr/bin:/usr/sbin:/bin:/sbin"'

LDSTATIC?=	-static

RSRCDIR= ${.CURDIR}/../../../usr.sbin/dhcp
.PATH: ${RSRCDIR}/client ${RSRCDIR}/common ${RSRCDIR}/dst ${RSRCDIR}/omapip
.NOPATH: ${PROG}

SRCS=	dhclient.c clparse.c

.include "Src-dst"
DSTOBJS+= ${DSTSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${DSTSRCS}

.include "Src-omapip"
OMOBJS+= ${OMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${OMSRCS}

.include "Src-common"
COMOBJS+= ${COMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+= ${COMSRCS}

CPPFLAGS+=-DSMALL -I${RSRCDIR} -I${RSRCDIR}/includes
.if ${MACHINE} != "sparc64"
COPTS+=	-Os
.endif

all: ${PROG}

${DSTOBJS}:
	${CC} ${CFLAGS} ${CPPFLAGS} -DMINIRES_LIB -c ${.IMPSRC}

.SUFFIXES: .OM.c

.c.OM.c:
	cp ${RSRCDIR}/omapip/$*.c $@

CLEANFILES+= ${SRCS:M*.OM.c}

.include <bsd.prog.mk>
