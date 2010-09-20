#	$eterna: Makefile,v 1.30 2010/07/11 00:34:27 mrg Exp $
#
# berkeley (netbsd) makefile.  see Makefile.boot for other systems.

# compile-time options are:
#	NO_DEBUG		/* don't include debugging support */
#	NO_USER_SUPPORT		/* don't support /~user requests */
#	NO_CGIBIN_SUPPORT	/* don't support cgi-bin requests */
#	NO_DIRINDEX_SUPPORT	/* don't support directory indexing */
#	NO_DAEMON_MODE		/* don't support daemon mode */
#	NO_DYNAMIC_CONTENT	/* don't support dynamic content updates */
#	NO_SSL_SUPPORT		/* don't support ssl (https) */
#	DO_HTPASSWD		/* support .htpasswd files */
#
# these are usually set via the "COPTS" variable, or some other method
# for setting CFLAGS relevant to your make, eg
#   % make COPTS="-DDO_HTPASSWD"

PROG?=	bozohttpd
MAN=	bozohttpd.8
SRCS=	bozohttpd.c ssl-bozo.c auth-bozo.c cgi-bozo.c daemon-bozo.c \
	tilde-luzah-bozo.c dir-index-bozo.c content-bozo.c
SRCS+=	main.c

WARNS?=	4

check:
	cd $(.CURDIR)/testsuite && $(MAKE) check

.if ${PROG} == "bozohttpd"
clean:
	cd $(.CURDIR)/testsuite && $(MAKE) clean
.endif

CRYPTOLIBS=	-lcrypto -lssl
CRYPTODEPS=	$(LIBCRYPTO) $(LIBSSL)

NROFF?=		nroff

# if not defining -DNO_SSL_SUPPORT
LDADD=	$(CRYPTOLIBS)
DPADD=	$(CRYPTODEPS)

PREHTMLFROB=	sed \
		    -e 's/&/\&amp;/' \
		    -e 's/</\&lt;/' \
		    -e 's/>/\&gt;/'

HTMLFROB=	sed \
		    -e 's/\([MC] "[^"]*\)<dd>$$/\1<b>"<\/b><dd>/' \
		    -e 's/'"''"'/\&rdquo;/' \
		    -e 's/""/\&ldquo;/' \
		    -e 's/<a href="\.\.\/html[^>]*>\(.*\)<\/a>/\1/'

TXTFROB=	col -b

bozohttpd.8.html: bozohttpd.8
	$(PREHTMLFROB) $> | $(NROFF) -mdoc2html | $(HTMLFROB) > $@

bozohttpd.8.txt: bozohttpd.8
	$(NROFF) -mdoc -Tascii $> | $(TXTFROB) > $@

CLEANFILES+=	bozohttpd.8.html bozohttpd.8.txt

.include <bsd.prog.mk>
