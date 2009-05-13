# $NetBSD: Makefile.inc,v 1.1.6.2 2009/05/13 17:18:10 jym Exp $

# MD libsa files shared among each SA program

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=	getchar.c putchar.c romcalls.S
