#	$NetBSD: Makefile.inc,v 1.7 1997/05/07 07:15:43 mycroft Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -s -f-
