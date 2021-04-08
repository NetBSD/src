#	$NetBSD: Makefile,v 1.6 2021/04/08 06:52:42 rillig Exp $
#	$eterna: Makefile,v 1.1 2009/05/22 21:51:39 mrg Exp $

# build a 100% lean bozohttpd-small.c
PROG=	bozohttpd-small
NOMAN=	# defined
SRCS=	bozohttpd-small.c content-bozo-small.c ssl-bozo.c main.c

LEAN_IFDEF_FLAGS=	-UDEBUG -DNO_USER_SUPPORT \
			-DNO_CGIBIN_SUPPORT -DNO_DIRINDEX_SUPPORT \
			-DNO_DAEMON_MODE -DNO_DYNAMIC_CONTENT \
			-DNO_SSL_SUPPORT -UDO_HTPASSWD \
			-DNO_LUA_SUPPORT -DNO_BLOCKLIST_SUPPORT

CPPFLAGS=	-I$(.CURDIR)/.. ${LEAN_IFDEF_FLAGS}

bozohttpd-small.c: bozohttpd.c
	unifdef $(LEAN_IFDEF_FLAGS) < $> > $@.tmp;			\
	status=$$?;							\
	if [ $$status -ne 1 ]; then					\
		echo "unifdef returned $$status, expecting 1" 2>&1;	\
		false;							\
	fi
	mv -f $@.tmp $@

content-bozo-small.c: content-bozo.c
	unifdef $(LEAN_IFDEF_FLAGS) < $> > $@.tmp;			\
	status=$$?;							\
	if [ $$status -ne 1 ]; then					\
		echo "unifdef returned $$status, expecting 1" 2>&1;	\
		false;							\
	fi
	mv -f $@.tmp $@

CLEANFILES+=	content-bozo-small.c bozohttpd-small.c

.PATH: $(.CURDIR)/..

.include <bsd.prog.mk>
