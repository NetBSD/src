/* Recoding functions.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Convert a string INPUT of INPUT_LEN bytes containing Serbian input
   to Latin script (not Latin language :-)), converting Cyrillic letters to
   Latin letters.
   Store the freshly allocated result in *OUTPUT_P and its length (in bytes)
   in *OUTPUT_LEN_P.
   Input and output are in UTF-8 encoding.  */
extern void serbian_to_latin (const char *input, size_t input_len,
			      char **output_p, size_t *output_len_p);

#ifdef __cplusplus
}
#endif

