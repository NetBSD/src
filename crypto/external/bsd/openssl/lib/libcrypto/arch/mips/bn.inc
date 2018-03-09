.if empty(MACHINE_ARCH:M*eh}
.PATH.S: ${.PARSEDIR}

BN_SRCS = mips${"${COPTS:M*-mabi=64*}" == "":?:64}.S
.endif

.include "../../bn.inc"
