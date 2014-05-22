#	$wasabi: Makefile,v 1.2 2006/05/03 22:45:02 wrstuden Exp $

PROG=	iscsictl
BINDIR=	/sbin

SRCS=	iscsic_main.c iscsic_driverif.c iscsic_daemonif.c iscsic_parse.c \
	iscsic_test.c

CPPFLAGS+= -I${DESTDIR}/usr/include/dev/iscsi

CPPFLAGS+=	-I${.CURDIR}/../../sys/dev/iscsi
CPPFLAGS+=	-I${.CURDIR}/../../sys
CPPFLAGS+=	-I${.CURDIR}/../iscsid

MAN=	iscsictl.8

# CPPFLAGS+= -DISCSI_TEST_MODE -DISCSI_DEBUG

.include <bsd.prog.mk>
