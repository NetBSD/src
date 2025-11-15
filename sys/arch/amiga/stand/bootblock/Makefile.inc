#       $NetBSD: Makefile.inc,v 1.7 2025/11/15 08:14:49 mrg Exp $

# XXX: turn off as-warning-as-error, various errors after txlt occur, eg:
#   Assembler messages:
#   Fatal error: invalid listing option `,'
NOGCCERROR=1

BINDIR?=/usr/mdec
