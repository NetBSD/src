
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

