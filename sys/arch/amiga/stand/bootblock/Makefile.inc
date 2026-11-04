#       $NetBSD: Makefile.inc,v 1.6.202.1 2026/11/04 23:20:02 perseant Exp $

# XXX: turn off as-warning-as-error, various errors after txlt occur, eg:
#   Assembler messages:
#   Fatal error: invalid listing option `,'
NOGCCERROR=1

BINDIR?=/usr/mdec
