#	$NetBSD: bsd.syspkg.mk,v 1.2 2002/03/29 20:56:28 jwise Exp $
#
#	This file is derived from:
#
#	NetBSD: bsd.pkg.mk,v 1.636 2001/01/05 18:03:14 jlam Exp
#
#	Plus many fixes and improvements from later revisions.
#	
#	However, it has been pared down to a minimum of targets, and
#	adapted to the standard bsd.*.mk world order.
#
#	Portions of this code are copyright (c) 2000-2002 Jim Wise
#

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.MAIN:		all
.endif

PREFIX:=		${DESTDIR}/${PREFIX}

OPSYS=			NetBSD
OS_VERSION!=		sh ${.PARSEDIR}/../../../sys/conf/osrelease.sh

# keep bsd.own.mk from generating an install: target.
NEED_OWN_INSTALL_TARGET=no

##### Some overrides of defaults below on a per-OS basis.

DEINSTALLDEPENDS?=	NO	# add -R to pkg_delete

PKGSRCDIR?=		${.CURDIR:C|/[^/]*/[^/]*$||}
PKGVERSION?=		${OS_VERSION}.${TINY_VERSION}
PKGWILDCARD?=		${PKGBASE}-[0-9]*

# For system packages, we set this here, as the version is auto-generated.
PKGNAME?=		${PKGBASE}-${PKGVERSION}

PACKAGES?=		${PKGSRCDIR}/../packages
PKGDIR?=		${.CURDIR}

# Don't change these!!!  These names are built into the _TARGET_USE macro,
# there is no way to refer to them cleanly from within the macro AFAIK.
PACKAGE_COOKIE=		${WRKDIR}/.package_done

# Miscellaneous overridable commands:
SHCOMMENT?=		${ECHO_MSG} >/dev/null '***'

MAKE_ENV+=		PREFIX=${PREFIX}

TOUCH_FLAGS?=		-f

# Debugging levels for this file, dependent on PKG_DEBUG_LEVEL definition
# 0 == normal, default, quiet operation
# 1 == all shell commands echoed before invocation
# 2 == shell "set -x" operation
PKG_DEBUG_LEVEL?=	0
_PKG_SILENT=		@
_PKG_DEBUG=		

.if ${PKG_DEBUG_LEVEL} > 0
_PKG_SILENT=	
.endif

.if ${PKG_DEBUG_LEVEL} > 1
_PKG_DEBUG=		set -x;
.endif

# In point of fact, this will often be ./obj, as per bsd.obj.mk
WRKDIR?=		.

COMMENT?=		${PKGDIR}/COMMENT
DESCR=			${PKGDIR}/DESCR
PLIST=			${WRKDIR}/PLIST
PLIST_SRC?=		${PKGDIR}/PLIST


# Set PKG_INSTALL_FILE to be the name of any INSTALL file
.if !defined(PKG_INSTALL_FILE) && exists(${PKGDIR}/INSTALL)
PKG_INSTALL_FILE=		${PKGDIR}/INSTALL
.endif

# Set PKG_DEINSTALL_FILE to be the name of any DEINSTALL file
.if !defined(PKG_DEINSTALL_FILE) && exists(${PKGDIR}/DEINSTALL)
PKG_DEINSTALL_FILE=		${PKGDIR}/DEINSTALL
.endif

# Set MESSAGE_FILE to be the name of any MESSAGE file
.if !defined(MESSAGE_FILE) && exists(${PKGDIR}/MESSAGE)
MESSAGE_FILE=		${PKGDIR}/MESSAGE
.endif

AWK?=		/usr/bin/awk
CAT?=		/bin/cat
CP?=		/bin/cp
DC?=		/usr/bin/dc
ECHO?=		echo				# Shell builtin
FALSE?=		false				# Shell builtin
FIND?=		/usr/bin/find
GREP?=		/usr/bin/grep
IDENT?=		/usr/bin/ident
LN?=		/bin/ln
LS?=		/bin/ls
MKDIR?=		/bin/mkdir -p
MV?=		/bin/mv
PKG_TOOLS_BIN?= /usr/sbin
RM?=		/bin/rm
SED?=		/usr/bin/sed
SETENV?=	/usr/bin/env
SH?=		/bin/sh
TEST?=		test				# Shell builtin
TOUCH?=		/usr/bin/touch
TRUE?=		true				# Shell builtin

