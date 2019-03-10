#	$NetBSD: mesa-which.mk,v 1.1 2019/03/10 02:29:52 mrg Exp $

OLD_PREFIX=
.if ${EXTERNAL_MESALIB_DIR} == "MesaLib.old"
OLD_PREFIX=.old
EXTRA_DRI_DIRS=dri7
.endif
