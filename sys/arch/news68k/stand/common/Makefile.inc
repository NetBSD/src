# $NetBSD: Makefile.inc,v 1.1.2.2 2009/04/28 07:34:30 skrll Exp $

# MD libsa files shared among each SA program

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=	getchar.c putchar.c romcalls.S
