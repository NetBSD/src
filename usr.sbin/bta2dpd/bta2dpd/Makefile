# $NetBSD: Makefile,v 1.2.2.2 2017/03/20 06:58:06 pgoyette Exp $

.include <bsd.own.mk>

WARNS?=		6

PROG=		bta2dpd
SRCS=		bta2dpd.c avdtp.c sbc_encode.c
MAN=		bta2dpd.8

CFLAGS+=	-I.
DPADD+=		${LIBBLUETOOTH} ${LIBEVENT}
LDADD+=		-lbluetooth -levent
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
