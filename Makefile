#	$NetBSD: Makefile,v 1.176 2002/06/19 00:17:11 lukem Exp $

# This is the top-level makefile for building NetBSD. For an outline of
# how to build a snapshot or release, as well as other release engineering
# information, see http://www.netbsd.org/developers/releng/index.html
#
# Not everything you can set or do is documented in this makefile. In
# particular, you should review the files in /usr/share/mk (especially
# bsd.README) for general information on building programs and writing
# Makefiles within this structure, and see the comments in src/etc/Makefile
# for further information on installation and release set options.
#
# Variables listed below can be set on the make command line (highest
# priority), in /etc/mk.conf (middle priority), or in the environment
# (lowest priority).
#
# Variables:
#   DESTDIR is the target directory for installation of the compiled
#	software. It defaults to /. Note that programs are built against
#	libraries installed in DESTDIR.
#   MKMAN, if set to `no', will prevent building of manual pages.
#   MKOBJDIRS, if not set to `no', will build object directories at 
#	an appropriate point in a build.
#   MKSHARE, if set to `no', will prevent building and installing
#	anything in /usr/share.
#   UPDATE, if defined, will avoid a `make cleandir' at the start of
#     `make build', as well as having the effects listed in
#     /usr/share/mk/bsd.README.
#   NOCLEANDIR, if defined, will avoid a `make cleandir' at the start
#     of the `make build'.
#   NOINCLUDES will avoid the `make includes' usually done by `make build'.
#
# Targets:
#   build:
#	Builds a full release of NetBSD in DESTDIR.  If BUILD_DONE is
#	set, this is an empty target.
#   release:
#	Does a `make build,' and then tars up the DESTDIR files
#	into RELEASEDIR, in release(7) format. (See etc/Makefile for
#	more information on this.)
#   regression-tests:
#	Runs the regression tests in "regress" on this host.
#
# Targets invoked by `make build,' in order:
#   cleandir:        cleans the tree.
#   obj:             creates object directories.
#   do-tools:        builds host toolchain.
#   do-distrib-dirs: creates the distribution directories.
#   includes:        installs include files.
#   do-lib-csu:      builds and installs prerequisites from lib/csu.
#   do-lib:          builds and installs prerequisites from lib.
#   do-gnu-lib:      builds and installs prerequisites from gnu/lib.
#   do-build:        builds and installs the entire system.

.if ${.MAKEFLAGS:M${.CURDIR}/share/mk} == ""
.MAKEFLAGS: -m ${.CURDIR}/share/mk
.endif

# If _SRC_TOP_OBJ_ gets set here, we will end up with a directory that may
# not be the top level objdir, because "make obj" can happen in the *middle*
# of "make build" (long after <bsd.own.mk> is calculated it).  So, pre-set
# _SRC_TOP_OBJ_ here so it will not be added to ${.MAKEOVERRIDES}.
_SRC_TOP_OBJ_=

.include <bsd.own.mk>

# Sanity check: make sure that "make build" is not invoked simultaneously
# with a standard recursive target.

.if make(build) || make(release) || make(snapshot)
.for targ in ${TARGETS:Nobj:Ncleandir}
.if make(${targ}) && !target(.BEGIN)
.BEGIN:
	@echo 'BUILD ABORTED: "make build" and "make ${targ}" are mutually exclusive.'
	@false
.endif
.endfor
.endif

_SUBDIR=	tools lib include gnu bin games libexec sbin usr.bin
_SUBDIR+=	usr.sbin share sys etc distrib regress

# Weed out directories that don't exist.

.for dir in ${_SUBDIR}
.if exists(${dir}/Makefile) && (${BUILD_${dir}:Uyes} != "no")
SUBDIR+=	${dir}
.endif
.endfor

.if exists(regress)
regression-tests:
	@echo Running regression tests...
	@(cd ${.CURDIR}/regress && ${MAKE} regress)
.endif

afterinstall:
.if ${MKMAN} != "no"
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)
.endif
.if defined(UNPRIVED) && (${MKINFO} != "no")
	(cd ${.CURDIR}/gnu/usr.bin/texinfo/install-info && ${MAKE} infodir-meta)
