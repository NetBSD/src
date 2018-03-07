.PATH.S: ${.PARSEDIR}

BN_SRCS = mips${"${COPTS:M*-mabi=64*}" == "":?:64}.S

.include "../../bn.inc"
