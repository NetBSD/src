/*	$NetBSD: softfloat.h,v 1.1 2000/05/09 21:55:46 bjh21 Exp $	*/

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

#include <machine/ieeefp.h>

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point types.
-------------------------------------------------------------------------------
*/
typedef unsigned int float32;
typedef struct {
    unsigned int high;
    unsigned int low;
} float64;

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point rounding mode and exception flags.
-------------------------------------------------------------------------------
*/
extern unsigned char float_rounding_mode;
extern unsigned char float_exception_flags;

/*
-------------------------------------------------------------------------------
Floating-point rounding mode codes.
-------------------------------------------------------------------------------
*/
enum {
    float_round_nearest_even = FP_RN,
    float_round_up           = FP_RP,
    float_round_down         = FP_RM,
    float_round_to_zero      = FP_RZ
};

/*
-------------------------------------------------------------------------------
Floating-point exception flag masks.
-------------------------------------------------------------------------------
*/
enum {
    float_flag_invalid   = FP_X_INV,
    float_flag_divbyzero = FP_X_DZ,
    float_flag_overflow  = FP_X_OFL,
    float_flag_underflow = FP_X_UFL,
    float_flag_inexact   = FP_X_IMP
};

/*
-------------------------------------------------------------------------------
Floating-point underflow tininess-detection mode.
-------------------------------------------------------------------------------
*/
extern char float_detect_tininess;
enum {
    float_tininess_before_rounding = 0,
    float_tininess_after_rounding  = 1
};

/*
-------------------------------------------------------------------------------
Routine to raise any or all of the software IEC/IEEE floating-point
exception flags.
-------------------------------------------------------------------------------
*/
void float_raise( unsigned char );

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point conversion routines.
-------------------------------------------------------------------------------
*/
float32 int32_to_float32( int );
float64 int32_to_float64( int );
#if 0 /* unused */
static int float32_to_int32( float32 );
#endif
int float32_to_int32_round_to_zero( float32 );
float64 float32_to_float64( float32 );
#if 0 /* unused */
static int float64_to_int32( float64 );
#endif
int float64_to_int32_round_to_zero( float64 );
float32 float64_to_float32( float64 );
unsigned int float64_to_uint32_round_to_zero( float64 a );
unsigned int float32_to_uint32_round_to_zero( float32 a );

/*
-------------------------------------------------------------------------------
Software IEC/IEEE single-precision operations.
-------------------------------------------------------------------------------
*/
#if 0 /* unused */
static float32 float32_round_to_int( float32 );
#endif
float32 float32_add( float32, float32 );
float32 float32_sub( float32, float32 );
float32 float32_mul( float32, float32 );
float32 float32_div( float32, float32 );
#if 0 /* unused */
static float32 float32_rem( float32, float32 );
static float32 float32_sqrt( float32 );
#endif
char float32_eq( float32, float32 );
char float32_le( float32, float32 );
char float32_lt( float32, float32 );
#if 0 /* unused */
static char float32_eq_signaling( float32, float32 );
static char float32_le_quiet( float32, float32 );
static char float32_lt_quiet( float32, float32 );
#endif

/*
-------------------------------------------------------------------------------
Software IEC/IEEE double-precision operations.
-------------------------------------------------------------------------------
*/
#if 0 /* unused */
static float64 float64_round_to_int( float64 );
#endif
float64 float64_add( float64, float64 );
float64 float64_sub( float64, float64 );
float64 float64_mul( float64, float64 );
float64 float64_div( float64, float64 );
#if 0 /* unused */
static float64 float64_rem( float64, float64 );
static float64 float64_sqrt( float64 );
#endif
char float64_eq( float64, float64 );
char float64_le( float64, float64 );
char float64_lt( float64, float64 );
#if 0 /* unused */
char float64_eq_signaling( float64, float64 );
#endif
char float64_le_quiet( float64, float64 );
char float64_lt_quiet( float64, float64 );
