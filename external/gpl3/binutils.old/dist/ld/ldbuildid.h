/* ldbuildid.h -
   Copyright (C) 2013-2016 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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

#ifndef LDBUILDID_H
#define LDBUILDID_H

extern bfd_boolean
validate_build_id_style (const char *);

extern bfd_size_type
compute_build_id_size (const char *);

typedef void (*sum_fn) (const void *, size_t, void *);

typedef bfd_boolean (*checksum_fn) (bfd *,
				    void (*) (const void *, size_t, void *),
				    void *);

extern bfd_boolean
generate_build_id (bfd *, const char *, checksum_fn, unsigned char *, int);

#endif /* LDBUILDID_H */
