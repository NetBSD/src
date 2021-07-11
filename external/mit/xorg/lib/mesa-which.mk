#	$NetBSD: mesa-which.mk,v 1.2 2021/07/11 01:13:26 mrg Exp $

OLD_PREFIX=
.if ${EXTERNAL_MESALIB_DIR} == "MesaLib.old"
OLD_PREFIX=.old
.endif
