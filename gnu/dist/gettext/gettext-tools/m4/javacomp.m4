# javacomp.m4 serial 6 (gettext-0.13)
dnl Copyright (C) 2001-2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prerequisites of javacomp.sh.
# Sets HAVE_JAVACOMP to nonempty if javacomp.sh will work.

AC_DEFUN([gt_JAVACOMP],
[
  AC_MSG_CHECKING([for Java compiler])
  AC_EGREP_CPP(yes, [
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  yes
#endif
], CLASSPATH_SEPARATOR=';', CLASSPATH_SEPARATOR=':')
  HAVE_JAVACOMP=1
  if test -n "$JAVAC"; then
    ac_result="$JAVAC"
  else
    pushdef([AC_MSG_CHECKING],[:])dnl
    pushdef([AC_CHECKING],[:])dnl
    pushdef([AC_MSG_RESULT],[:])dnl
    AC_CHECK_PROG(HAVE_GCJ_IN_PATH, gcj, yes)
    AC_CHECK_PROG(HAVE_JAVAC_IN_PATH, javac, yes)
    AC_CHECK_PROG(HAVE_JIKES_IN_PATH, jikes, yes)
    popdef([AC_MSG_RESULT])dnl
    popdef([AC_CHECKING])dnl
    popdef([AC_MSG_CHECKING])dnl
changequote(,)dnl
    # Test for a good gcj version (>= 3.0).
    # Exclude some versions of gcj: gcj 3.0.4 compiles GetURL.java to invalid
    # bytecode, that crashes with an IllegalAccessError when executed by
    # gij 3.0.4 or with a VerifyError when executed by Sun Java. Likewise for
    # gcj 3.1.
    # I also exclude gcj 3.2, 3.3 etc. because I have no idea when this bug
    # will be fixed. The bug is registered as java/7066, see
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=7066
    # FIXME: Check new versions of gcj as they come out.
    if test -n "$HAVE_GCJ_IN_PATH" \
       && gcj --version 2>/dev/null | sed -e 's,^[^0-9]*,,' -e 1q | sed -e '/^3\.[0123456789]/d' | grep '^[3-9]' >/dev/null \
       && (
        # See if libgcj.jar is well installed.
        cat > conftest.java <<EOF
public class conftest {
  public static void main (String[] args) {
  }
}
EOF
        gcj -C -d . conftest.java 2>/dev/null
        error=$?
        rm -f conftest.java conftest.class
        exit $error
       ); then
      HAVE_GCJ_C=1
      ac_result="gcj -C"
    else
      if test -n "$HAVE_JAVAC_IN_PATH" \
         && (javac -version >/dev/null 2>/dev/null || test $? -le 2) \
         && (if javac -help 2>&1 >/dev/null | grep at.dms.kjc.Main >/dev/null && javac -help 2>/dev/null | grep 'released.*2000' >/dev/null ; then exit 1; else exit 0; fi); then
        HAVE_JAVAC=1
        ac_result="javac"
      else
        if test -n "$HAVE_JIKES_IN_PATH" \
           && (jikes >/dev/null 2>/dev/null || test $? = 1) \
           && (
            # See if the existing CLASSPATH is sufficient to make jikes work.
            cat > conftest.java <<EOF
public class conftest {
  public static void main (String[] args) {
  }
}
EOF
            unset JAVA_HOME
            jikes conftest.java 2>/dev/null
            error=$?
            rm -f conftest.java conftest.class
            exit $error
           ); then
          HAVE_JIKES=1
          ac_result="jikes"
        else
          HAVE_JAVACOMP=
          ac_result="no"
        fi
      fi
    fi
changequote([,])dnl
  fi
  AC_MSG_RESULT([$ac_result])
  AC_SUBST(JAVAC)
  AC_SUBST(CLASSPATH)
  AC_SUBST(CLASSPATH_SEPARATOR)
  AC_SUBST(HAVE_GCJ_C)
  AC_SUBST(HAVE_JAVAC)
  AC_SUBST(HAVE_JIKES)
])