PKG_ADD?=	PKG_DBDIR=${PKG_DBDIR} ${PKG_TOOLS_BIN}/pkg_add
PKG_ADMIN?=	PKG_DBDIR=${PKG_DBDIR} ${PKG_TOOLS_BIN}/pkg_admin
PKG_CREATE?=	PKG_DBDIR=${PKG_DBDIR} ${PKG_TOOLS_BIN}/pkg_create
PKG_DELETE?=	PKG_DBDIR=${PKG_DBDIR} ${PKG_TOOLS_BIN}/pkg_delete
PKG_INFO?=	PKG_DBDIR=${PKG_DBDIR} ${PKG_TOOLS_BIN}/pkg_info

.if !defined(PKGTOOLS_VERSION)
.if !exists(${IDENT})
PKGTOOLS_VERSION=${PKGTOOLS_REQD}
.else
PKGTOOLS_VERSION!=${IDENT} ${PKG_TOOLS_BIN}/pkg_add ${PKG_TOOLS_BIN}/pkg_admin ${PKG_TOOLS_BIN}/pkg_create ${PKG_TOOLS_BIN}/pkg_delete ${PKG_TOOLS_BIN}/pkg_info | ${AWK} 'BEGIN {n = 0;}; $$1 ~ /\$$NetBSD/ && $$2 !~ /^crt0/ {gsub("/", "", $$4); if ($$4 > n) {n = $$4;}}; END {print n;}'
.endif
.endif
MAKEFLAGS+=	PKGTOOLS_VERSION="${PKGTOOLS_VERSION}"

# Latest version of pkgtools required for this file.
PKGTOOLS_REQD=		20000202

# Check that we're using up-to-date pkg_* tools with this file.
uptodate-pkgtools:
	${_PKG_SILENT}${_PKG_DEBUG}					\
	if [ ${PKGTOOLS_VERSION} -lt ${PKGTOOLS_REQD} ]; then		\
		case ${PKGNAME} in					\
		pkg_install-*)						\
			;;						\
		*)							\
			${ECHO} "Your package tools need to be updated to ${PKGTOOLS_REQD:C|(....)(..)(..)|\1/\2/\3|} versions."; \
			${ECHO} "The installed package tools were last updated on ${PKGTOOLS_VERSION:C|(....)(..)(..)|\1/\2/\3|}."; \
			${FALSE} ;;					\
		esac							\
	fi

# Files to create for versioning and build information
BUILD_VERSION_FILE=	${WRKDIR}/.build_version
BUILD_INFO_FILE=	${WRKDIR}/.build_info

# Files containing size of pkg w/o and w/ all required pkgs
SIZE_PKG_FILE=		${WRKDIR}/.SizePkg
SIZE_ALL_FILE=		${WRKDIR}/.SizeAll

.ifndef PKG_ARGS_COMMON
PKG_ARGS_COMMON=	-v -c ${COMMENT} -d ${DESCR} -f ${PLIST} -l
PKG_ARGS_COMMON+=	-b ${BUILD_VERSION_FILE} -B ${BUILD_INFO_FILE}
PKG_ARGS_COMMON+=	-s ${SIZE_PKG_FILE} -S ${SIZE_ALL_FILE}
PKG_ARGS_COMMON+=	-P "`${MAKE} ${MAKEFLAGS} run-depends-list PACKAGE_DEPENDS_QUICK=true|sort -u`"
.ifdef CONFLICTS
PKG_ARGS_COMMON+=	-C "${CONFLICTS}"
.endif
.ifdef PKG_INSTALL_FILE
PKG_ARGS_COMMON+=	-i ${PKG_INSTALL_FILE}
.endif
.ifdef PKG_DEINSTALL_FILE
PKG_ARGS_COMMON+=	-k ${PKG_DEINSTALL_FILE}
.endif
.ifdef MESSAGE_FILE
PKG_ARGS_COMMON+=	-D ${MESSAGE_FILE}
.endif

