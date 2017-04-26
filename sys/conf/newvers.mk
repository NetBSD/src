# $NetBSD: newvers.mk,v 1.1.4.1 2017/04/26 02:53:10 pgoyette Exp $

MKREPRO?=no

.if ${MKREPRO} == "yes"
.	if ${MKREPRO_TIMESTAMP:U0} != 0
_NVFLAGS=${NVFLAGS} -r ${MKREPRO_TIMESTAMP} -i ${KERNEL_BUILD:T} -m ${MACHINE}
.	else
_NVFLAGS=${NVFLAGS} -R
.	endif
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
