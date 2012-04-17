#	$NetBSD: Makefile,v 1.1.38.1 2012/04/17 00:02:47 yamt Exp $
CDBASE=		sparccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

# for PRINTOBJDIR
.include <bsd.own.mk>

CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=    ${CDBOOTIMAGEDIR}/boot.fs
SUN_BOOT_ARGS:= ${CDBOOTIMAGE} ${CDBOOTIMAGE} ${CDBOOTIMAGE} - -
image_md_post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
		${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
		${CDIMAGE} ${SUN_BOOT_ARGS}
	

.include "${.CURDIR}/../../../common/Makefile.bootcd"
