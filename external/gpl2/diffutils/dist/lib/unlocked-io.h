/*	$NetBSD: unlocked-io.h,v 1.1.1.1 2016/01/13 03:15:30 christos Exp $	*/

#ifndef UNLOCKED_IO_H
# define UNLOCKED_IO_H 1

# ifndef USE_UNLOCKED_IO
#  define USE_UNLOCKED_IO 1
# endif

# if USE_UNLOCKED_IO

/* These are wrappers for functions/macros from GNU libc.
   The standard I/O functions are thread-safe.  These *_unlocked ones are
   more efficient but not thread-safe.  That they're not thread-safe is
   fine since all of the applications in this package are single threaded.  */

#  if HAVE_DECL_CLEARERR_UNLOCKED
#   undef clearerr
#   define clearerr(x) clearerr_unlocked (x)
#  endif
#  if HAVE_DECL_FEOF_UNLOCKED
#   undef feof
#   define feof(x) feof_unlocked (x)
#  endif
#  if HAVE_DECL_FERROR_UNLOCKED
#   undef ferror
#   define ferror(x) ferror_unlocked (x)
#  endif
#  if HAVE_DECL_FFLUSH_UNLOCKED
#   undef fflush
#   define fflush(x) fflush_unlocked (x)
#  endif
#  if HAVE_DECL_FGETS_UNLOCKED
#   undef fgets
#   define fgets(x,y,z) fgets_unlocked (x,y,z)
#  endif
#  if HAVE_DECL_FPUTC_UNLOCKED
#   undef fputc
#   define fputc(x,y) fputc_unlocked (x,y)
#  endif
#  if HAVE_DECL_FPUTS_UNLOCKED
#   undef fputs
#   define fputs(x,y) fputs_unlocked (x,y)
#  endif
#  if HAVE_DECL_FREAD_UNLOCKED
#   undef fread
#   define fread(w,x,y,z) fread_unlocked (w,x,y,z)
#  endif
#  if HAVE_DECL_FWRITE_UNLOCKED
#   undef fwrite
#   define fwrite(w,x,y,z) fwrite_unlocked (w,x,y,z)
#  endif
#  if HAVE_DECL_GETC_UNLOCKED
#   undef getc
#   define getc(x) getc_unlocked (x)
#  endif
#  if HAVE_DECL_GETCHAR_UNLOCKED
#   undef getchar
#   define getchar() getchar_unlocked ()
#  endif
#  if HAVE_DECL_PUTC_UNLOCKED
#   undef putc
#   define putc(x,y) putc_unlocked (x,y)
#  endif
#  if HAVE_DECL_PUTCHAR_UNLOCKED
#   undef putchar
#   define putchar(x) putchar_unlocked (x)
#  endif

# endif /* USE_UNLOCKED_IO */
#endif /* UNLOCKED_IO_H */
