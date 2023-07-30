/* Implementation header.

   Copyright (C) 2022 Free Software Foundation, Inc.

   This file is part of libsframe.

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

#ifndef _SFRAME_IMPL_H
#define _SFRAME_IMPL_H

#include "sframe-api.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#include <assert.h>
#define sframe_assert(expr) (assert (expr))

struct sframe_decoder_ctx
{
  sframe_header sfd_header;	      /* SFrame header.  */
  uint32_t *sfd_funcdesc;	      /* SFrame function desc entries table.  */
  void *sfd_fres;		      /* SFrame FRE table.  */
  int sfd_fre_nbytes;		      /* Number of bytes needed for SFrame FREs.  */
};

struct sframe_encoder_ctx
{
  sframe_header sfe_header;		/* SFrame header.  */
  uint32_t *sfe_funcdesc;		/* SFrame function desc entries table.  */
  sframe_frame_row_entry *sfe_fres;	/* SFrame FRE table.  */
  uint32_t sfe_fre_nbytes;		/* Number of bytes needed for SFrame FREs.  */
  char *sfe_data;			/* SFrame data buffer.  */
  size_t sfe_data_size;			/* Size of the SFrame data buffer.  */
};

#ifdef  __cplusplus
}
#endif

#endif /* _SFRAME_IMPL_H */
