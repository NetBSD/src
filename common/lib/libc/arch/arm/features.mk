# $NetBSD: features.mk,v 1.1.6.1 2015/06/10 17:16:23 snj Exp $

.ifnmake obj
TESTFILE=${NETBSDSRCDIR}/common/lib/libc/arch/arm/features.c
FEAT_EABI!=if ${COMPILE.c} ${COPTS} -fsyntax-only -DEABI_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_LDREX!=if ${COMPILE.c} ${COPTS} -fsyntax-only -DLDREX_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_LDRD!=if ${COMPILE.c} ${COPTS} -fsyntax-only -DLDRD_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_THUMB2!=if ${COMPILE.c} ${COPTS} -fsyntax-only -DTHUMB2_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
.endif
