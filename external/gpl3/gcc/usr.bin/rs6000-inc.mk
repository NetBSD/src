#	$NetBSD: rs6000-inc.mk,v 1.1 2024/06/18 04:07:18 mrg Exp $

# makefile snippet to build the rs6000-vecdefs files, used by include and
# usr.bin.

.if ${GCC_MACHINE_ARCH} == "powerpc" || ${GCC_MACHINE_ARCH} == "powerpc64" # {

rs6000-gen-builtins.lo: ${HH} ${DIST}/gcc/config/rs6000/rs6000-gen-builtins.cc
rbtree.lo: ${HH} ${DIST}/gcc/config/rs6000/rbtree.cc
rs6000-gen-builtins: rs6000-gen-builtins.lo rbtree.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}}
rs6000-builtins.cc: rs6000-gen-builtins \
		    ${DIST}/gcc/config/rs6000/rs6000-builtins.def \
		    ${DIST}/gcc/config/rs6000/rs6000-overload.def
	${_MKTARGET_CREATE}
	./rs6000-gen-builtins \
		    ${DIST}/gcc/config/rs6000/rs6000-builtins.def \
		    ${DIST}/gcc/config/rs6000/rs6000-overload.def \
                rs6000-builtins.h rs6000-builtins.cc rs6000-vecdefines.h
rs6000-builtins.h rs6000-vecdefines.h: rs6000-builtins.cc

CLEANFILES+=	rs6000-builtins.h rs6000-builtins.cc rs6000-vecdefines.h
CLEANFILES+=	rs6000-gen-builtins rbtree.lo rs6000-gen-builtins.lo

.endif # }
