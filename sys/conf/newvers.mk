# $NetBSD: newvers.mk,v 1.1.2.2 2015/09/22 12:05:56 skrll Exp $

MKREPRO?=no

.if ${MKREPRO} == "yes"
_NVFLAGS=${NVFLAGS} -r
.else
_NVFLAGS=${NVFLAGS}
.endif

.if !target(vers.o)
newvers: vers.o
vers.o: ${SYSTEM_OBJ:O} Makefile $S/conf/newvers.sh \
		$S/conf/osrelease.sh ${_NETBSD_VERSION_DEPENDS}
	${_MKMSG_CREATE} vers.c
	${HOST_SH} $S/conf/newvers.sh ${_NVFLAGS}
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c
	${COMPILE_CTFCONVERT}
.endif
