/* CRIS v10 simulator support code
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.
   Contributed by Axis Communications.

This file is part of the GNU simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* The infrastructure is based on that of i960.c.  */

#define WANT_CPU_CRISV10F

#define BASENUM 10
#include "cris-tmpl.c"

#if WITH_PROFILE_MODEL_P

/* Model function for u-multiply unit.  */

int
MY (XCONCAT3 (f_model_crisv,BASENUM,
	      _u_multiply)) (SIM_CPU *current_cpu ATTRIBUTE_UNUSED,
			     const IDESC *idesc ATTRIBUTE_UNUSED,
			     int unit_num ATTRIBUTE_UNUSED,
			     int referenced ATTRIBUTE_UNUSED)
{
  return 1;
}

#endif /* WITH_PROFILE_MODEL_P */
