# $NetBSD: Makefile.inc,v 1.3 2020/01/17 16:50:51 christos Exp $

.include "${NETBSDSRCDIR}/external/bsd/pkg_install/Makefile.inc"

.if !defined(HOSTLIB)
INSTALL_OBJDIR!= cd ${NETBSDSRCDIR}/tools/pkg_install/lib && ${PRINTOBJDIR}
LDFLAGS+=-L${INSTALL_OBJDIR}
LDADD+=-linstall
.endif

