#	$NetBSD: Makefile.inc,v 1.1 2004/05/12 16:56:43 cl Exp $
#
#	etc.xen/Makefile.inc -- xen-specific etc Makefile targets
#

KERNEL_SETS=		

EXTRA_KERNELS=		GENERIC XEN

BUILD_KERNELS=		

INSTALLATION_DIRS+=	installation/xen12load

# xen specific distrib stuff
snap_md_post:
	 cd ${KERNSRCDIR}/arch/xen/stand/xen12load && ${MAKE} release
	 ${MAKESUMS} -t \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/xen12load '*.*'
