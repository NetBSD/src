# $NetBSD: Makefile,v 1.2.16.1 2020/04/13 08:05:51 martin Exp $

.include <bsd.own.mk>

WARNS?=		6

PROG=		bta2dpd
SRCS=		bta2dpd.c avdtp.c sbc_encode.c
MAN=		bta2dpd.8

CFLAGS+=	-I.
DPADD+=		${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=		-lbluetooth -levent -lutil
CLEANFILES+=	sbc_crc.h sbc_coeffs.h

sbc_encode.c:	sbc_coeffs.h sbc_crc.h

COSDATA_GENOBJDIR!=  cd ${.CURDIR}/../cosdata-gen && ${PRINTOBJDIR}
SBC_CRC_GENOBJDIR!=  cd ${.CURDIR}/../sbc_crc-gen && ${PRINTOBJDIR}


sbc_coeffs.h: ${COSDATA_GENOBJDIR}/cosdata
	${.ALLSRC} > ${.TARGET}

sbc_crc.h: ${SBC_CRC_GENOBJDIR}/sbc_crc
	${.ALLSRC} > ${.TARGET}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.include <bsd.prog.mk>
