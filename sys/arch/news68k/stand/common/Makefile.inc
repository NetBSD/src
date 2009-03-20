# $NetBSD: Makefile.inc,v 1.1 2009/03/20 13:59:49 tsutsui Exp $

# MD libsa files shared among each SA program

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=	getchar.c putchar.c romcalls.S
