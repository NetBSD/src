#	$NetBSD: Makefile,v 1.3 2000/05/02 03:03:21 sommerfeld Exp $

# Note that the order here is picked to look much like that in the
# original Makefile.in in order to make it easier to visually compare
# when updating to a new version of Postfix.

# The first four of these are libraries and have to be built first.

SUBDIR=	util global dns masterlib master postfix sendmail error \
	pickup cleanup smtpd local trivial-rewrite qmgr smtp bounce pipe \
	showq postalias postcat postconf postdrop postkick postlock postlog \
	postmap postsuper man conf # postfix-script

# We don't want to build postfix-script (at least for now) because
# we're treating it as a configuration file.  So, we have
# conf/Makefile install a copy into share/examples/postfix, and only
# invoke postfix-script/Makefile from the make distribution target in
# src/etc/Makefile via make distribution in here).

# not built: smtpstone (unneeded), spawn (experimental),
# and html (unsure what to do with it (yet))

distribution:
	cd ${.CURDIR}/postfix-etc; ${MAKE} install


.include <bsd.subdir.mk>
