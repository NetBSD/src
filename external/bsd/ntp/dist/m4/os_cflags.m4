dnl ######################################################################
dnl Specify additional compile options based on the OS and the compiler
AC_DEFUN([NTP_OS_CFLAGS], [
    AC_MSG_CHECKING([additional compiler flags])
    # allow ntp_os_flags to be preset to skip this stuff
    case "${ntp_os_cflags+set}" in
     set)
	;;
     *)
	ntp_os_cflags=""
	case "$host_os" in
	 aix[[1-3]]*)
	    ;;
	 aix4.[[0-2]]*)
	    # turn on additional headers
	    ntp_os_cflags="-D_XOPEN_EXTENDED_SOURCE"
	    ;;
	 aix5.3*)
	    # avoid circular dependencies in yp headers, and more
	    ntp_os_cflags="-DHAVE_BAD_HEADERS -D_XOPEN_EXTENDED_SOURCE"
	    ntp_os_cflags="${ntp_os_cflags} -D_USE_IRS -D_MSGQSUPPORT"
	    ;;
	 aix*)
	    # avoid circular dependencies in yp headers
	    ntp_os_cflags="-DHAVE_BAD_HEADERS -D_XOPEN_EXTENDED_SOURCE"
	    ntp_os_cflags="${ntp_os_cflags} -D_USE_IRS"
	    ;;
	 amigaos)
	    ntp_os_cflags="-DSYS_AMIGA"
	    ;;
	 darwin*|macosx*|rhapsody*)
	    ntp_os_cflags="-D_P1003_1B_VISIBLE"
	    ;;
	 hpux10.*)		# at least for hppa2.0-hp-hpux10.20
	    case "$GCC" in
	     yes)
		;;
	     *)
		# use Ansi compiler on HPUX, and some -Wp magic
		ntp_os_cflags="-Ae -Wp,-H18816"
		;;
	    esac
	    ntp_os_cflags="${ntp_os_cflags} -D_HPUX_SOURCE"
	    ;;
	 hpux*)
	    case "$GCC" in
	     yes)
		;;
	     *)
		# use Ansi compiler on HPUX
		ntp_os_cflags="-Ae"
	    esac
	    ntp_os_cflags="${ntp_os_cflags} -D_HPUX_SOURCE"
	    ;;
	 irix6*)
	    case "$CC" in
	     cc)
		# do not use 64-bit compiler
		ntp_os_cflags="-n32 -mips3 -Wl,-woff,84"
	    esac
	    ;;
	 nextstep3)
	    ntp_os_cflags="-posix"
	    ;;
	 solaris1*|solaris2.[[0-5]]|solaris2.5.*)
	    ;;
	 sunos[[34]]*|sunos5.[[0-5]]|sunos5.5.*)
	    ;;
	 solaris2*|sunos5*)
	    # turn on 64-bit file offset interface
	    ntp_os_cflags="-D_LARGEFILE64_SOURCE"
	    ;;
	 vxworks*)
	    case "$build" in
	     $host)
		;;
	     *)
		# Quick and dirty sanity check
		case "$VX_KERNEL" in
		 '')
		    AC_MSG_ERROR([See html/build/hints/vxworks.html])
		esac
		ntp_os_cflags="-DSYS_VXWORKS"
	    esac
	    ;;
	esac
    esac
    case "$ntp_os_flags" in
     '')
	ntp_os_cflags_msg="none needed"
	;;
     *)
	ntp_os_cflags_msg="$ntp_os_cflags"
	CFLAGS="$CFLAGS $ntp_os_cflags"
    esac
    AC_MSG_RESULT([$ntp_os_cflags_msg])
    AS_UNSET([ntp_os_cflags_msg])
])
dnl ======================================================================
