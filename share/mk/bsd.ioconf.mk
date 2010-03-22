#	$NetBSD: bsd.ioconf.mk,v 1.2 2010/03/22 14:42:01 pooka Exp $
#

# If IOCONF is defined, autocreate ioconf.[ch] and locators.h.
# This is useful mainly for devices.
.if !empty(IOCONF)

# discourage direct inclusion.  bsd.ioconf.mk will hopefully go away
# when the kernel build procedures are unified.
.if defined(_BSD_IOCONF_MK_USER_)

ioconf.c: ${IOCONF}
	${TOOL_CONFIG} -b ${.OBJDIR} -s ${S} ${.CURDIR}/${IOCONF}
	# config doesn't change the files if they're unchanged.  however,
	# here we want to satisfy our make dependency, so force a
	# timestamp update
	touch ioconf.c ioconf.h locators.h

.else # _BSD_IOCONF_MK_USER_

ioconf.c:
	@echo do not include bsd.ioconf.mk directly
	@false

.endif # _BSD_IOCONF_MK_USER_

locators.h: ioconf.c
ioconf.h: ioconf.c

CLEANFILES+= ioconf.c ioconf.h locators.h
DPSRCS+= ioconf.c ioconf.h locators.h
.endif
