#	$NetBSD

PROG=	ntpdc
MAN=	ntpdc.8
SRCS=	ntpdc.c ntpdc_ops.c

SRCS+=	version.c
CLEANFILES+=version.c version.o .version

.include <bsd.prog.mk>

version.c: ${OBJS:S/version.o//} ${LIBNTP}/libntp.a
	sh ${.CURDIR}/../scripts/mkver ntpd
