# $NetBSD: Makefile.inc,v 1.2 2020/01/12 21:27:43 christos Exp $

.include "${NETBSDSRCDIR}/external/bsd/pkg_install/Makefile.inc"

INSTALL_OBJDIR!= cd ${NETBSDSRCDIR}/tools/pkg_install/lib && ${PRINTOBJDIR}
LDFLAGS+=-L${INSTALL_OBJDIR}
LDADD+=-linstall

