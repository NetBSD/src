#	%W% (Berkeley) %G%
#
# PATCHES MAGIC                LEVEL   PATCH THAT GOT US HERE
# --------------------         -----   ----------------------
# CURRENT PATCH LEVEL:         1       90001 
# --------------------         -----   ----------------------
#
# 29 Sep 92	Terry Lambert		Set NO_REGEX flag
#

PROG=	fortune
MAN6=	fortune.0
CFLAGS+=-I${.CURDIR}/../strfile -DNO_REGEX
DPADD=	${COMPAT}
LDADD=	-lcompat

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>
