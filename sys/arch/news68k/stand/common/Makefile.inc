# $NetBSD: Makefile.inc,v 1.1.4.2 2009/05/04 08:11:37 yamt Exp $

# MD libsa files shared among each SA program

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=	getchar.c putchar.c romcalls.S