PKG_ARGS_INSTALL=	-p ${PREFIX} ${PKG_ARGS_COMMON}
PKG_ARGS_BINPKG=	-p ${PREFIX:C/^${DESTDIR}//} -L ${PREFIX} ${PKG_ARGS_COMMON}
.endif # !PKG_ARGS_COMMON

PKG_SUFX?=		.tgz
#PKG_SUFX?=		.tbz		# bzip2(1) pkgs
# where pkg_add records its dirty deeds.
PKG_DBDIR?=		${DESTDIR}/var/db/syspkg

# Define SMART_MESSAGES in /etc/mk.conf for messages giving the tree
# of depencies for building, and the current target.
.ifdef SMART_MESSAGES
_PKGSRC_IN?=		===> ${.TARGET} [${PKGNAME}${_PKGSRC_DEPS}] ===
.else
_PKGSRC_IN?=		===
.endif

# Used to print all the '===>' style prompts - override this to turn them off.
ECHO_MSG?=		${ECHO}

# How to do nothing.  Override if you, for some strange reason, would rather
# do something.
DO_NADA?=		${TRUE}

.if !defined(PKGBASE) || !defined(SETNAME)
.BEGIN:
	@${ECHO_MSG} "PKGBASE and SETNAME are mandatory."
	@${FALSE}
.endif

PKGREPOSITORY?=		${PACKAGES}
PKGFILE?=		${PKGREPOSITORY}/${PKGNAME}${PKG_SUFX}

.MAIN: all

# Add these defs to the ones dumped into +BUILD_DEFS
BUILD_DEFS+=	OPSYS OS_VERSION MACHINE_ARCH OBJECT_FMT

.if !target(all)
# We don't build here
all:
.endif

################################################################
# More standard targets start here.
#
# These are the body of the build/install framework.  If you are
# not happy with the default actions, and you can't solve it by
# adding pre-* or post-* targets/scripts, override these.
################################################################

.if !target(show-downlevel)
show-downlevel:
	${_PKG_SILENT}${_PKG_DEBUG}					\
	found="`${PKG_INFO} -e \"${PKGBASE}<${PKGVERSION}\" || ${TRUE}`";\
	if [ "X$$found" != "X" -a "X$$found" != "X${PKGNAME}" ]; then	\
		${ECHO} "${PKGBASE} package: $$found installed, pkgsrc version ${PKGNAME}"; \
	fi
.endif

# Package

.if !target(do-package)
do-package: ${PLIST}
	${_PKG_SILENT}${_PKG_DEBUG}\
	${ECHO_MSG} "${_PKGSRC_IN}> Building binary package for ${PKGNAME}"; \
	if [ ! -d ${PKGREPOSITORY} ]; then			\
		${MKDIR} ${PKGREPOSITORY};			\
		if [ $$? -ne 0 ]; then				\
			${ECHO_MSG} "=> Can't create directory ${PKGREPOSITORY}."; \
			exit 1;					\
		fi;						\
	fi;							\
	if ${PKG_CREATE} ${PKG_ARGS_BINPKG} ${PKGFILE}; then		\
		(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} package-links);		\
	else							\
		(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} delete-package);		\
		exit 1;						\
	fi
.endif

# Some support rules for do-package

.if !target(package-links)
package-links:
	${_PKG_SILENT}${_PKG_DEBUG}(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} delete-package-links)
	${_PKG_SILENT}${_PKG_DEBUG}for cat in ${CATEGORIES}; do		\
		if [ ! -d ${PACKAGES}/$$cat ]; then			\
			${MKDIR} ${PACKAGES}/$$cat;			\
			if [ $$? -ne 0 ]; then				\
				${ECHO_MSG} "=> Can't create directory ${PACKAGES}/$$cat."; \
				exit 1;					\
			fi;						\
		fi;							\
		${RM} -f ${PACKAGES}/$$cat/${PKGNAME}${PKG_SUFX};	\
		${LN} -s ../${PKGREPOSITORYSUBDIR}/${PKGNAME}${PKG_SUFX} ${PACKAGES}/$$cat; \
	done;
.endif

.if !target(delete-package-links)
delete-package-links:
	${_PKG_SILENT}${_PKG_DEBUG}\
	${FIND} ${PACKAGES} -type l -name ${PKGNAME}${PKG_SUFX} | xargs ${RM} -f
.endif

