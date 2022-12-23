/* Copyright (C) 2021 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef _I18N_H
#define _I18N_H

#include <libintl.h>

#define GTXT(x) gettext(x)      /* x - string literal to be i18n-ed */
#define PTXT(x) gettext(x)      /* x - expression to be i18n-ed */
#define STXT(x) ((char *) (x))  /* x - static string literal to be i18n-ed */
#define NTXT(x) ((char *) (x))  /* x - string literal not to be i18n-ed */

#ifdef __cplusplus
extern "C"
{
#endif
  void init_locale (char *Path);
#ifdef __cplusplus
}
#endif

#endif /* _I18N_H */
