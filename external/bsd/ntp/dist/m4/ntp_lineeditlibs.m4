AC_DEFUN([NTP_LINEEDITLIBS], [
    NTP_ORIG_LIBS="$LIBS"
    AC_ARG_WITH([lineeditlibs],
	[AC_HELP_STRING([--with-lineeditlibs], [edit,editline (readline may be specified if desired)])],
	[use_lineeditlibs="$withval"],
	[use_lineeditlibs="edit,editline"])

    AC_MSG_CHECKING([line editing libraries])
    AC_MSG_RESULT([$use_lineeditlibs])
    case "$use_lineeditlibs" in
     no) 
	ntp_lib_lineedit=no
	;;
     *)
	for lineedit_lib in `echo $use_lineeditlibs | sed -e 's/,/ /'`; do
	    for term_lib in "" termcap curses ncurses; do
		case "$term_lib" in
		 '') 
		    TRY_LIB="-l$lineedit_lib"
		    ;;
		 *) TRY_LIB="-l$lineedit_lib -l$term_lib"
		esac	# $term_lib
		LIBS="$NTP_ORIG_LIBS $TRY_LIB"
		AC_MSG_CHECKING([for readline() with $TRY_LIB])
		AC_TRY_LINK_FUNC([readline], [ntp_lib_lineedit="$TRY_LIB"])
		case "$ntp_lib_lineedit" in
		 '')
		    AC_MSG_RESULT([no])
		    ;;
		 *) 
		    # Use readline()
		    AC_MSG_RESULT([yes])
		    break
		esac		# $ntp_lib_lineedit
		case "$term_lib" in
		 '')
		    # do not try el_gets without a terminal library
		    ;;
		 *)
		    AC_MSG_CHECKING([for el_gets() with $TRY_LIB])
		    AC_TRY_LINK_FUNC([el_gets], [ntp_lib_lineedit="$TRY_LIB"])
		    case "$ntp_lib_lineedit" in
		     '')
			AC_MSG_RESULT([no])
			;;
		     *) # Use el_gets()
			AC_MSG_RESULT([yes])
			break
			;;
		    esac	# $ntp_lib_lineedit
		esac		# $term_lib
	    done
	    case "$ntp_lib_lineedit" in
	     '') ;;
	     *)  break ;;
	    esac
	done
	LIBS="$NTP_ORIG_LIBS"
	;;
    esac	# $use_lineeditlibs

    case "$ntp_lib_lineedit" in
     '')
	ntp_lib_lineedit="no"
	;;
     no)
	;;
     *)
	EDITLINE_LIBS="$ntp_lib_lineedit"
	AC_SUBST(EDITLINE_LIBS)
	;;
    esac	# $ntp_lib_lineedit

    case "$ntp_lib_lineedit" in
     no)
	;;
     *)
	dnl AC_DEFINE(HAVE_LIBREADLINE, 1,
        dnl           [Define if you have a readline compatible library])
	AC_CHECK_HEADERS([readline.h readline/readline.h histedit.h])
	AC_CHECK_HEADERS([history.h readline/history.h])
	
	AC_MSG_CHECKING([whether readline supports history])
	
	ntp_lib_lineedit_history="no"
	ORIG_LIBS="$LIBS"
	LIBS="$ORIG_LIBS $ntp_lib_lineedit"
	AC_TRY_LINK_FUNC([add_history], [ntp_lib_lineedit_history="yes"])
	LIBS="$ORIG_LIBS"

	AC_MSG_RESULT([$ntp_lib_lineedit_history])

	case "$ntp_lib_lineedit_history" in
	 yes)
	    AC_DEFINE(HAVE_READLINE_HISTORY, 1,
		      [Define if your readline library has \`add_history'])
	esac	# $ntp_lib_lineedit_history
    esac	# $ntp_lib_lineedit
    dnl when oldest supported autoconf has AS_UNSET
    dnl AS_UNSET([NTP_ORIG_LIBS TRY_LIB use_lineeditlibs])
    $as_unset NTP_ORIG_LIBS TRY_LIB use_lineeditlibs
])dnl
