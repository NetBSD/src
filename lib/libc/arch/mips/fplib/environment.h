
/*
===============================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 1a.

Written by John R. Hauser.  This work was made possible by the International
Computer Science Institute, located at Suite 600, 1947 Center Street,
Berkeley, California 94704.  Funding was provided in part by the National
Science Foundation under grant MIP-9311980.  The original version of
this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the web page `http://www.cs.berkeley.edu/~jhauser/
softfloat.html'.

THIS PACKAGE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS PACKAGE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS PACKAGE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY AND ALL
LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these three paragraphs for those parts of
this code that are retained.

===============================================================================
*/

/*
-------------------------------------------------------------------------------
Include common integer types and flags.
-------------------------------------------------------------------------------
*/
#include "hpcmips-gcc.h"

/*
-------------------------------------------------------------------------------
The word `INLINE' appears before all routines that should be inlined.  If
a compiler does not support inlining, this macro should be defined to be
`static'.
-------------------------------------------------------------------------------
*/
#define INLINE extern inline

#ifdef __lint
#undef INLINE
#define INLINE	/* for lint */
#endif

/*
-------------------------------------------------------------------------------
Move private identifiers with external linkage into implementation namespace.
-------------------------------------------------------------------------------
*/
#define float32_eq			_mips_float32_eq
#define float32_eq_signaling		_mips_float32_eq_signaling
#define float32_is_nan			_mips_float32_is_nan
#define float32_is_signaling_nan	_mips_float32_is_signaling_nan
#define float32_le			_mips_float32_le
#define float32_le_quiet		_mips_float32_le_quiet
#define float32_lt			_mips_float32_lt
#define float32_lt_quiet		_mips_float32_lt_quiet
#define float32_rem			_mips_float32_rem
#define float32_round_to_int		_mips_float32_round_to_int
#define float32_sqrt			_mips_float32_sqrt
#define float32_to_int32		_mips_float32_to_int32
#define float32_to_int64		_mips_float32_to_int64
#define float32_to_int64_round_to_zero	_mips_float32_to_int64_round_to_zero
#define float64_eq			_mips_float64_eq
#define float64_eq_signaling		_mips_float64_eq_signaling
#define float64_is_nan			_mips_float64_is_nan
#define float64_is_signaling_nan	_mips_float64_is_signaling_nan
#define float64_le			_mips_float64_le
#define float64_le_quiet		_mips_float64_le_quiet
#define float64_lt			_mips_float64_lt
#define float64_lt_quiet		_mips_float64_lt_quiet
#define float64_rem			_mips_float64_rem
#define float64_round_to_int		_mips_float64_round_to_int
#define float64_sqrt			_mips_float64_sqrt
#define float64_to_int32		_mips_float64_to_int32
#define float64_to_int64		_mips_float64_to_int64
#define float64_to_int64_round_to_zero	_mips_float64_to_int64_round_to_zero
#define float_detect_tininess		_mips_float_detect_tininess
#define float_exception_flags		_mips_float_exception_flags
#define float_raise			_mips_float_raise
#define float_rounding_mode		_mips_float_rounding_mode
#define int64_to_float32		_mips_int64_to_float32
#define int64_to_float64		_mips_int64_to_float64
