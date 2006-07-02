/* BSD Kernel Data Access Library (libkvm) interface.

   Copyright (C) 2004 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#ifndef BSD_KVM_H
#define BSD_KVM_H

struct pcb;
struct regcache;

/* Add the libkvm interface to the list of all possible targets and
   register CUPPLY_PCB as the architecture-specific process control
   block interpreter.  */

extern void
  bsd_kvm_add_target (int (*supply_pcb)(struct regcache *, struct pcb *));

#endif /* bsd-kvm.h */