.if !target(delete-package)
delete-package:
	${_PKG_SILENT}${_PKG_DEBUG}(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} delete-package-links)
	${_PKG_SILENT}${_PKG_DEBUG}${RM} -f ${PKGFILE}
.endif

################################################################
# This is the "generic" package target, actually a macro used from the
# six main targets.  See below for more.
################################################################

real-package:
	${_PKG_SILENT}${_PKG_DEBUG}cd ${.CURDIR} && ${SETENV} ${MAKE_ENV} ${MAKE} ${MAKEFLAGS} ${.TARGET:S/^real-/pre-/}
	${_PKG_SILENT}${_PKG_DEBUG}cd ${.CURDIR} && ${SETENV} ${MAKE_ENV} ${MAKE} ${MAKEFLAGS} ${.TARGET:S/^real-/do-/}
	${_PKG_SILENT}${_PKG_DEBUG}cd ${.CURDIR} && ${SETENV} ${MAKE_ENV} ${MAKE} ${MAKEFLAGS} ${.TARGET:S/^real-/post-/}

################################################################
# Skeleton targets start here
# 
# You shouldn't have to change these.  Either add the pre-* or
# post-* targets/scripts or redefine the do-* targets.  These
# targets don't do anything other than checking for cookies and
# call the necessary targets/scripts.
################################################################

.if !target(package)
package: uptodate-pkgtools ${PACKAGE_COOKIE}
.endif

${PACKAGE_COOKIE}:
	${_PKG_SILENT}${_PKG_DEBUG}(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} real-package)

# Empty pre-* and post-* targets, note we can't use .if !target()
# in the _PORT_USE macro

.for name in package

.if !target(pre-${name})
pre-${name}:
	@${DO_NADA}
.endif

.if !target(post-${name})
post-${name}:
	@${DO_NADA}
.endif

.endfor

# Deinstall
#
# Special target to remove installation

.if !target(deinstall)
deinstall: real-deinstall

.if (${DEINSTALLDEPENDS} != "NO")
.if (${DEINSTALLDEPENDS} != "ALL")
# used for removing stuff in bulk builds
real-su-deinstall-flags+=	-r -R
# used for "update" target
.else
real-su-deinstall-flags+=	-r
.endif
.endif
.ifdef PKG_VERBOSE
real-su-deinstall-flags+=	-v
.endif

real-deinstall:
	${_PKG_SILENT}${_PKG_DEBUG} \
	found="`${PKG_INFO} -e \"${PKGWILDCARD}\" || ${TRUE}`"; \
	if [ "$$found" != "" ]; then \
		${ECHO} Running ${PKG_DELETE} ${real-su-deinstall-flags} $$found ; \
		${PKG_DELETE} ${real-su-deinstall-flags} $$found || ${TRUE} ; \
	fi
.for pkg in ${BUILD_DEPENDS:C/:.*$//}
	${_PKG_SILENT}${_PKG_DEBUG} \
	found="`${PKG_INFO} -e \"${pkg}\" || ${TRUE}`"; \
	if [ "$$found" != "" ]; then \
		${ECHO} Running ${PKG_DELETE} $$found ; \
		${PKG_DELETE} ${real-su-deinstall-flags} $$found || ${TRUE} ; \
	fi
.endfor
.endif						# target(deinstall)


################################################################
# Some more targets supplied for users' convenience
################################################################

# The 'info' target can be used to display information about a package.
info: uptodate-pkgtools
	${_PKG_SILENT}${_PKG_DEBUG}${PKG_INFO} ${PKGWILDCARD}

# The 'check' target can be used to check an installed package.
check: uptodate-pkgtools
	${_PKG_SILENT}${_PKG_DEBUG}${PKG_ADMIN} check ${PKGWILDCARD}

# Cleaning up

.if !target(pre-clean)
pre-clean:
	@${DO_NADA}
.endif

.if !target(clean)
clean: pre-clean
	${RM} -f ${PLIST} ${BUILD_VERSION_FILE} ${BUILD_INFO_FILE} ${SIZE_PKG_FILE} ${SIZE_ALL_FILE}
.endif

.if !target(cleandir)
cleandir: clean
.endif

# Install binary pkg, without strict uptodate-check first
# (XXX should be able to snarf via FTP. Later...)
bin-install:
	@if [ -f ${PKGFILE} ] ; then 					\
		${ECHO_MSG} "Installing from binary pkg ${PKGFILE}" ;	\
		${PKG_ADD} ${PKGFILE} ; 				\
	else 				 				\
		${SHCOMMENT} Cycle through some FTP server here ;\
		${ECHO_MSG} "Installing from source" ;			\
		(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} package &&				\
		${MAKE} ${MAKEFLAGS} clean) ;				\
	fi


