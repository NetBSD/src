/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
 
    */


typedef struct _filter filter;


/* append the filter onto the end of the list */

extern filter *new_filter
(const char *filt,
 filter *filters);


/* returns true if the flags are non empty and some are missing from the filter list */

extern int is_filtered_out
(const char *flags,
 filter *filters);

/* true if the flag is in the list */

extern int it_is
(const char *flag,
 const char *flags);

