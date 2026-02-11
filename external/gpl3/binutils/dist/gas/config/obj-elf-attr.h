/* Object attributes parsing.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef _OBJ_ELF_ATTR_H
#define _OBJ_ELF_ATTR_H

#include "as.h"

#ifndef TC_OBJ_ATTR_v1
#define TC_OBJ_ATTR_v1 0
#endif
#ifndef TC_OBJ_ATTR_v2
#define TC_OBJ_ATTR_v2 0
#endif

#if (TC_OBJ_ATTR_v1 || TC_OBJ_ATTR_v2)
#define TC_OBJ_ATTR 1
#endif

#ifdef TC_OBJ_ATTR

#if (TC_OBJ_ATTR_v1)
extern void oav1_attr_info_init (void);
extern void oav1_attr_info_exit (void);
extern bool oav1_attr_seen (obj_attr_vendor_t, obj_attr_tag_t);
#endif /* TC_OBJ_ATTR_v1 */

/* Object attributes parsers.  */

extern obj_attr_tag_t obj_attr_process_attribute (obj_attr_vendor_t);
#if (TC_OBJ_ATTR_v2)
extern void obj_attr_process_subsection (void);
#endif /* (TC_OBJ_ATTR_v2) */

extern void obj_elf_gnu_attribute (int);
#if (TC_OBJ_ATTR_v2)
extern void obj_elf_gnu_subsection (int);
#endif /* TC_OBJ_ATTR_v2 */

#endif /* TC_OBJ_ATTR */

#endif /* _OBJ_ELF_ATTR_H */