################################################################
# The special package-building targets
# You probably won't need to touch these
################################################################

# Nobody should want to override this unless PKGNAME is simply bogus.

.if !target(package-name)
package-name:
	@${ECHO} ${PKGNAME}
.endif # !target(package-name)

# Show (recursively) all the packages this package depends on.
# If PACKAGE_DEPENDS_WITH_PATTERNS is set, print as pattern (if possible)
PACKAGE_DEPENDS_WITH_PATTERNS?=true
# To be used (-> true) ONLY if the pkg in question is known to be installed
# (i.e. when calling for pkg_create args, and for fake-pkg)
# Will probably not work with PACKAGE_DEPENDS_WITH_PATTERNS=false ...
PACKAGE_DEPENDS_QUICK?=false
.if !target(run-depends-list)
run-depends-list:
.for dep in ${DEPENDS}
	${_PKG_SILENT}${_PKG_DEBUG}					\
	pkg="${dep:C/:.*//}";						\
	dir="${dep:C/[^:]*://}";					\
	cd ${.CURDIR};							\
	if ${PACKAGE_DEPENDS_WITH_PATTERNS}; then			\
		${ECHO} "$$pkg";					\
	else								\
		if cd $$dir 2>/dev/null; then				\
			(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} package-name; \
		else 							\
			${ECHO_MSG} "Warning: \"$$dir\" non-existent -- @pkgdep registration incomplete" >&2; \
		fi;							\
	fi;								\
	if ${PACKAGE_DEPENDS_QUICK} ; then 			\
		${PKG_INFO} -qf "$$pkg" | ${AWK} '/^@pkgdep/ {print $$2}'; \
	else 							\
		if cd $$dir 2>/dev/null; then				\
			(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} run-depends-list; \
		else 							\
			${ECHO_MSG} "Warning: \"$$dir\" non-existent -- @pkgdep registration incomplete" >&2; \
		fi;							\
	fi
.endfor
.endif # target(run-depends-list)

# Build a package but don't check the package cookie

.if !target(repackage)
repackage: pre-repackage package

pre-repackage:
	${_PKG_SILENT}${_PKG_DEBUG}${RM} -f ${PACKAGE_COOKIE}
.endif

# Build a package but don't check the cookie for installation, also don't
# install package cookie

.if !target(package-noinstall)
package-noinstall:
	${_PKG_SILENT}${_PKG_DEBUG}(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} PACKAGE_NOINSTALL=yes real-package)
.endif

################################################################
# Dependency checking
################################################################

.if !target(install-depends)
install-depends:
.endif

################################################################
# Everything after here are internal targets and really
# shouldn't be touched by anybody but the release engineers.
################################################################

.if !target(show-pkgtools-version)
show-pkgtools-version:
	@${ECHO} ${PKGTOOLS_VERSION}
.endif

# convenience target, to display make variables from command line
# i.e. "make show-var VARNAME=var", will print var's value
show-var:
	@${ECHO} "${${VARNAME}}"

# Stat all the files of one pkg and sum the sizes up. 
# 
# XXX This is intended to be run before pkg_create is called, so the
# existence of ${PLIST} can be assumed.
print-pkg-size-this:
	@${SHCOMMENT} "This pkg's files" ;				\
	${AWK} 'BEGIN { base = "${PREFIX}/" }				\
		/^@cwd/ { base = $$2 "/" }				\
		/^@ignore/ { next }					\
		NF == 1 { print base $$1 }'				\
		<${PLIST} 						\
	| sort -u							\
	| ${SED} -e 's, ,\\ ,g'						\
	| xargs ${LS} -ld						\
	| ${AWK} 'BEGIN { print("0 "); }				\
		  { print($$5, " + "); }				\
		  END { print("p"); }'					\
	| ${DC}

