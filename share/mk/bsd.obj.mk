#	$NetBSD: bsd.obj.mk,v 1.41 2003/07/18 04:06:18 lukem Exp $

.if !target(__initialized_obj__)
__initialized_obj__:
.include <bsd.own.mk>

__curdir:=	${.CURDIR}

.if ${MKOBJ} == "no"
obj:
.else
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
.if defined(MAKEOBJDIRPREFIX)
__objdir:= ${MAKEOBJDIRPREFIX}${__curdir}
.else
__objdir:= ${MAKEOBJDIR}
.endif
# MAKEOBJDIR and MAKEOBJDIRPREFIX are env variables supported
# by make(1).  We simply mkdir -p the specified path.
# If that fails - we do a mkdir to get the appropriate error message
# before bailing out.
obj:
.if defined(MAKEOBJDIRPREFIX)
	@if [ ! -d ${MAKEOBJDIRPREFIX} ]; then \
		echo "MAKEOBJDIRPREFIX ${MAKEOBJDIRPREFIX} does not exist, bailing..."; \
		exit 1; \
	fi;
.endif
	@if [ ! -d ${__objdir} ]; then \
		mkdir -p ${__objdir}; \
		if [ ! -d ${__objdir} ]; then \
			mkdir ${__objdir}; exit 1; \
		fi; \
		echo "${__curdir} -> ${__objdir}"; \
	fi
.else
PAWD?=		/bin/pwd

__objdir=	obj${OBJMACHINE:D.${MACHINE}}

__usrobjdir=	${BSDOBJDIR}${USR_OBJMACHINE:D.${MACHINE}}
__usrobjdirpf=	${USR_OBJMACHINE:D:U${OBJMACHINE:D.${MACHINE}}}

.if defined(BUILDID)
__objdir:=	${__objdir}.${BUILDID}
__usrobjdirpf:=	${__usrobjdirpf}.${BUILDID}
__need_objdir_target=yes
.endif

.if defined(OBJHOSTMACHINE) && (${MKHOSTOBJ:Uno} != "no")
# In case .CURDIR has been twiddled by a .mk file and is now relative,
# make it absolute again.
.if ${__curdir:M/*} == ""
__curdir!=	cd ${__curdir} && ${PAWD}
.endif

__objdir:=	${__objdir}.${HOST_OSTYPE}
__usrobjdirpf:=	${__usrobjdirpf}.${HOST_OSTYPE}
__need_objdir_target=yes
.endif

.if defined(__need_objdir_target)
.OBJDIR:	${__objdir}
.endif

obj:
	@cd ${__curdir}; \
	here=`${PAWD}`/; subdir=$${here#${BSDSRCDIR}/}; \
	if [ "$$here" != "$$subdir" ]; then \
		if [ ! -d ${__usrobjdir} ]; then \
			echo "BSDOBJDIR ${__usrobjdir} does not exist, bailing..."; \
			exit 1; \
		fi; \
		subdir=$${subdir%/}; \
		dest=${__usrobjdir}/$$subdir${__usrobjdirpf}; \
		if [ -h $${here}${__objdir} ]; then \
			curtarg=`ls -ld $${here}${__objdir} | awk '{print $$NF}'` ; \
			if [ "$$curtarg" = "$$dest" ]; then \
				: ; \
			else \
				echo "$${here}${__objdir} -> $$dest"; \
				rm -rf ${__objdir}; \
				ln -s $$dest ${__objdir}; \
			fi; \
		else \
			echo "$${here}${__objdir} -> $$dest"; \
			rm -rf ${__objdir}; \
			ln -s $$dest ${__objdir}; \
		fi; \
		if [ ! -d $$dest ]; then \
			mkdir -p $$dest; \
		else \
			true; \
		fi; \
	else \
		true ; \
		dest=$${here}${__objdir} ; \
		if [ ! -d ${__objdir} ] || [ -h ${__objdir} ]; then \
			echo "making $$dest" ; \
			rm -f ${__objdir}; \
			mkdir $$dest; \
		fi ; \
	fi;
.endif
.endif

print-objdir:
	@echo ${.OBJDIR}
.endif
