#	$NetBSD: bsd.crypto.mk,v 1.1 1999/07/11 20:17:01 thorpej Exp $

# XXX error out if SRCTOP isn't defined.

.include <bsd.own.mk>	# for configuration variables

# Compatibility with old Makefile variables
.if defined(EXPORTABLE_SYSTEM)
CRYPTOBASE=none
.endif

.if !defined(CRYPTOBASE)
.if exists(${SRCTOP}/crypto-us)
CRYPTOBASE=crypto-us
.elif exists(${SRCTOP}/crypto-intl)
CRYPTOBASE=crypto-intl
.else
.undef CRYPTOBASE
.endif # exists
.endif # CRYPTOBASE

.if defined(CRYPTOBASE) && (${CRYPTOBASE} != "none")
CRYPTOPATH=${SRCTOP}/${CRYPTOBASE}
.else
.undef CRYPTOPATH
.endif

.if defined(CRYPTOPATH) && !exists(${CRYPTOPATH})
.undef CRYPTOPATH
.endif
