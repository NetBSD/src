#	$NetBSD: bsd.obj.mk,v 1.21 1999/08/21 00:41:41 sommerfeld Exp $

.if !target(__initialized_obj__)
__initialized_obj__:
.include <bsd.own.mk>
.endif

.if ${MKOBJ} == "no"
obj:
.else

.if defined(MAKEOBJDIR)
__objdir=	${MAKEOBJDIR}
.elif defined(OBJMACHINE)
__objdir=	obj.${MACHINE}
.else
__objdir=	obj
.endif

.if defined(USR_OBJMACHINE)
__usrobjdir=	${BSDOBJDIR}.${MACHINE}
__usrobjdirpf=	
.else
__usrobjdir=	${BSDOBJDIR}
.if defined(OBJMACHINE)
__usrobjdirpf=	.${MACHINE}
.else
__usrobjdirpf=
.endif
.endif

PAWD?=		/bin/pwd

obj:
	@cd ${.CURDIR}; rm -f ${__objdir} > /dev/null 2>&1 || true; \
	here=`${PAWD}`; subdir=$${here#${BSDSRCDIR}/}; \
	if test $$here != $$subdir ; then \
		dest=${__usrobjdir}/$$subdir${__usrobjdirpf} ; \
		echo "$$here/${__objdir} -> $$dest"; \
		rm -rf ${__objdir}; \
		ln -s $$dest ${__objdir}; \
		if test -d ${__usrobjdir} -a ! -d $$dest; then \
			mkdir -p $$dest; \
		else \
			true; \
		fi; \
	else \
		true ; \
		dest=$$here/${__objdir} ; \
		if test ! -d ${__objdir} ; then \
			echo "making $$dest" ; \
			mkdir $$dest; \
		fi ; \
	fi;
.endif

print-objdir:
	@echo ${.OBJDIR}
