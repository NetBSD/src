/* mpc_set_str -- Convert a string into a complex number.

Copyright (C) INRIA, 2009, 2010

This file is part of the MPC Library.

The MPC Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The MPC Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the MPC Library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include <ctype.h>
#include "mpc-impl.h"

int
mpc_set_str (mpc_t z, const char *str, int base, mpc_rnd_t rnd)
{
  char *p;
  int inex;

  inex = mpc_strtoc (z, str, &p, base, rnd);

  if (inex != -1){
     while (isspace ((unsigned char) (*p)))
        p++;
     if (*p == '\0')
        return inex;
  }

  mpfr_set_nan (MPC_RE (z));
  mpfr_set_nan (MPC_IM (z));
  return -1;
}
