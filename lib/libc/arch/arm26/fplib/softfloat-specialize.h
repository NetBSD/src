/*	$NetBSD: softfloat-specialize.h,v 1.1 2000/05/09 21:55:46 bjh21 Exp $	*/

/*
===============================================================================

This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
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

static flag float32_is_signaling_nan( float32 a );
static flag float64_is_signaling_nan( float64 a );

/*
-------------------------------------------------------------------------------
Underflow tininess-detection mode.  (Statically initialized to default.)
-------------------------------------------------------------------------------
*/
static flag float_detect_tininess = float_tininess_after_rounding;

/*
-------------------------------------------------------------------------------
Raises the exceptions specified by `flags'.  Floating-point traps can be
defined here if desired.  It is currently not possible for such a trap to
substitute a result value.  If traps are not implemented, this routine
should be simply `float_exception_flags |= flags;'.
-------------------------------------------------------------------------------
*/
void float_raise( uint8 flags )
{

    float_exception_flags |= flags;

}

/*
-------------------------------------------------------------------------------
The pattern for a default generated single-precision NaN.
-------------------------------------------------------------------------------
*/
enum {
    float32_default_nan = 0xFFC00000
};

/*
-------------------------------------------------------------------------------
The pattern for a default generated double-precision NaN.  The `high' and
`low' words hold the most- and least-significant bits, respectively.
-------------------------------------------------------------------------------
*/
enum {
    float64_default_nan_high = 0xFFF80000,
    float64_default_nan_low  = 0x00000000
};

/*
-------------------------------------------------------------------------------
Returns true if the single-precision floating-point value `a' is a signaling
NaN; otherwise returns false.
-------------------------------------------------------------------------------
*/
static flag float32_is_signaling_nan( float32 a )
{

    return ( ( a & 0x7FC00000 ) == 0x7F800000 ) && ( a & 0x003FFFFF );

}

/*
-------------------------------------------------------------------------------
Returns true if the double-precision floating-point value `a' is a signaling
NaN; otherwise returns false.
-------------------------------------------------------------------------------
*/
static flag float64_is_signaling_nan( float64 a )
{

    return
           ( ( a.high & 0x7FF80000 ) == 0x7FF00000 )
        && ( ( a.high & 0x0007FFFF ) || a.low );

}

/*
-------------------------------------------------------------------------------
Converts a single-precision NaN `a' to a double-precision quiet NaN.  If `a'
is a signaling NaN, the invalid exception is raised.
-------------------------------------------------------------------------------
*/
static float64 float32ToFloat64NaN( float32 a )
{
    float64 z;

    if ( float32_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
    z.low = float64_default_nan_low;
    z.high = float64_default_nan_high;
    return z;

}

/*
-------------------------------------------------------------------------------
Converts a double-precision NaN `a' to a single-precision quiet NaN.  If `a'
is a signaling NaN, the invalid exception is raised.
-------------------------------------------------------------------------------
*/
static float32 float64ToFloat32NaN( float64 a )
{

    if ( float64_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
    return float32_default_nan;

}

/*
-------------------------------------------------------------------------------
Takes two single-precision floating-point values `a' and `b', one of which
is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
signaling NaN, the invalid exception is raised.
-------------------------------------------------------------------------------
*/
static float32 propagateFloat32NaN( float32 a, float32 b )
{

    if ( float32_is_signaling_nan( a ) || float32_is_signaling_nan( b ) ) {
        float_raise( float_flag_invalid );
    }
    return float32_default_nan;

}

/*
-------------------------------------------------------------------------------
Takes two double-precision floating-point values `a' and `b', one of which
is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
signaling NaN, the invalid exception is raised.
-------------------------------------------------------------------------------
*/
static float64 propagateFloat64NaN( float64 a, float64 b )
{
    float64 z;

    if ( float64_is_signaling_nan( a ) || float64_is_signaling_nan( b ) ) {
        float_raise( float_flag_invalid );
    }
    z.low = float64_default_nan_low;
    z.high = float64_default_nan_high;
    return z;

}

