/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>
    Copyright (C) 1997, 1998, Free Software Foundation

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#ifndef _SIM_MAIN_H_
#define _SIM_MAIN_H_

/* This simulator suports watchpoints */
#define WITH_WATCHPOINTS 1

#include "sim-basics.h"
#include "sim-signal.h"

/* needed */
typedef address_word sim_cia;
#define INVALID_INSTRUCTION_ADDRESS ((address_word) 0 - 1)

/* This simulator doesn't cache anything so no saving of context is
   needed during either of a halt or restart */
#define SIM_ENGINE_HALT_HOOK(SD,CPU,CIA) while (0)
#define SIM_ENGINE_RESTART_HOOK(SD,CPU,CIA) while (0)

#include "sim-base.h"

/* These are generated files.  */
#include "itable.h"
#include "s_idecode.h"
#include "l_idecode.h"

#include "cpu.h"
#include "alu.h"


struct sim_state {

  sim_event *pending_interrupt;

  /* the processors proper */
  sim_cpu cpu[MAX_NR_PROCESSORS];
#if (WITH_SMP)
#define STATE_CPU(sd, n) (&(sd)->cpu[n])
#else
#define STATE_CPU(sd, n) (&(sd)->cpu[0])
#endif

  /* The base class.  */
  sim_state_base base;

};


/* deliver an interrupt */
sim_event_handler d30v_interrupt_event;


#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#endif /* _SIM_MAIN_H_ */
