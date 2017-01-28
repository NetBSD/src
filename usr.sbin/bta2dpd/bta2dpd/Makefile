# $NetBSD: Makefile,v 1.1 2017/01/28 16:55:54 nat Exp $

WARNS?=		6

PROG=		bta2dpd
SRCS=		bta2dpd.c avdtp.c sbc_encode.c
MAN=		bta2dpd.8

CFLAGS+=	-I.
DPADD+=		${LIBBLUETOOTH} ${LIBEVENT}
LDADD+=		-lbluetooth -levent
CLEANFILES+=	sbc_crc.h sbc_coeffs.h

sbc_encode.c:	sbc_coeffs.h sbc_crc.h
sbc_coeffs.h:
	../cosdata-gen/cosdata > sbc_coeffs.h
sbc_crc.h:
	../sbc_crc-gen/sbc_crc > sbc_crc.h

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.include <bsd.prog.mk>
