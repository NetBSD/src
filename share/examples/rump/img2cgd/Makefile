PROG=	img2cgd
SRCS=	img2cgd.c
NOMAN=

.include <bsd.own.mk>

RUMP_ACTION=	#defined
CPPFLAGS+=	-DCGDCONFIG_AS_LIB
.include "${NETBSDSRCDIR}/sbin/cgdconfig/Makefile.cgdconfig"

DBG=	-g -O0
WARNS=	3

.include <bsd.prog.mk>
