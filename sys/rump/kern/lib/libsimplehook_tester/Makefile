

LIB=	rumpkern_simplehook_tester
COMMENT=Tester for simplehook

SRCS=	simplehook_tester.c

.ifdef RUMP_DEBUG
CPPFLAGS.simplehook_tester.c+=	-DSIMPLEHOOK_TESTER_DEBUG
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
