
.if !defined(ARM_MAX_ARCH)
.	if !empty(MACHINE_ARCH:Mearmv4*) || \
	    ${MACHINE_ARCH} == "armeb" || \
	    ${MACHINE_ARCH} == "arm"
ARM_MAX_ARCH=4
.	elif ${MACHINE_ARCH} == "earm" || \
	    ${MACHINE_ARCH} == "earmhf" || \
	    ${MACHINE_ARCH} == "earmeb" || \
	    ${MACHINE_ARCH} == "earmhfeb"
ARM_MAX_ARCH=5
.	elif !empty(MACHINE_ARCH:Mearmv6*)
ARM_MAX_ARCH=6
.	elif !empty(MACHINE_ARCH:Mearmv7*)
ARM_MAX_ARCH=7
.	else
ARM_MAX_ARCH=8
.	endif
CPPFLAGS += -D__ARM_MAX_ARCH__=${ARM_MAX_ARCH}
.endif
