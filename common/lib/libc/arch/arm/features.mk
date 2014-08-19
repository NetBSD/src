# $NetBSD: features.mk,v 1.1.8.2 2014/08/19 23:45:12 tls Exp $

.ifnmake obj
TESTFILE=${NETBSDSRCDIR}/common/lib/libc/arch/arm/features.c
FEAT_EABI!=if ${COMPILE.c} -fsyntax-only -DEABI_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_LDREX!=if ${COMPILE.c} -fsyntax-only -DLDREX_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_LDRD!=if ${COMPILE.c} -fsyntax-only -DLDRD_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
FEAT_THUMB2!=if ${COMPILE.c} -fsyntax-only -DTHUMB2_TEST ${TESTFILE} >/dev/null 2>/dev/null; then echo yes; else echo no; fi
.endif
