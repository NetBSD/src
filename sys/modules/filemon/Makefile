# $NetBSD: Makefile,v 1.6 2019/12/23 06:45:38 maxv Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
IOCONF=	filemon.ioconf
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

COPTS.filemon_wrapper.c+=	${GCC_NO_CAST_FUNCTION_TYPE}

# Due to security concerns, we don't install the filemon module.  We
# do, however, want to keep building it to prevent bit-rot.  Define
# an empty install target for this.

kmodinstall:

.include <bsd.kmodule.mk>
