/* Target wait definitions and prototypes.

   Copyright (C) 1990-2016 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef WAIT_H
#define WAIT_H

/* Options that can be passed to target_wait.  */

/* Return immediately if there's no event already queued.  If this
   options is not requested, target_wait blocks waiting for an
   event.  */
#define TARGET_WNOHANG 1

#endif /* WAIT_H */
