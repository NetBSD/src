/* collection of junk waiting time to sort out
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef FR30_SIM_H
#define FR30_SIM_H

/* gdb register numbers */
#define PC_REGNUM	16
#define PS_REGNUM	17
#define TBR_REGNUM	18
#define RP_REGNUM	19
#define SSP_REGNUM	20
#define USP_REGNUM	21
#define MDH_REGNUM	22
#define MDL_REGNUM	23

extern BI fr30bf_h_sbit_get_handler (SIM_CPU *);
extern void fr30bf_h_sbit_set_handler (SIM_CPU *, BI);

extern UQI fr30bf_h_ccr_get_handler (SIM_CPU *);
extern void fr30bf_h_ccr_set_handler (SIM_CPU *, UQI);

extern UQI fr30bf_h_scr_get_handler (SIM_CPU *);
extern void fr30bf_h_scr_set_handler (SIM_CPU *, UQI);

extern UQI fr30bf_h_ilm_get_handler (SIM_CPU *);
extern void fr30bf_h_ilm_set_handler (SIM_CPU *, UQI);

extern USI fr30bf_h_ps_get_handler (SIM_CPU *);
extern void fr30bf_h_ps_set_handler (SIM_CPU *, USI);

extern SI fr30bf_h_dr_get_handler (SIM_CPU *, UINT);
extern void fr30bf_h_dr_set_handler (SIM_CPU *, UINT, SI);

#define GETTWI GETTSI
#define SETTWI SETTSI

/* Hardware/device support.
   ??? Will eventually want to move device stuff to config files.  */

/* Special purpose traps.  */
#define TRAP_SYSCALL	10
#define TRAP_BREAKPOINT	9

/* Support for the MCCR register (Cache Control Register) is needed in order
   for overlays to work correctly with the scache: cached instructions need
   to be flushed when the instruction space is changed at runtime.  */

/* Cache Control Register */
#define MCCR_ADDR 0xffffffff
#define MCCR_CP 0x80
/* not supported */
#define MCCR_CM0 2
#define MCCR_CM1 1

/* Serial device addresses.  */
/* These are the values for the MSA2000 board.
   ??? Will eventually need to move this to a config file.  */
#define UART_INCHAR_ADDR	0xff004009
#define UART_OUTCHAR_ADDR	0xff004007
#define UART_STATUS_ADDR	0xff004002

#define UART_INPUT_READY	0x4
#define UART_OUTPUT_READY	0x1

/* Start address and length of all device support.  */
#define FR30_DEVICE_ADDR	0xff000000
#define FR30_DEVICE_LEN		0x00ffffff

/* sim_core_attach device argument.  */
extern device fr30_devices;

/* FIXME: Temporary, until device support ready.  */
struct _device { int foo; };

/* Handle the trap insn.  */
USI fr30_int (SIM_CPU *, PCADDR, int);

#endif /* FR30_SIM_H */
