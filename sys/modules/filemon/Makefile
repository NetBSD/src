# $NetBSD: Makefile,v 1.2.22.1 2019/10/28 16:37:55 martin Exp $

.include "../Makefile.inc"

.PATH: ${S}/dev/filemon

KMOD = filemon
IOCONF=	filemon.ioconf
SRCS = filemon.c filemon_wrapper.c
NOMAN = no

# Due to security concerns, we don't install the filemon module.  We
# do, however, want to keep building it to prevent bit-rot.  Define
# an empty install target for this.

kmodinstall:

.include <bsd.kmodule.mk>
