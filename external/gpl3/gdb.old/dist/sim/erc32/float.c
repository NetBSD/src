/* This file is part of SIS (SPARC instruction simulator)

   Copyright (C) 1995-2017 Free Software Foundation, Inc.
   Contributed by Jiri Gaisler, European Space Agency

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file implements the interface between the host and the simulated
   FPU.  IEEE trap handling is done as follows:
     1. In the host, all IEEE traps are masked
     2. After each simulated FPU instruction, check if any exception
        occured by reading the exception bits from the host FPU status
        register (get_accex()).
     3. Propagate any exceptions to the simulated FSR.
     4. Clear host exception bits.
 */

#include "config.h"
#include "sis.h"
#include <fenv.h>

/* This routine should return the accrued exceptions */
int
get_accex()
{
    int fexc, accx;

    fexc = fetestexcept (FE_ALL_EXCEPT);
    accx = 0;
    if (fexc & FE_INEXACT)
        accx |= 1;
    if (fexc & FE_DIVBYZERO)
        accx |= 2;
    if (fexc & FE_UNDERFLOW)
        accx |= 4;
    if (fexc & FE_OVERFLOW)
        accx |= 8;
    if (fexc & FE_INVALID)
        accx |= 0x10;
    return accx;
}

/* How to clear the accrued exceptions */
void
clear_accex()
{
    feclearexcept (FE_ALL_EXCEPT);
}

/* How to map SPARC FSR onto the host */
void
set_fsr(fsr)
uint32 fsr;
{
    int fround;

    fsr >>= 30;
    switch (fsr) {
	case 0: 
	    fround = FE_TONEAREST;
	    break;
	case 1:
	    fround = FE_TOWARDZERO;
	    break;
	case 2:
	    fround = FE_UPWARD;
	    break;
	case 3:
	    fround = FE_DOWNWARD;
	    break;
     }
     fesetround (fround);
}