# Sizes of required pkgs (only)
# 
# XXX This is intended to be run before pkg_create is called, so the
# dependencies are all installed. 
print-pkg-size-depends:
	@(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} run-depends-list PACKAGE_DEPENDS_QUICK=true) \
	| xargs -n 1 ${SETENV} ${PKG_INFO} -e				\
	| sort -u							\
	| xargs ${SETENV} ${PKG_INFO} -qs				\
	| ${AWK} -- 'BEGIN { print("0 "); }				\
		/^[0-9]+$$/ { print($$1, " + "); }			\
		END { print("p"); }'					\
	| ${DC}


# Fake installation of package so that user can pkg_delete it later.
# Also, make sure that an installed package is recognized correctly in
# accordance to the @pkgdep directive in the packing lists

.if !target(register)
register: fake-pkg
.endif

.if !target(fake-pkg)
fake-pkg: ${PLIST}
	${_PKG_SILENT}${_PKG_DEBUG}\
	if [ ! -f ${PLIST} -o ! -f ${COMMENT} -o ! -f ${DESCR} ]; then \
		${ECHO} "** Missing package files for ${PKGNAME} - installation not recorded."; \
		exit 1;							\
	fi
	${_PKG_SILENT}${_PKG_DEBUG}\
	if [ ! -d ${PKG_DBDIR} ]; then	\
		${RM} -f ${PKG_DBDIR};					\
		${MKDIR} ${PKG_DBDIR};					\
	fi
.if defined(FORCE_PKG_REGISTER)
	${_PKG_SILENT}${_PKG_DEBUG}${PKG_DELETE} -O ${PKGNAME}
	${_PKG_SILENT}${_PKG_DEBUG}${RM} -rf ${PKG_DBDIR}/${PKGNAME}
