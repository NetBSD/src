#	$NetBSD: Makefile,v 1.1 2014/07/16 20:59:58 riastradh Exp $

.include <bsd.own.mk>

DRMSRCDIR=	../../dist/drm
RADEONSRCDIR=	${DRMSRCDIR}/radeon
MKREGTABLE=	../../radeon/mkregtable.awk

RADEON_REGS=
RADEON_REGS+=	rn50
RADEON_REGS+=	r100
RADEON_REGS+=	r200
RADEON_REGS+=	rv515
RADEON_REGS+=	r300
RADEON_REGS+=	r420
RADEON_REGS+=	rs600
RADEON_REGS+=	r600
RADEON_REGS+=	evergreen
RADEON_REGS+=	cayman

default-target: all

all: .PHONY

.for _regs_ in ${RADEON_REGS}
all: ${_regs_}_reg_safe.h
CLEANFILES+=	${_regs_}_reg_safe.h
${_regs_}_reg_safe.h: ${MKREGTABLE} ${RADEONSRCDIR}/reg_srcs/${_regs_}
	${TOOL_AWK} -f ${MKREGTABLE} < ${RADEONSRCDIR}/reg_srcs/${_regs_} \
	  > $@.tmp && \
	mv -f $@.tmp $@
.endfor

clean:
	rm -f ${CLEANFILES}
