/* $NetBSD: namespace.h,v 1.5 2013/01/28 06:26:20 matt Exp $ */

#define atan2 _atan2
#define atan2f _atan2f
#define atan2l _atan2l
#define hypot _hypot
#define hypotf _hypotf
#define hypotl _hypotl

#define exp _exp
#define expf _expf
#define expl _expl
#define log _log
#define logf _logf
#define logl _logl

#if 0 /* not yet - need to review use in machdep code first */
#define sin _sin
#define sinf _sinf
#define cos _cos
#define cosf _cosf
#define finite _finite
#define finitef _finitef
#endif /* notyet */
#define sinh _sinh
#define sinhf _sinhf
#define sinhl _sinhl
#define cosh _cosh
#define coshf _coshf
#define coshl _coshl
#define asin _asin
#define asinf _asinf
#define asinl _asinl

#define casin _casin
#define casinf _casinf
#define casinl _casinl
#define catan _catan
#define catanf _catanf
#define catanl _catanl

#define scalbn _scalbn
#define scalbnf _scalbnf
#define scalbnl _scalbnl
#define scalbln _scalbln
#define scalblnf _scalblnf
#define scalblnl _scalblnl
