PROG=	img2cgd
SRCS=	img2cgd.c cgd_rumpops.c
NOMAN=

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/sbin/cgdconfig/Makefile.cgdconfig"

LDADD+=	-lrumpdev_cgd -lrumpdev_disk -lrumpdev -lrumpcrypto
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread

CPPFLAGS+=	-DCGDCONFIG_AS_LIB

DBG=-g -O0

.include <bsd.prog.mk>
