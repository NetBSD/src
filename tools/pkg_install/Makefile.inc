# $NetBSD: Makefile.inc,v 1.1.46.1 2020/04/08 14:09:13 martin Exp $

.include "${NETBSDSRCDIR}/external/bsd/pkg_install/Makefile.inc"

.if !defined(HOSTLIB)
INSTALL_OBJDIR!= cd ${NETBSDSRCDIR}/tools/pkg_install/lib && ${PRINTOBJDIR}
LDFLAGS+=-L${INSTALL_OBJDIR}
LDADD+=-linstall
.endif

