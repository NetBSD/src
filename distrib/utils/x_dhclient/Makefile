#	$NetBSD: Makefile,v 1.11 2001/12/12 00:05:12 tv Exp $

RSRCDIR=	${.CURDIR}/../../../dist/dhcp
.PATH: ${RSRCDIR}/client ${RSRCDIR}/common ${RSRCDIR}/dst ${RSRCDIR}/omapip

PROG=		dhclient
NOMAN=		# defined
SRCS=		dhclient.c clparse.c

CPPFLAGS+=	-DCLIENT_PATH='"PATH=/usr/bin:/usr/sbin:/bin:/sbin"' \
		-DSMALL -I${RSRCDIR} -I${RSRCDIR}/includes

#.include "Src-dst"
#DSTOBJS+=	${DSTSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
#SRCS+=		${DSTSRCS}

.include "Src-omapip"
OMOBJS+=	${OMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+=		${OMSRCS}

.include "Src-common"
COMOBJS+=	${COMSRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}
SRCS+=		${COMSRCS}

${DSTOBJS}:
	${CC} ${CFLAGS} ${CPPFLAGS} -DMINIRES_LIB -c ${.IMPSRC}

.SUFFIXES: .OM.c

.c.OM.c:
	cp ${RSRCDIR}/omapip/$*.c $@

CLEANFILES+=	${SRCS:M*.OM.c}

.include <bsd.prog.mk>
