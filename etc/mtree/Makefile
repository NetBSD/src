#	$NetBSD: Makefile,v 1.6 2009/09/07 19:34:30 mrg Exp $

.include <bsd.own.mk>

.if ${MKX11} != "no"
. if ${X11FLAVOUR} == "Xorg"
NETBSD_DIST_X11_FILE=	NetBSD.dist.Xorg
. else
NETBSD_DIST_X11_FILE=	NetBSD.dist.XFree86
. endif
.else
NETBSD_DIST_X11_FILE=
.endif

NetBSD.dist:	NetBSD.dist.base ${NETBSD_DIST_X11_FILE}
	${TOOL_CAT} ${.CURDIR}/NetBSD.dist.base ${.CURDIR}/${NETBSD_DIST_X11_FILE} > ${.TARGET}

CONFIGFILES=	NetBSD.dist special
FILESDIR=	/etc/mtree

# distrib-dirs --
#	Populate $DESTDIR with directories needed by NetBSD
#
.if ${MKUNPRIVED} == "no"
TOOL_MTREE.unpriv=
.else
TOOL_MTREE.unpriv=	-W
.endif

distrib-dirs: .PHONY check_DESTDIR NetBSD.dist
.if !defined(DISTRIBUTION_DONE)						# {
# Create DESTDIR using HOST_INSTALL_DIR, not INSTALL_DIR, because
# INSTALL_DIR would want to write to the metalog, and it can't do that
# if the metalog is inside DESTDIR but DESTDIR doesn't yet exist.
	${HOST_INSTALL_DIR} -m 755 ${DESTDIR}
# Inkoke mtree to create the directories listed in NetBSD.dist;
# then invoke mtree again to register those directories in the metalog.
	${TOOL_MTREE} -def ${.OBJDIR}/NetBSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -U ${TOOL_MTREE.unpriv}
.if ${MKUNPRIVED} != "no"						# {
	${TOOL_MTREE} -def ${.OBJDIR}/NetBSD.dist -N ${.CURDIR}/.. \
	    -p ${DESTDIR}/ -C -k all | \
	    ${TOOL_AWK} '/ optional/ {next} // {print}' | ${METALOG.add}
.endif	# MKUNPRIVED							# }
.endif	# DISTRIBUTION_DONE						# }

.include <bsd.prog.mk>
