/* Disable address space randomization based on inferior personality.

   Copyright (C) 2008-2017 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_PERSONALITY_H
#define NAT_LINUX_PERSONALITY_H

/* Disable the inferior's address space randomization if
   DISABLE_RANDOMIZATION is not zero and if we have
   <sys/personality.h>.  Return a cleanup which, when called, will
   re-enable the inferior's address space randomization.  */

extern struct cleanup *maybe_disable_address_space_randomization
  (int disable_randomization);

#endif /* ! NAT_LINUX_PERSONALITY_H */
