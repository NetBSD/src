# 	$NetBSD: Makefile,v 1.8.2.1 2000/07/01 17:48:21 he Exp $
# Build a smaller sh (e.g. for boot media)
#
# Be very careful before attempting to "clean up" the ugly location
# of the bin/sh source directory. The problem is that crunchgen(1)
# doesn't run from this directory, thus creating a triple-indirect
# sh Makefile include and requiring the whole thing to work from
# different levels in the tree.
# 
# We could just use only the BSDSRCDIR assignment, but right now the use
# of BSDSRCDIR can be deleted here, and so I didn't want to depend on it
# in case someday we completely nuke the rarely used BSDSRCDIR.

.include <bsd.own.mk>

MKMAN=no

.for i in     ../../bin/sh		\
	   ../../../bin/sh		\
	../../../../bin/sh
.if exists(${.CURDIR:H}/$i/Makefile)
SRCDIR?= ${.CURDIR:H}/$i
.endif
.endfor

SRCDIR?= ${BSDSRCDIR}/bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf \
	${SRCDIR}/../test

.include "${SRCDIR}/Makefile"
