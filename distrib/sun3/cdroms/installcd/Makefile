#	$NetBSD: Makefile,v 1.4 2019/09/23 13:42:35 christos Exp $
CDBASE=		sun3cd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true

# for PRINTOBJDIR
.include <bsd.own.mk>

CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sun3/miniroot && ${PRINTOBJDIR}
CDBOOTIMAGE=    ${CDBOOTIMAGEDIR}/miniroot.fs
SUN_BOOT_ARGS:= - - - ${CDBOOTIMAGE} ${CDBOOTIMAGE}

image_md_post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
		${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
		${CDIMAGE} ${SUN_BOOT_ARGS}
	

.include "${.CURDIR}/../../../common/Makefile.bootcd"
