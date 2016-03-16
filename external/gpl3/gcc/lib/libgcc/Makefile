#	$NetBSD: Makefile,v 1.4 2016/03/16 05:44:16 mrg Exp $

.include <bsd.own.mk>

# XXXGCC5
#
# XXX libgcc (and libobjc) need insn-modes.h.
# XXX for now, when doing make obj, create these needed objdirs
# XXX in the obj phase, so that both normal and compat builds
# XXX get here

.if make(obj)
SUBDIR+=	../../usr.bin/host-libiberty .WAIT
SUBDIR+=	../../usr.bin/backend .WAIT
.endif
.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	${MAKEDIRTARGET} ../../usr.bin/host-libiberty all
	${MAKEDIRTARGET} ../../usr.bin/backend insn-modes.h
.endif

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
SUBDIR+=	libgcc_eh libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