.endif
.if !defined(DESTDIR) || ${DESTDIR} == "" || ${DESTDIR} == "/"
	(${MAKE} postinstall-check)
.endif

postinstall-check:
	@echo "   === Post installation checks ==="
	sh ${.CURDIR}/etc/postinstall -s ${.CURDIR} -d ${DESTDIR}/ check
	@echo "   ================================"

postinstall-fix: .NOTMAIN
	@echo "   === Post installation fixes ==="
	sh ${.CURDIR}/etc/postinstall -s ${.CURDIR} -d ${DESTDIR}/ fix
	@echo "   ================================"


# Targets (in order!) called by "make build".

BUILDTARGETS+=	check-tools
.if !defined(UPDATE) && !defined(NOCLEANDIR)
BUILDTARGETS+=	cleandir
.endif
.if ${MKOBJDIRS} != "no"
BUILDTARGETS+=	obj
.endif
.if ${USETOOLS} == "yes"
BUILDTARGETS+=	do-tools
.endif
.if !defined(NODISTRIBDIRS)
BUILDTARGETS+=	do-distrib-dirs
.endif
.if !defined(NOINCLUDES)
BUILDTARGETS+=	includes
.endif
BUILDTARGETS+=	do-lib-csu do-lib do-gnu-lib do-build

# Enforce proper ordering of some rules.

.ORDER:		${BUILDTARGETS}
includes-lib:	includes-include includes-sys
includes-gnu:	includes-lib

# Build the system and install into DESTDIR.

build:
.if defined(BUILD_DONE)
	@echo "Build already installed into ${DESTDIR}"
.else
	@printf "Build started at: " && date
.for tgt in ${BUILDTARGETS}
	@(cd ${.CURDIR} && ${MAKE} ${tgt})
.endfor
	@printf "Build finished at: " && date
.endif

# Build a full distribution, but not a release (i.e. no sets into
# ${RELEASEDIR}).

distribution: build
	(cd ${.CURDIR}/etc && ${MAKE} INSTALL_DONE=1 distribution)

# Build a release or snapshot (implies "make build").

release snapshot: build
	(cd ${.CURDIR}/etc && ${MAKE} INSTALL_DONE=1 release)

# Special components of the "make build" process.

check-tools:
.if defined(USE_NEW_TOOLCHAIN) && (${USE_NEW_TOOLCHAIN} != "nowarn")
	@echo '*** WARNING:  Building on MACHINE=${MACHINE} with USE_NEW_TOOLCHAIN.'
	@echo '*** This platform is not yet verified to work with the new toolchain,'
	@echo '*** and may result in a failed build or corrupt binaries!'
.endif
.if defined(NBUILDJOBS)
	@echo '*** WARNING: NBUILDJOBS is obsolete; use -j directly instead!'
.endif

do-distrib-dirs:
.if !defined(DESTDIR) || ${DESTDIR} == ""
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=${DESTDIR} distrib-dirs)
.endif

.for dir in tools lib/csu lib gnu/lib
do-${dir:S/\//-/}:
.for targ in dependall install
	(cd ${.CURDIR}/${dir} && ${MAKE} ${targ})
.endfor
.endfor

do-build:
.for targ in dependall install
	(cd ${.CURDIR} && ${MAKE} ${targ} BUILD_tools=no BUILD_lib=no)
.endfor

# Speedup stubs for some subtrees that don't need to run these rules.
# (Tells <bsd.subdir.mk> not to recurse for them.)

.for dir in bin etc distrib games libexec regress sbin usr.sbin tools
includes-${dir}:
	@true
.endfor
.for dir in etc distrib regress
install-${dir}:
	@true
.endfor

# XXX this needs to change when distrib Makefiles are recursion compliant
dependall-distrib depend-distrib all-distrib:
	@true

.include <bsd.obj.mk>
.include <bsd.subdir.mk>

build-docs: ${.CURDIR}/BUILDING
${.CURDIR}/BUILDING: BUILDING.mdoc
	groff -mdoc -Tascii -P-b -P-u -P-o $> >$@
