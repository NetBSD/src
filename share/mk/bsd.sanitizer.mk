#	$NetBSD: bsd.sanitizer.mk,v 1.1 2019/08/27 22:48:54 kamil Exp $

.if !defined(_BSD_SANITIZER_MK_)
_BSD_SANITIZER_MK_=1

.if ${MKSANITIZER:Uno} == "yes"
CPPFLAGS+=	-D_REENTRANT
.endif

# Rename the local function definitions to not conflict with libc/rt/pthread/m.
.if ${MKSANITIZER:Uno} == "yes" && defined(SANITIZER_RENAME_SYMBOL)
.	for _symbol in ${SANITIZER_RENAME_SYMBOL}
CPPFLAGS+=	-D${_symbol}=__mksanitizer_${_symbol}
.	endfor
.endif

.if ${MKSANITIZER:Uno} == "yes" && defined(SANITIZER_RENAME_CLASSES)
.	for _class in ${SANITIZER_RENAME_CLASSES}
.		for _file in ${SANITIZER_RENAME_FILES.${_class}}
.			for _symbol in ${SANITIZER_RENAME_SYMBOL.${_class}}
COPTS.${_file}+=	-D${_symbol}=__mksanitizer_${_symbol}
.			endfor
.		endfor
.	endfor
.endif

.endif  # !defined(_BSD_SANITIZER_MK_)
