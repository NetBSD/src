.include <bsd.own.mk>

MKMAN=no
PROGS=srvtest cltest blacklistd
SRCS.srvtest = bl.c srvtest.c
SRCS.cltest = cltest.c
SRCS.blacklistd = bl.c blacklistd.c conf.c
LDADD.blacklistd += -lutil
LPADD.blacklistd += ${LIBUTIL}

.include <bsd.prog.mk>
