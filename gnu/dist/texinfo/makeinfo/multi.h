/*	$NetBSD: multi.h,v 1.1.1.1 2004/07/12 23:26:49 wiz Exp $	*/

/* multi.h -- declarations for multi.c.
   Id: multi.h,v 1.1 2004/02/13 21:30:41 dirt Exp

   Copyright (C) 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   */

#ifndef MULTI_H
#define MULTI_H

extern void do_multitable (void);
extern void end_multitable (void);
extern int multitable_item (void);

#endif /* !MULTI_H */
