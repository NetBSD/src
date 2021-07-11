#	$NetBSD: mesa-which.mk,v 1.3 2021/07/11 20:52:06 mrg Exp $

OLD_SUFFIX=
.if ${EXTERNAL_MESALIB_DIR} == "MesaLib.old"
OLD_SUFFIX=.old
.endif
