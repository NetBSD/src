#	$NetBSD: mesa-which.mk,v 1.1.2.2 2019/06/10 22:02:37 christos Exp $

OLD_PREFIX=
.if ${EXTERNAL_MESALIB_DIR} == "MesaLib.old"
OLD_PREFIX=.old
EXTRA_DRI_DIRS=dri7
.endif
