# $NetBSD: bsd.clean.mk,v 1.1 2011/09/10 16:57:35 apb Exp $

# <bsd.clean.mk>
#
# Public targets:
#
# clean:	Delete files listed in ${CLEANFILES}.
# cleandir:	Delete files listed in ${CLEANFILES} and ${CLEANDIRFILES}.
#
# Public variables:
#
# CLEANFILES	Files to remove for both the clean and cleandir targets.
#
# CLEANDIRFILES	Files to remove for the cleandir target, but not for
#		the clean target.

.if !defined(_BSD_CLEAN_MK_)
_BSD_CLEAN_MK_=1

.include <bsd.init.mk>

clean:		.PHONY __doclean
__doclean:	.PHONY .MADE __cleanuse CLEANFILES
cleandir:	.PHONY clean __docleandir
__docleandir:	.PHONY .MADE __cleanuse CLEANDIRFILES

# __cleanuse is invoked with ${.ALLSRC} as the name of a variable
# (such as CLEANFILES or CLEANDIRFILES), or possibly a list of
# variable names.  ${.ALLSRC:@v@${${v}}@} will be the list of
# files to delete.  (We pass the variable name, e.g. CLEANFILES,
# instead of the file names, e.g. ${CLEANFILES}, because we don't
# want make to replace any of the file names with the result of
# searching .PATH.)
#
# If the list of file names is non-empty then use "rm -f" to
# delete the files, and "ls -d" to check that the deletion was
# successful.  If the list of files is empty, then the commands
# reduce to "true", with an "@" prefix to prevent echoing.
#
# If .OBJDIR is different from .SRCDIR then repeat all this for
# both .OBJDIR and .SRCDIR.
#
__cleanuse: .USE
	${"${.ALLSRC:@v@${${v}}@}" == "":?@true:${_MKMSG} \
		"clean" ${.ALLSRC} }
.for _d in ${"${.OBJDIR}" == "${.CURDIR}" \
		:? ${.OBJDIR} \
		:  ${.OBJDIR} ${.CURDIR} }
	-${"${.ALLSRC:@v@${${v}}@}" == "":?@true: \
	    (cd ${_d} && rm -f ${.ALLSRC:@v@${${v}}@}) }
	@${"${.ALLSRC:@v@${${v}}@}" == "":?true: \
	    bad="\$(cd ${_d} && ls -d ${.ALLSRC:@v@${${v}}@} 2>/dev/null)"; \
	    if test -n "\$bad"; then \
	        echo "Failed to remove files from ${_d}:" ; \
	        echo "\$bad" ; \
	        false ; \
	    fi }
.endfor

.endif	# !defined(_BSD_CLEAN_MK)
