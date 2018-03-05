.PATH.S: ${.PARSEDIR}

BN_SRCS = mips${${MACHINE_ARCH:Mmips64*} == "":?:64}.S

.include "../../bn.inc"
