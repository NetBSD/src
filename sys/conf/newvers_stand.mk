#	$NetBSD: newvers_stand.mk,v 1.5 2024/04/05 22:28:20 christos Exp $

VERSIONFILE?=version
VERSIONMACHINE?=${MACHINE}
.if exists(${VERSIONFILE})

.if !make(depend)
SRCS+=		vers.c
.endif
CLEANFILES+=	vers.c

.if ${MKREPRO:Uno} == "yes"
.	if ${MKREPRO_TIMESTAMP:U0} != 0
VERSIONFLAGS+=-D ${MKREPRO_TIMESTAMP}
.	else
VERSIONFLAGS+=-d
.	endif
.endif

vers.c:	${VERSIONFILE} ${_NETBSD_VERSION_DEPENDS}
	${_MKTARGET_CREATE}
	TOOL_DATE=${TOOL_DATE} ${HOST_SH} ${S}/conf/newvers_stand.sh \
	    -m ${VERSIONMACHINE} ${VERSIONFLAGS} ${.ALLSRC:[1]} ${NEWVERSWHAT}

.endif
