#	$NetBSD: Makefile.inc,v 1.2 1997/05/07 07:15:48 mycroft Exp $

WRTVID_BASE_DIR=${S}/arch/${MACHINE}/stand/wrtvid

WRTVID_DIR!=cd ${WRTVID_BASE_DIR}; \
     printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

WRTVID=${WRTVID_DIR}/wrtvid

$(WRTVID):	.NOTMAIN __always_make_WRTVID
	@echo making sure the wrtvid is up to date...
	@(cd ${WRTVID_BASE_DIR}; ${MAKE})

__always_make_WRTVID:	.NOTMAIN
