#	$NetBSD: Makefile.inc,v 1.10 1999/01/15 13:31:15 bouyer Exp $

KMINCLUDES=
KMSRCS=	divrem.m4 \
	bcopy.S bzero.S ffs.S \
	memcpy.S memmove.S
#KMSRCS=	bcmp.S strcat.S strcmp.S strcpy.S strlen.S

# `source' files built from m4 source
SRCS+=	__divqu.S __divq.S __divlu.S __divl.S
SRCS+=	__remqu.S __remq.S __remlu.S __reml.S
CLEANFILES+=	__divqu.S __divq.S __divlu.S __divl.S
CLEANFILES+=	__remqu.S __remq.S __remlu.S __reml.S

__divqu.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__divqu -DOP=div -DS=false -DWORDSIZE=64 \
		${.ALLSRC} > ${.TARGET}

__divq.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__divq -DOP=div -DS=true -DWORDSIZE=64 \
		${.ALLSRC} > ${.TARGET}

__divlu.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__divlu -DOP=div -DS=false -DWORDSIZE=32 \
		${.ALLSRC} > ${.TARGET}

__divl.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__divl -DOP=div -DS=true -DWORDSIZE=32 \
		${.ALLSRC} > ${.TARGET}

__remqu.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__remqu -DOP=rem -DS=false -DWORDSIZE=64 \
		${.ALLSRC} > ${.TARGET}

__remq.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__remq -DOP=rem -DS=true -DWORDSIZE=64 \
		${.ALLSRC} > ${.TARGET}

__remlu.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__remlu -DOP=rem -DS=false -DWORDSIZE=32 \
		${.ALLSRC} > ${.TARGET}

__reml.S: ${.CURDIR}/arch/alpha/gen/divrem.m4
	m4 -DNAME=__reml -DOP=rem -DS=true -DWORDSIZE=32 \
		${.ALLSRC} > ${.TARGET}
