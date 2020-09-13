/* BFD support for C-SKY processors.
   Copyright (C) 1994-2019 Free Software Foundation, Inc.
   Contributed by C-SKY Microsystems and Mentor Graphics.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define N(NUMBER, PRINT, ISDEFAULT, NEXT)  \
{                                                                      \
  32,                     /* 32 bits in a word */                      \
  32,                     /* 32 bits in an address */                  \
  8,                      /* 8 bits in a byte */                       \
  bfd_arch_csky,          /* Architecture */                           \
  NUMBER,                 /* Machine number */                         \
  "csky",                 /* Architecture name */                      \
  PRINT,                  /* Printable name */                         \
  3,                      /* Section align power */                    \
  ISDEFAULT,              /* Is this the default architecture ? */     \
  bfd_default_compatible, /* Architecture comparison function */       \
  bfd_default_scan,       /* String to architecture conversion */      \
  bfd_arch_default_fill,                                               \
  NEXT                    /* Next in list */                           \
}

static const bfd_arch_info_type arch_info_struct[] =
{
  /* ABI v1 processors. */
  N (bfd_mach_ck510,   "csky:ck510",    FALSE, & arch_info_struct[1]),
  N (bfd_mach_ck610,   "csky:ck610",    FALSE, & arch_info_struct[2]),

  /* ABI v2 processors.  */
  N (bfd_mach_ck801,   "csky:ck801",    FALSE, & arch_info_struct[3]),
  N (bfd_mach_ck802,   "csky:ck802",    FALSE, & arch_info_struct[4]),
  N (bfd_mach_ck803,   "csky:ck803",    FALSE, & arch_info_struct[5]),
  N (bfd_mach_ck807,   "csky:ck807",    FALSE, & arch_info_struct[6]),
  N (bfd_mach_ck810,   "csky:ck810",    FALSE, & arch_info_struct[7]),
  N (bfd_mach_ck_unknown, "csky:any",   FALSE, NULL)
};

const bfd_arch_info_type bfd_csky_arch =
  N (0, "csky", TRUE, & arch_info_struct[0]);
