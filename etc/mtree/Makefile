#	$NetBSD: Makefile,v 1.14 2009/12/18 04:12:36 uebayasi Exp $

.include <bsd.own.mk>

.if ${MKX11} != "no"
. if ${X11FLAVOUR} == "Xorg"
EXTRA_DIST_FILES=	${.CURDIR}/NetBSD.dist.Xorg
. else
EXTRA_DIST_FILES=	${.CURDIR}/NetBSD.dist.XFree86
. endif
.endif

.if exists(NetBSD.dist.${MACHINE_ARCH})
EXTRA_DIST_FILES+=	${.CURDIR}/NetBSD.dist.${MACHINE_ARCH}
.endif

.if ${MKEXTSRC} != "no"
EXTRA_DIST_FILES+=	${.CURDIR}/NetBSD.dist.extsrc
.endif

NetBSD.dist:	NetBSD.dist.tmp
	cmp -s NetBSD.dist.tmp NetBSD.dist || { \
		echo "Updating NetBSD.dist"; \
		mv NetBSD.dist.tmp NetBSD.dist; \
	}
NetBSD.dist.tmp::
	${TOOL_CAT} ${.CURDIR}/NetBSD.dist.base ${EXTRA_DIST_FILES} > \
	    ${.TARGET}

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

CLEANFILES+=	NetBSD.dist

.include <bsd.prog.mk>
