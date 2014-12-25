/*	$NetBSD: _Noreturn.h,v 1.2.8.2 2014/12/25 02:34:46 snj Exp $	*/

#if !defined _Noreturn && __STDC_VERSION__ < 201112
# if (3 <= __GNUC__ || (__GNUC__ == 2 && 8 <= __GNUC_MINOR__) \
      || 0x5110 <= __SUNPRO_C)
#  define _Noreturn __attribute__ ((__noreturn__))
# elif 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn
# endif
#endif
