#	$NetBSD: bsd.obj.mk,v 1.22 1999/12/04 02:44:07 sommerfeld Exp $

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
	@cd ${.CURDIR}; \
	here=`${PAWD}`; subdir=$${here#${BSDSRCDIR}/}; \
	if test $$here != $$subdir ; then \
		dest=${__usrobjdir}/$$subdir${__usrobjdirpf} ; \
		if [ -L $$here/${__objdir} ]; then \
			curtarg=`ls -ld $$here/${__objdir} | awk '{print $$NF}'` ; \
			if [ "$$curtarg" = "$$dest" ]; then \
				: ; \
			else \
				echo "$$here/${__objdir} -> $$dest"; \
				rm -rf ${__objdir}; \
				ln -s $$dest ${__objdir}; \
			fi; \
		else \
			echo "$$here/${__objdir} -> $$dest"; \
			rm -rf ${__objdir}; \
			ln -s $$dest ${__objdir}; \
		fi; \
		if test -d ${__usrobjdir} -a ! -d $$dest; then \
			mkdir -p $$dest; \
		else \
			true; \
		fi; \
	else \
		true ; \
		dest=$$here/${__objdir} ; \
		if test ! -d ${__objdir} || test -L ${__objdir}; then \
			echo "making $$dest" ; \
			rm -f ${__objdir}; \
			mkdir $$dest; \
		fi ; \
	fi;
.endif

print-objdir:
	@echo ${.OBJDIR}
