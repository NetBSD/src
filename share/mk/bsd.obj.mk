#	$NetBSD: bsd.obj.mk,v 1.25 2000/07/07 04:35:36 cgd Exp $

.if !target(__initialized_obj__)
__initialized_obj__:
.include <bsd.own.mk>
.endif

.if ${MKOBJ} == "no"
obj:
.else
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
.if defined(MAKEOBJDIRPREFIX)
__objdir:= ${MAKEOBJDIRPREFIX}${.CURDIR}
.else
__objdir:= ${MAKEOBJDIR}
.endif
# MAKEOBJDIR and MAKEOBJDIRPREFIX are env variables supported
# by make(1).  We simply mkdir -p the specified path.
# If that fails - we do a mkdir to get the appropriate error message
# before bailing out.
obj:
	@if ! test -d ${__objdir}; then \
		mkdir -p ${__objdir}; \
		if ! test -d ${__objdir}; then \
			mkdir ${__objdir}; exit 1; \
		fi; \
		echo "${.CURDIR} -> ${__objdir}"; \
	fi
.else
.if defined(OBJMACHINE)
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
		if [ -h $$here/${__objdir} ]; then \
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
		if test ! -d ${__objdir} || test -h ${__objdir}; then \
			echo "making $$dest" ; \
			rm -f ${__objdir}; \
			mkdir $$dest; \
		fi ; \
	fi;
.endif
.endif

print-objdir:
	@echo ${.OBJDIR}
