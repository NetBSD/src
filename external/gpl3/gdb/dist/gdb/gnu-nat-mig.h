/* Common things used by the various *gnu-nat.c files
   Copyright (C) 2020-2023 Free Software Foundation, Inc.

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

#ifndef GNU_NAT_MIG_H
#define GNU_NAT_MIG_H

#include <mach/boolean.h>
#include <mach/message.h>

boolean_t exc_server (mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);
boolean_t msg_reply_server (mach_msg_header_t *InHeadP,
			    mach_msg_header_t *OutHeadP);
boolean_t notify_server (mach_msg_header_t *InHeadP,
			 mach_msg_header_t *OutHeadP);
boolean_t process_reply_server (mach_msg_header_t *InHeadP,
				mach_msg_header_t *OutHeadP);

#endif /* GNU_NAT_MIG_H */
