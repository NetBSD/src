.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_ext_route

SRCS=		npf_ext_route.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
