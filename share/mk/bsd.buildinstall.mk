#	$NetBSD: bsd.buildinstall.mk,v 1.3 2025/08/25 02:44:31 riastradh Exp $

#
# build_install logic for src/Makefile
# Used by src/lib/Makefile and src/tools/Makefile.
#
# Compute a list of subdirectories delimited by .WAIT.
# Run "make dependall && make install" for all subdirectories in a group
# concurrently, but wait after each group.
#
SUBDIR_GROUPS=	0
CUR_GROUP:=	0
.for dir in ${SUBDIR}
.  if ${dir} == ".WAIT"
CUR_GROUP:=	${SUBDIR_GROUPS:[#]}
SUBDIR_GROUPS:=	${SUBDIR_GROUPS} ${CUR_GROUP}
.  else
SUBDIR_GROUP.${CUR_GROUP}+=	${dir}
.endif

.endfor

build_install: .MAKE
.for group in ${SUBDIR_GROUPS}
.  if !empty(SUBDIR_GROUP.${group})
	${MAKEDIRTARGET} . ${SUBDIR_GROUP.${group}:C/^/dependall-/}
	${MAKEDIRTARGET} . ${SUBDIR_GROUP.${group}:C/^/install-/}
.  endif
.endfor
