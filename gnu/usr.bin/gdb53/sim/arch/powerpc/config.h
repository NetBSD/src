/* config.h.  Generated automatically by configure.  */
/* config.in.  Generated automatically from configure.in by autoheader.  */

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if your struct stat has st_rdev.  */
#define HAVE_ST_RDEV 1

/* Define if your struct tm has tm_zone.  */
#define HAVE_TM_ZONE 1

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
/* #undef HAVE_TZNAME */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if NLS is requested.  */
#define ENABLE_NLS 1

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
/* #undef HAVE_GETTEXT */

/* Define as 1 if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

/* Define if your locale.h file contains LC_MESSAGES.  */
#define HAVE_LC_MESSAGES 1

/* Define if you have the __argz_count function.  */
/* #undef HAVE___ARGZ_COUNT */

/* Define if you have the __argz_next function.  */
/* #undef HAVE___ARGZ_NEXT */

/* Define if you have the __argz_stringify function.  */
/* #undef HAVE___ARGZ_STRINGIFY */

/* Define if you have the access function.  */
#define HAVE_ACCESS 1

/* Define if you have the cfgetispeed function.  */
#define HAVE_CFGETISPEED 1

/* Define if you have the cfgetospeed function.  */
#define HAVE_CFGETOSPEED 1

/* Define if you have the cfsetispeed function.  */
#define HAVE_CFSETISPEED 1

/* Define if you have the cfsetospeed function.  */
#define HAVE_CFSETOSPEED 1

/* Define if you have the chdir function.  */
#define HAVE_CHDIR 1

/* Define if you have the chmod function.  */
#define HAVE_CHMOD 1

/* Define if you have the chown function.  */
#define HAVE_CHOWN 1

/* Define if you have the dcgettext function.  */
/* #undef HAVE_DCGETTEXT */

/* Define if you have the dup function.  */
#define HAVE_DUP 1

/* Define if you have the dup2 function.  */
#define HAVE_DUP2 1

/* Define if you have the fchmod function.  */
#define HAVE_FCHMOD 1

/* Define if you have the fchown function.  */
#define HAVE_FCHOWN 1

/* Define if you have the fcntl function.  */
#define HAVE_FCNTL 1

/* Define if you have the fstat function.  */
#define HAVE_FSTAT 1

/* Define if you have the fstatfs function.  */
#define HAVE_FSTATFS 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getdirentries function.  */
#define HAVE_GETDIRENTRIES 1

/* Define if you have the getegid function.  */
#define HAVE_GETEGID 1

/* Define if you have the geteuid function.  */
#define HAVE_GETEUID 1

/* Define if you have the getgid function.  */
#define HAVE_GETGID 1

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the getpid function.  */
#define HAVE_GETPID 1

/* Define if you have the getppid function.  */
#define HAVE_GETPPID 1

/* Define if you have the getrusage function.  */
#define HAVE_GETRUSAGE 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the getuid function.  */
#define HAVE_GETUID 1

/* Define if you have the ioctl function.  */
#define HAVE_IOCTL 1

/* Define if you have the kill function.  */
#define HAVE_KILL 1

/* Define if you have the link function.  */
#define HAVE_LINK 1

/* Define if you have the lseek function.  */
#define HAVE_LSEEK 1

/* Define if you have the lstat function.  */
#define HAVE_LSTAT 1

/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the munmap function.  */
#define HAVE_MUNMAP 1

/* Define if you have the pipe function.  */
#define HAVE_PIPE 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the readlink function.  */
#define HAVE_READLINK 1

/* Define if you have the rmdir function.  */
#define HAVE_RMDIR 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the setregid function.  */
#define HAVE_SETREGID 1

/* Define if you have the setreuid function.  */
#define HAVE_SETREUID 1

/* Define if you have the sigprocmask function.  */
#define HAVE_SIGPROCMASK 1

/* Define if you have the stat function.  */
#define HAVE_STAT 1

/* Define if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the symlink function.  */
#define HAVE_SYMLINK 1

/* Define if you have the tcdrain function.  */
#define HAVE_TCDRAIN 1

/* Define if you have the tcflow function.  */
#define HAVE_TCFLOW 1

/* Define if you have the tcflush function.  */
#define HAVE_TCFLUSH 1

/* Define if you have the tcgetattr function.  */
#define HAVE_TCGETATTR 1

/* Define if you have the tcgetpgrp function.  */
#define HAVE_TCGETPGRP 1

/* Define if you have the tcsendbreak function.  */
#define HAVE_TCSENDBREAK 1

/* Define if you have the tcsetattr function.  */
#define HAVE_TCSETATTR 1

/* Define if you have the tcsetpgrp function.  */
#define HAVE_TCSETPGRP 1

/* Define if you have the time function.  */
#define HAVE_TIME 1

/* Define if you have the umask function.  */
#define HAVE_UMASK 1

/* Define if you have the unlink function.  */
#define HAVE_UNLINK 1

/* Define if you have the <argz.h> header file.  */
/* #undef HAVE_ARGZ_H */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <nl_types.h> header file.  */
#define HAVE_NL_TYPES_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/mount.h> header file.  */
/* #undef HAVE_SYS_MOUNT_H */

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/termio.h> header file.  */
/* #undef HAVE_SYS_TERMIO_H */

/* Define if you have the <sys/termios.h> header file.  */
#define HAVE_SYS_TERMIOS_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/times.h> header file.  */
#define HAVE_SYS_TIMES_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <time.h> header file.  */
#define HAVE_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <values.h> header file.  */
/* #undef HAVE_VALUES_H */