.endif
	${_PKG_SILENT}${_PKG_DEBUG}${RM} -f ${BUILD_VERSION_FILE} ${BUILD_INFO_FILE}
	${_PKG_SILENT}${_PKG_DEBUG}${RM} -f ${SIZE_PKG_FILE} ${SIZE_ALL_FILE}
	${_PKG_SILENT}${_PKG_DEBUG}\
	files="";				\
	for f in ${.CURDIR}/Makefile ${FILESDIR}/* ${PKGDIR}/*; do	\
		if [ -f $$f ]; then					\
			files="$$files $$f";				\
		fi;							\
	done;								\
	${GREP} '\$$NetBSD' $$files | ${SED} -e 's|^${PKGSRCDIR}/||' > ${BUILD_VERSION_FILE};
.for def in ${BUILD_DEFS}
	@${ECHO} ${def}=	${${def}:Q} | ${SED} -e 's|^PATH=[^ 	]*|PATH=...|' >> ${BUILD_INFO_FILE}
.endfor
	@${ECHO} "CC=	${CC}-`${CC} --version`" >> ${BUILD_INFO_FILE}
	${_PKG_SILENT}${_PKG_DEBUG}					\
	${ECHO} "_PKGTOOLS_VER=${PKGTOOLS_VERSION}" >> ${BUILD_INFO_FILE}
	${_PKG_SILENT}${_PKG_DEBUG}					\
	size_this=`(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} print-pkg-size-this)`;		\
	size_depends=`(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} print-pkg-size-depends)`;	\
	${ECHO} $$size_this >${SIZE_PKG_FILE};				\
	${ECHO} $$size_this $$size_depends + p | ${DC} >${SIZE_ALL_FILE}
	${_PKG_SILENT}${_PKG_DEBUG}					\
	if [ ! -d ${PKG_DBDIR}/${PKGNAME} ]; then			\
		${ECHO_MSG} "${_PKGSRC_IN}> Registering installation for ${PKGNAME}"; \
		${MKDIR} ${PKG_DBDIR}/${PKGNAME};			\
		${PKG_CREATE} ${PKG_ARGS_INSTALL} -O ${PKGFILE} > ${PKG_DBDIR}/${PKGNAME}/+CONTENTS; \
		${CP} ${DESCR} ${PKG_DBDIR}/${PKGNAME}/+DESC;		\
		${CP} ${COMMENT} ${PKG_DBDIR}/${PKGNAME}/+COMMENT;	\
		${CP} ${BUILD_VERSION_FILE} ${PKG_DBDIR}/${PKGNAME}/+BUILD_VERSION; \
		${CP} ${BUILD_INFO_FILE} ${PKG_DBDIR}/${PKGNAME}/+BUILD_INFO; \
		if ${TEST} -e ${SIZE_PKG_FILE}; then 			\
			${CP} ${SIZE_PKG_FILE} ${PKG_DBDIR}/${PKGNAME}/+SIZE_PKG; \
		fi ; 							\
		if ${TEST} -e ${SIZE_ALL_FILE}; then 			\
			${CP} ${SIZE_ALL_FILE} ${PKG_DBDIR}/${PKGNAME}/+SIZE_ALL; \
		fi ; 							\
		if [ -n "${PKG_INSTALL_FILE}" ]; then			\
			if ${TEST} -e ${PKG_INSTALL_FILE}; then		\
				${CP} ${PKG_INSTALL_FILE} ${PKG_DBDIR}/${PKGNAME}/+INSTALL; \
			fi;						\
		fi;							\
		if [ -n "${PKG_DEINSTALL_FILE}" ]; then			\
			if ${TEST} -e ${PKG_DEINSTALL_FILE}; then		\
				${CP} ${PKG_DEINSTALL_FILE} ${PKG_DBDIR}/${PKGNAME}/+DEINSTALL; \
			fi;						\
		fi;							\
		if [ -n "${MESSAGE_FILE}" ]; then			\
			if ${TEST} -e ${MESSAGE_FILE}; then		\
				${CP} ${MESSAGE_FILE} ${PKG_DBDIR}/${PKGNAME}/+DISPLAY; \
			fi;						\
		fi;							\
		list="`(cd ${.CURDIR} && ${MAKE} ${MAKEFLAGS} run-depends-list PACKAGE_DEPENDS_QUICK=true ECHO_MSG=${TRUE} | sort -u)`" ; \
		for dep in $$list; do \
			realdep="`${PKG_INFO} -e \"$$dep\" || ${TRUE}`" ; \
			if [ `${ECHO} $$realdep | wc -w` -gt 1 ]; then 				\
				${ECHO} '***' "WARNING: '$$dep' expands to several installed packages " ; \
				${ECHO} "    (" `${ECHO} $$realdep` ")." ; \
				${ECHO} "    Please check if this is really intended!" ; \
				continue ; 				\
			fi ; 						\
		done ; 						\
		for realdep in `echo $$list | xargs -n 1 ${SETENV} ${PKG_INFO} -e | sort -u`; do \
			if ${TEST} -z "$$realdep"; then			\
				${ECHO} "$$dep not installed - dependency NOT registered" ; \
			elif [ -d ${PKG_DBDIR}/$$realdep ]; then	\
				if ${TEST} ! -e ${PKG_DBDIR}/$$realdep/+REQUIRED_BY; then \
					${TOUCH} ${PKG_DBDIR}/$$realdep/+REQUIRED_BY; \
				fi; 					\
				${AWK} 'BEGIN { found = 0; } 		\
					$$0 == "${PKGNAME}" { found = 1; } \
					{ print $$0; } 			\
					END { if (!found) { printf("%s\n", "${PKGNAME}"); }}' \
					< ${PKG_DBDIR}/$$realdep/+REQUIRED_BY > ${PKG_DBDIR}/$$realdep/reqby.$$$$; \
				${MV} ${PKG_DBDIR}/$$realdep/reqby.$$$$ ${PKG_DBDIR}/$$realdep/+REQUIRED_BY; \
				${ECHO} "${PKGNAME} requires installed package $$realdep"; \
			fi;						\
		done;							\
	fi
.endif

# Depend is generally meaningless for arbitrary packages, but if someone wants
# one they can override this.  This is just to catch people who've gotten into
# the habit of typing `${MAKE} depend all install' as a matter of course.
#
.if !target(depend)
depend:
.endif

# Same goes for tags
.if !target(tags)
tags:
.endif

${PLIST}:
	sh ${.CURDIR}/../../../../sets/makeplist ${SETNAME} ${PKGBASE} > ${PLIST}
