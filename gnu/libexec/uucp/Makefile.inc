LIBUNIX!=cd $(.CURDIR)/../libunix; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/libunix.a\n" | ${MAKE} -s -f-

LIBUUCONF!=cd $(.CURDIR)/../libuuconf; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/libuuconf.a\n" | ${MAKE} -s -f-

LIBUUCP!=cd $(.CURDIR)/../libuucp; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/libuucp.a\n" | ${MAKE} -s -f-

VERSION=	1.06.1
owner=		uucp
bindir=		/usr/bin
sbindir=	/usr/libexec/uucp

# The directory to look in for new style configuration files (when
# using HAVE_TAYLOR_CONFIG).
newconfigdir=	/etc/uucp

# The directory to look in for BNU (when using HAVE_BNU_CONFIG) or
# V2 (when using HAVE_V2_CONFIG) style configuration files.
oldconfigdir=	/etc/uucp

# this is sick, but much easier than putting in the individual programs'
# makefiles.
.if defined(PROG)
# use gnu malloc
LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a
.endif
