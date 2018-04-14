/* tc-avr.c -- Assembler code for the ATMEL AVR

   Copyright (C) 1999-2016 Free Software Foundation, Inc.
   Contributed by Denis Chertykov <denisc@overta.ru>

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "as.h"
#include "safe-ctype.h"
#include "subsegs.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"
#include "elf/avr.h"
#include "elf32-avr.h"

/* For building a linked list of AVR_PROPERTY_RECORD structures.  */
struct avr_property_record_link
{
  struct avr_property_record record;
  struct avr_property_record_link *next;
};

struct avr_opcodes_s
{
  const char *        name;
  const char *        constraints;
  const char *        opcode;
  int           insn_size;		/* In words.  */
  int           isa;
  unsigned int  bin_opcode;
};

#define AVR_INSN(NAME, CONSTR, OPCODE, SIZE, ISA, BIN) \
{#NAME, CONSTR, OPCODE, SIZE, ISA, BIN},

struct avr_opcodes_s avr_opcodes[] =
{
  #include "opcode/avr.h"
  {NULL, NULL, NULL, 0, 0, 0}
};

const char comment_chars[] = ";";
const char line_comment_chars[] = "#";
const char line_separator_chars[] = "$";

const char *md_shortopts = "m:";
struct mcu_type_s
{
  const char *name;
  int isa;
  int mach;
};

/* XXX - devices that don't seem to exist (renamed, replaced with larger
   ones, or planned but never produced), left here for compatibility.  */

static struct mcu_type_s mcu_types[] =
{
  {"avr1",       AVR_ISA_AVR1,    bfd_mach_avr1},
/* TODO: insruction set for avr2 architecture should be AVR_ISA_AVR2,
 but set to AVR_ISA_AVR25 for some following version
 of GCC (from 4.3) for backward compatibility.  */
  {"avr2",       AVR_ISA_AVR25,   bfd_mach_avr2},
  {"avr25",      AVR_ISA_AVR25,   bfd_mach_avr25},
/* TODO: insruction set for avr3 architecture should be AVR_ISA_AVR3,
 but set to AVR_ISA_AVR3_ALL for some following version
 of GCC (from 4.3) for backward compatibility.  */
  {"avr3",       AVR_ISA_AVR3_ALL, bfd_mach_avr3},
  {"avr31",      AVR_ISA_AVR31,   bfd_mach_avr31},
  {"avr35",      AVR_ISA_AVR35,   bfd_mach_avr35},
  {"avr4",       AVR_ISA_AVR4,    bfd_mach_avr4},
/* TODO: insruction set for avr5 architecture should be AVR_ISA_AVR5,
 but set to AVR_ISA_AVR51 for some following version
 of GCC (from 4.3) for backward compatibility.  */
  {"avr5",       AVR_ISA_AVR51,   bfd_mach_avr5},
  {"avr51",      AVR_ISA_AVR51,   bfd_mach_avr51},
  {"avr6",       AVR_ISA_AVR6,    bfd_mach_avr6},
  {"avrxmega1",  AVR_ISA_XMEGA,   bfd_mach_avrxmega1},
  {"avrxmega2",  AVR_ISA_XMEGA,   bfd_mach_avrxmega2},
  {"avrxmega3",  AVR_ISA_XMEGA,   bfd_mach_avrxmega3},
  {"avrxmega4",  AVR_ISA_XMEGA,   bfd_mach_avrxmega4},
  {"avrxmega5",  AVR_ISA_XMEGA,   bfd_mach_avrxmega5},
  {"avrxmega6",  AVR_ISA_XMEGA,   bfd_mach_avrxmega6},
  {"avrxmega7",  AVR_ISA_XMEGA,   bfd_mach_avrxmega7},
  {"avrtiny",    AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"at90s1200",  AVR_ISA_1200,    bfd_mach_avr1},
  {"attiny11",   AVR_ISA_AVR1,    bfd_mach_avr1},
  {"attiny12",   AVR_ISA_AVR1,    bfd_mach_avr1},
  {"attiny15",   AVR_ISA_AVR1,    bfd_mach_avr1},
  {"attiny28",   AVR_ISA_AVR1,    bfd_mach_avr1},
  {"at90s2313",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"at90s2323",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"at90s2333",  AVR_ISA_AVR2,    bfd_mach_avr2}, /* XXX -> 4433 */
  {"at90s2343",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"attiny22",   AVR_ISA_AVR2,    bfd_mach_avr2}, /* XXX -> 2343 */
  {"attiny26",   AVR_ISA_2xxe,    bfd_mach_avr2},
  {"at90s4414",  AVR_ISA_AVR2,    bfd_mach_avr2}, /* XXX -> 8515 */
  {"at90s4433",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"at90s4434",  AVR_ISA_AVR2,    bfd_mach_avr2}, /* XXX -> 8535 */
  {"at90s8515",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"at90c8534",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"at90s8535",  AVR_ISA_AVR2,    bfd_mach_avr2},
  {"ata5272",    AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny13",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny13a",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny2313", AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny2313a",AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny24",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny24a",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny4313", AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny44",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny44a",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny84",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny84a",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny25",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny45",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny85",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny261",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny261a", AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny461",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny461a", AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny861",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny861a", AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny87",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny43u",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny48",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny88",   AVR_ISA_AVR25,   bfd_mach_avr25},
  {"attiny828",  AVR_ISA_AVR25,   bfd_mach_avr25},
  {"at86rf401",  AVR_ISA_RF401,   bfd_mach_avr25},
  {"at43usb355", AVR_ISA_AVR3,    bfd_mach_avr3},
  {"at76c711",   AVR_ISA_AVR3,    bfd_mach_avr3},
  {"atmega103",  AVR_ISA_AVR31,   bfd_mach_avr31},
  {"at43usb320", AVR_ISA_AVR31,   bfd_mach_avr31},
  {"attiny167",  AVR_ISA_AVR35,   bfd_mach_avr35},
  {"at90usb82",  AVR_ISA_AVR35,   bfd_mach_avr35},
  {"at90usb162", AVR_ISA_AVR35,   bfd_mach_avr35},
  {"ata5505",    AVR_ISA_AVR35,   bfd_mach_avr35},
  {"atmega8u2",  AVR_ISA_AVR35,   bfd_mach_avr35},
  {"atmega16u2", AVR_ISA_AVR35,   bfd_mach_avr35},
  {"atmega32u2", AVR_ISA_AVR35,   bfd_mach_avr35},
  {"attiny1634", AVR_ISA_AVR35,   bfd_mach_avr35},
  {"atmega8",    AVR_ISA_M8,      bfd_mach_avr4},
  {"ata6289",    AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega8a",   AVR_ISA_M8,      bfd_mach_avr4},
  {"ata6285",    AVR_ISA_AVR4,    bfd_mach_avr4},
  {"ata6286",    AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega48",   AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega48a",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega48pa", AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega48p",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega88",   AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega88a",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega88p",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega88pa", AVR_ISA_AVR4,    bfd_mach_avr4},
  {"atmega8515", AVR_ISA_M8,      bfd_mach_avr4},
  {"atmega8535", AVR_ISA_M8,      bfd_mach_avr4},
  {"atmega8hva", AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm1",   AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm2",   AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm2b",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm3",   AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm3b",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm81",  AVR_ISA_AVR4,    bfd_mach_avr4},
  {"at90pwm161", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"ata5790",    AVR_ISA_AVR5,    bfd_mach_avr5},
  {"ata5795",    AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16",   AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16a",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega161",  AVR_ISA_M161,    bfd_mach_avr5},
  {"atmega162",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega163",  AVR_ISA_M161,    bfd_mach_avr5},
  {"atmega164a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega164p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega164pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega165",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega165a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega165p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega165pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega168",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega168a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega168p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega168pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega169",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega169a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega169p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega169pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32",   AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32a",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega323",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega324a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega324p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega324pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega325",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega325a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega325p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega325pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3250", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3250a",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3250p",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3250pa",AVR_ISA_AVR5,   bfd_mach_avr5},
  {"atmega328",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega328p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega329",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega329a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega329p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega329pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3290", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3290a",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3290p",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega3290pa",AVR_ISA_AVR5,   bfd_mach_avr5},
  {"atmega406",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega64rfr2", AVR_ISA_AVR5,  bfd_mach_avr5},
  {"atmega644rfr2",AVR_ISA_AVR5,  bfd_mach_avr5},
  {"atmega64",   AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega64a",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega640",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega644",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega644a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega644p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega644pa",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega645",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega645a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega645p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega649",  AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega649a", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega649p", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6450", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6450a",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6450p",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6490", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6490a",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega6490p",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega64rfr2",AVR_ISA_AVR5,   bfd_mach_avr5},
  {"atmega644rfr2",AVR_ISA_AVR5,  bfd_mach_avr5},
  {"atmega16hva",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16hva2",AVR_ISA_AVR5,   bfd_mach_avr5},
  {"atmega16hvb",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16hvbrevb",AVR_ISA_AVR5,bfd_mach_avr5},
  {"atmega32hvb",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32hvbrevb",AVR_ISA_AVR5,bfd_mach_avr5},
  {"atmega64hve",AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90can32" , AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90can64" , AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90pwm161", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90pwm216", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90pwm316", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32c1", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega64c1", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16m1", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32m1", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega64m1", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega16u4", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32u4", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega32u6", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90usb646", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90usb647", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at90scr100", AVR_ISA_AVR5,    bfd_mach_avr5},
  {"at94k",      AVR_ISA_94K,     bfd_mach_avr5},
  {"m3000",      AVR_ISA_AVR5,    bfd_mach_avr5},
  {"atmega128",  AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega128a", AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega1280", AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega1281", AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega1284", AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega1284p",AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega128rfa1",AVR_ISA_AVR51, bfd_mach_avr51},
  {"atmega128rfr2",AVR_ISA_AVR51, bfd_mach_avr51},
  {"atmega1284rfr2",AVR_ISA_AVR51, bfd_mach_avr51},
  {"at90can128", AVR_ISA_AVR51,   bfd_mach_avr51},
  {"at90usb1286",AVR_ISA_AVR51,   bfd_mach_avr51},
  {"at90usb1287",AVR_ISA_AVR51,   bfd_mach_avr51},
  {"atmega2560", AVR_ISA_AVR6,    bfd_mach_avr6},
  {"atmega2561", AVR_ISA_AVR6,    bfd_mach_avr6},
  {"atmega256rfr2", AVR_ISA_AVR6, bfd_mach_avr6},
  {"atmega2564rfr2", AVR_ISA_AVR6, bfd_mach_avr6},
  {"atxmega16a4", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega16a4u",AVR_ISA_XMEGAU, bfd_mach_avrxmega2},
  {"atxmega16c4", AVR_ISA_XMEGAU, bfd_mach_avrxmega2},
  {"atxmega16d4", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega32a4", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega32a4u",AVR_ISA_XMEGAU, bfd_mach_avrxmega2},
  {"atxmega32c4", AVR_ISA_XMEGAU, bfd_mach_avrxmega2},
  {"atxmega32d4", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega32e5", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega16e5", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega8e5",  AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega32x1", AVR_ISA_XMEGA,  bfd_mach_avrxmega2},
  {"atxmega64a3", AVR_ISA_XMEGA,  bfd_mach_avrxmega4},
  {"atxmega64a3u",AVR_ISA_XMEGAU, bfd_mach_avrxmega4},
  {"atxmega64a4u",AVR_ISA_XMEGAU, bfd_mach_avrxmega4},
  {"atxmega64b1", AVR_ISA_XMEGAU, bfd_mach_avrxmega4},
  {"atxmega64b3", AVR_ISA_XMEGAU, bfd_mach_avrxmega4},
  {"atxmega64c3", AVR_ISA_XMEGAU, bfd_mach_avrxmega4},
  {"atxmega64d3", AVR_ISA_XMEGA,  bfd_mach_avrxmega4},
  {"atxmega64d4", AVR_ISA_XMEGA,  bfd_mach_avrxmega4},
  {"atxmega64a1", AVR_ISA_XMEGA,  bfd_mach_avrxmega5},
  {"atxmega64a1u",AVR_ISA_XMEGAU, bfd_mach_avrxmega5},
  {"atxmega128a3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega128a3u",AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega128b1", AVR_ISA_XMEGAU, bfd_mach_avrxmega6},
  {"atxmega128b3", AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega128c3", AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega128d3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega128d4", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega192a3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega192a3u",AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega192c3", AVR_ISA_XMEGAU, bfd_mach_avrxmega6},
  {"atxmega192d3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega256a3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega256a3u",AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega256a3b",AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega256a3bu",AVR_ISA_XMEGAU, bfd_mach_avrxmega6},
  {"atxmega256c3", AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega256d3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega384c3", AVR_ISA_XMEGAU,bfd_mach_avrxmega6},
  {"atxmega384d3", AVR_ISA_XMEGA, bfd_mach_avrxmega6},
  {"atxmega128a1", AVR_ISA_XMEGA, bfd_mach_avrxmega7},
  {"atxmega128a1u", AVR_ISA_XMEGAU, bfd_mach_avrxmega7},
  {"atxmega128a4u", AVR_ISA_XMEGAU, bfd_mach_avrxmega7},
  {"attiny4",      AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"attiny5",      AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"attiny9",      AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"attiny10",     AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"attiny20",     AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {"attiny40",     AVR_ISA_AVRTINY, bfd_mach_avrtiny},
  {NULL, 0, 0}
};


/* Current MCU type.  */
static struct mcu_type_s   default_mcu = {"avr2", AVR_ISA_AVR2, bfd_mach_avr2};
static struct mcu_type_s   specified_mcu;
static struct mcu_type_s * avr_mcu = & default_mcu;

/* AVR target-specific switches.  */
struct avr_opt_s
{
  int all_opcodes;  /* -mall-opcodes: accept all known AVR opcodes.  */
  int no_skip_bug;  /* -mno-skip-bug: no warnings for skipping 2-word insns.  */
  int no_wrap;      /* -mno-wrap: reject rjmp/rcall with 8K wrap-around.  */
  int no_link_relax;   /* -mno-link-relax / -mlink-relax: generate (or not)
                          relocations for linker relaxation.  */
};

static struct avr_opt_s avr_opt = { 0, 0, 0, 0 };

const char EXP_CHARS[] = "eE";
const char FLT_CHARS[] = "dD";

static void avr_set_arch (int);

/* The target specific pseudo-ops which we support.  */
const pseudo_typeS md_pseudo_table[] =
{
  {"arch", avr_set_arch,	0},
  { NULL,	NULL,		0}
};

#define LDI_IMMEDIATE(x) (((x) & 0xf) | (((x) << 4) & 0xf00))

#define EXP_MOD_NAME(i)       exp_mod[i].name
#define EXP_MOD_RELOC(i)      exp_mod[i].reloc
#define EXP_MOD_NEG_RELOC(i)  exp_mod[i].neg_reloc
#define HAVE_PM_P(i)          exp_mod[i].have_pm

struct exp_mod_s
{
  const char *                    name;
  bfd_reloc_code_real_type  reloc;
  bfd_reloc_code_real_type  neg_reloc;
  int                       have_pm;
};

static struct exp_mod_s exp_mod[] =
{
  {"hh8",    BFD_RELOC_AVR_HH8_LDI,    BFD_RELOC_AVR_HH8_LDI_NEG,    1},
  {"pm_hh8", BFD_RELOC_AVR_HH8_LDI_PM, BFD_RELOC_AVR_HH8_LDI_PM_NEG, 0},
  {"hi8",    BFD_RELOC_AVR_HI8_LDI,    BFD_RELOC_AVR_HI8_LDI_NEG,    1},
  {"pm_hi8", BFD_RELOC_AVR_HI8_LDI_PM, BFD_RELOC_AVR_HI8_LDI_PM_NEG, 0},
  {"lo8",    BFD_RELOC_AVR_LO8_LDI,    BFD_RELOC_AVR_LO8_LDI_NEG,    1},
  {"pm_lo8", BFD_RELOC_AVR_LO8_LDI_PM, BFD_RELOC_AVR_LO8_LDI_PM_NEG, 0},
  {"hlo8",   BFD_RELOC_AVR_HH8_LDI,    BFD_RELOC_AVR_HH8_LDI_NEG,    0},
  {"hhi8",   BFD_RELOC_AVR_MS8_LDI,    BFD_RELOC_AVR_MS8_LDI_NEG,    0},
};

/* A union used to store indicies into the exp_mod[] array
   in a hash table which expects void * data types.  */
typedef union
{
  void * ptr;
  int    index;
} mod_index;

/* Opcode hash table.  */
static struct hash_control *avr_hash;

/* Reloc modifiers hash control (hh8,hi8,lo8,pm_xx).  */
static struct hash_control *avr_mod_hash;

#define OPTION_MMCU 'm'
enum options
{
  OPTION_ALL_OPCODES = OPTION_MD_BASE + 1,
  OPTION_NO_SKIP_BUG,
  OPTION_NO_WRAP,
  OPTION_ISA_RMW,
  OPTION_LINK_RELAX,
  OPTION_NO_LINK_RELAX
};

struct option md_longopts[] =
{
  { "mmcu",   required_argument, NULL, OPTION_MMCU        },
  { "mall-opcodes", no_argument, NULL, OPTION_ALL_OPCODES },
  { "mno-skip-bug", no_argument, NULL, OPTION_NO_SKIP_BUG },
  { "mno-wrap",     no_argument, NULL, OPTION_NO_WRAP     },
  { "mrmw",         no_argument, NULL, OPTION_ISA_RMW     },
  { "mlink-relax",  no_argument, NULL, OPTION_LINK_RELAX  },
  { "mno-link-relax",  no_argument, NULL, OPTION_NO_LINK_RELAX  },
  { NULL, no_argument, NULL, 0 }
};

size_t md_longopts_size = sizeof (md_longopts);

/* Display nicely formatted list of known MCU names.  */

static void
show_mcu_list (FILE *stream)
{
  int i, x;

  fprintf (stream, _("Known MCU names:"));
  x = 1000;

  for (i = 0; mcu_types[i].name; i++)
    {
      int len = strlen (mcu_types[i].name);

      x += len + 1;

      if (x < 75)
	fprintf (stream, " %s", mcu_types[i].name);
      else
	{
	  fprintf (stream, "\n  %s", mcu_types[i].name);
	  x = len + 2;
	}
    }

  fprintf (stream, "\n");
}

static inline char *
skip_space (char *s)
{
  while (*s == ' ' || *s == '\t')
    ++s;
  return s;
}

/* Extract one word from FROM and copy it to TO.  */

static char *
extract_word (char *from, char *to, int limit)
{
  char *op_end;
  int size = 0;

  /* Drop leading whitespace.  */
  from = skip_space (from);
  *to = 0;

  /* Find the op code end.  */
  for (op_end = from; *op_end != 0 && is_part_of_name (*op_end);)
    {
      to[size++] = *op_end++;
      if (size + 1 >= limit)
	break;
    }

  to[size] = 0;
  return op_end;
}

int
md_estimate_size_before_relax (fragS *fragp ATTRIBUTE_UNUSED,
			       asection *seg ATTRIBUTE_UNUSED)
{
  abort ();
  return 0;
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream,
      _("AVR Assembler options:\n"
	"  -mmcu=[avr-name] select microcontroller variant\n"
	"                   [avr-name] can be:\n"
	"                   avr1  - classic AVR core without data RAM\n"
	"                   avr2  - classic AVR core with up to 8K program memory\n"
	"                   avr25 - classic AVR core with up to 8K program memory\n"
	"                           plus the MOVW instruction\n"
	"                   avr3  - classic AVR core with up to 64K program memory\n"
	"                   avr31 - classic AVR core with up to 128K program memory\n"
	"                   avr35 - classic AVR core with up to 64K program memory\n"
	"                           plus the MOVW instruction\n"
	"                   avr4  - enhanced AVR core with up to 8K program memory\n"
	"                   avr5  - enhanced AVR core with up to 64K program memory\n"
	"                   avr51 - enhanced AVR core with up to 128K program memory\n"
	"                   avr6  - enhanced AVR core with up to 256K program memory\n"
	"                   avrxmega2 - XMEGA, > 8K, < 64K FLASH, < 64K RAM\n"
	"                   avrxmega3 - XMEGA, > 8K, <= 64K FLASH, > 64K RAM\n"
	"                   avrxmega4 - XMEGA, > 64K, <= 128K FLASH, <= 64K RAM\n"
	"                   avrxmega5 - XMEGA, > 64K, <= 128K FLASH, > 64K RAM\n"
	"                   avrxmega6 - XMEGA, > 128K, <= 256K FLASH, <= 64K RAM\n"
	"                   avrxmega7 - XMEGA, > 128K, <= 256K FLASH, > 64K RAM\n"
	"                   avrtiny   - AVR Tiny core with 16 gp registers\n"));
  fprintf (stream,
      _("  -mall-opcodes    accept all AVR opcodes, even if not supported by MCU\n"
	"  -mno-skip-bug    disable warnings for skipping two-word instructions\n"
	"                   (default for avr4, avr5)\n"
	"  -mno-wrap        reject rjmp/rcall instructions with 8K wrap-around\n"
	"                   (default for avr3, avr5)\n"
	"  -mrmw            accept Read-Modify-Write instructions\n"
	"  -mlink-relax     generate relocations for linker relaxation (default)\n"
	"  -mno-link-relax  don't generate relocations for linker relaxation.\n"
        ));
  show_mcu_list (stream);
}

static void
avr_set_arch (int dummy ATTRIBUTE_UNUSED)
{
  char str[20];

  input_line_pointer = extract_word (input_line_pointer, str, 20);
  md_parse_option (OPTION_MMCU, str);
  bfd_set_arch_mach (stdoutput, TARGET_ARCH, avr_mcu->mach);
}

int
md_parse_option (int c, const char *arg)
{
  switch (c)
    {
    case OPTION_MMCU:
      {
	int i;

	for (i = 0; mcu_types[i].name; ++i)
	  if (strcasecmp (mcu_types[i].name, arg) == 0)
	    break;

	if (!mcu_types[i].name)
	  {
	    show_mcu_list (stderr);
	    as_fatal (_("unknown MCU: %s\n"), arg);
	  }

	/* It is OK to redefine mcu type within the same avr[1-5] bfd machine
	   type - this for allows passing -mmcu=... via gcc ASM_SPEC as well
	   as .arch ... in the asm output at the same time.  */
	if (avr_mcu == &default_mcu || avr_mcu->mach == mcu_types[i].mach)
	  {
	    specified_mcu.name = mcu_types[i].name;
	    specified_mcu.isa  |= mcu_types[i].isa;
	    specified_mcu.mach = mcu_types[i].mach;
	    avr_mcu = &specified_mcu;
	  }
	else
	  as_fatal (_("redefinition of mcu type `%s' to `%s'"),
		    avr_mcu->name, mcu_types[i].name);
	return 1;
      }
    case OPTION_ALL_OPCODES:
      avr_opt.all_opcodes = 1;
      return 1;
    case OPTION_NO_SKIP_BUG:
      avr_opt.no_skip_bug = 1;
      return 1;
    case OPTION_NO_WRAP:
      avr_opt.no_wrap = 1;
      return 1;
    case OPTION_ISA_RMW:
      specified_mcu.isa |= AVR_ISA_RMW;
      return 1;
    case OPTION_LINK_RELAX:
      avr_opt.no_link_relax = 0;
      return 1;
    case OPTION_NO_LINK_RELAX:
      avr_opt.no_link_relax = 1;
      return 1;
    }

  return 0;
}

symbolS *
md_undefined_symbol (char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

const char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, FALSE);
}

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 asection *sec ATTRIBUTE_UNUSED,
		 fragS *fragP ATTRIBUTE_UNUSED)
{
  abort ();
}

void
md_begin (void)
{
  unsigned int i;
  struct avr_opcodes_s *opcode;

  avr_hash = hash_new ();

  /* Insert unique names into hash table.  This hash table then provides a
     quick index to the first opcode with a particular name in the opcode
     table.  */
  for (opcode = avr_opcodes; opcode->name; opcode++)
    hash_insert (avr_hash, opcode->name, (char *) opcode);

  avr_mod_hash = hash_new ();

  for (i = 0; i < ARRAY_SIZE (exp_mod); ++i)
    {
      mod_index m;

      m.index = i + 10;
      hash_insert (avr_mod_hash, EXP_MOD_NAME (i), m.ptr);
    }

  bfd_set_arch_mach (stdoutput, TARGET_ARCH, avr_mcu->mach);
  linkrelax = !avr_opt.no_link_relax;
}

/* Resolve STR as a constant expression and return the result.
   If result greater than MAX then error.  */

static unsigned int
avr_get_constant (char *str, int max)
{
  expressionS ex;

  str = skip_space (str);
  input_line_pointer = str;
  expression (& ex);

  if (ex.X_op != O_constant)
    as_bad (_("constant value required"));

  if (ex.X_add_number > max || ex.X_add_number < 0)
    as_bad (_("number must be positive and less than %d"), max + 1);

  return ex.X_add_number;
}

/* Parse for ldd/std offset.  */

static void
avr_offset_expression (expressionS *exp)
{
  char *str = input_line_pointer;
  char *tmp;
  char op[8];

  tmp = str;
  str = extract_word (str, op, sizeof (op));

  input_line_pointer = tmp;
  expression (exp);

  /* Warn about expressions that fail to use lo8 ().  */
  if (exp->X_op == O_constant)
    {
      int x = exp->X_add_number;

      if (x < -255 || x > 255)
	as_warn (_("constant out of 8-bit range: %d"), x);
    }
}

/* Parse ordinary expression.  */

static char *
parse_exp (char *s, expressionS *op)
{
  input_line_pointer = s;
  expression (op);
  if (op->X_op == O_absent)
    as_bad (_("missing operand"));
  return input_line_pointer;
}

/* Parse special expressions (needed for LDI command):
   xx8 (address)
   xx8 (-address)
   pm_xx8 (address)
   pm_xx8 (-address)
   where xx is: hh, hi, lo.  */

static bfd_reloc_code_real_type
avr_ldi_expression (expressionS *exp)
{
  char *str = input_line_pointer;
  char *tmp;
  char op[8];
  int mod;
  int linker_stubs_should_be_generated = 0;

  tmp = str;

  str = extract_word (str, op, sizeof (op));

  if (op[0])
    {
      mod_index m;

      m.ptr = hash_find (avr_mod_hash, op);
      mod = m.index;

      if (mod)
	{
	  int closes = 0;

	  mod -= 10;
	  str = skip_space (str);

	  if (*str == '(')
	    {
	      bfd_reloc_code_real_type  reloc_to_return;
	      int neg_p = 0;

	      ++str;

	      if (strncmp ("pm(", str, 3) == 0
                  || strncmp ("gs(",str,3) == 0
                  || strncmp ("-(gs(",str,5) == 0
		  || strncmp ("-(pm(", str, 5) == 0)
		{
		  if (HAVE_PM_P (mod))
		    {
		      ++mod;
		      ++closes;
		    }
		  else
		    as_bad (_("illegal expression"));

                  if (str[0] == 'g' || str[2] == 'g')
                    linker_stubs_should_be_generated = 1;

		  if (*str == '-')
		    {
		      neg_p = 1;
		      ++closes;
		      str += 5;
		    }
		  else
		    str += 3;
		}

	      if (*str == '-' && *(str + 1) == '(')
		{
		  neg_p ^= 1;
		  ++closes;
		  str += 2;
		}

	      input_line_pointer = str;
	      expression (exp);

	      do
		{
		  if (*input_line_pointer != ')')
		    {
		      as_bad (_("`)' required"));
		      break;
		    }
		  input_line_pointer++;
		}
	      while (closes--);

	      reloc_to_return =
		neg_p ? EXP_MOD_NEG_RELOC (mod) : EXP_MOD_RELOC (mod);
	      if (linker_stubs_should_be_generated)
		{
		  switch (reloc_to_return)
		    {
		    case BFD_RELOC_AVR_LO8_LDI_PM:
		      reloc_to_return = BFD_RELOC_AVR_LO8_LDI_GS;
		      break;
		    case BFD_RELOC_AVR_HI8_LDI_PM:
		      reloc_to_return = BFD_RELOC_AVR_HI8_LDI_GS;
		      break;

		    default:
		      /* PR 5523: Do not generate a warning here,
			 legitimate code can trigger this case.  */
		      break;
		    }
		}
	      return reloc_to_return;
	    }
	}
    }

  input_line_pointer = tmp;
  expression (exp);

  /* Warn about expressions that fail to use lo8 ().  */
  if (exp->X_op == O_constant)
    {
      int x = exp->X_add_number;

      if (x < -255 || x > 255)
	as_warn (_("constant out of 8-bit range: %d"), x);
    }

  return BFD_RELOC_AVR_LDI;
}

/* Parse one instruction operand.
   Return operand bitmask.  Also fixups can be generated.  */

static unsigned int
avr_operand (struct avr_opcodes_s *opcode,
	     int where,
	     const char *op,
	     char **line)
{
  expressionS op_expr;
  unsigned int op_mask = 0;
  char *str = skip_space (*line);

  switch (*op)
    {
      /* Any register operand.  */
    case 'w':
    case 'd':
    case 'r':
    case 'a':
    case 'v':
      {
        char * old_str = str;
        char *lower;
        char r_name[20];

        str = extract_word (str, r_name, sizeof (r_name));
        for (lower = r_name; *lower; ++lower)
	  {
	    if (*lower >= 'A' && *lower <= 'Z')
	      *lower += 'a' - 'A';
          }

        if (r_name[0] == 'r' && ISDIGIT (r_name[1]) && r_name[2] == 0)
          /* Single-digit register number, ie r0-r9.  */
          op_mask = r_name[1] - '0';
        else if (r_name[0] == 'r' && ISDIGIT (r_name[1])
		 && ISDIGIT (r_name[2]) && r_name[3] == 0)
          /* Double-digit register number, ie r10 - r32.  */
          op_mask = (r_name[1] - '0') * 10 + r_name[2] - '0';
        else if (r_name[0] >= 'x' && r_name[0] <= 'z'
		 && (r_name[1] == 'l' || r_name[1] == 'h') && r_name[2] == 0)
          /* Registers r26-r31 referred to by name, ie xl, xh, yl, yh, zl, zh.  */
          op_mask = (r_name[0] - 'x') * 2 + (r_name[1] == 'h') + 26;
        else if ((*op == 'v' || *op == 'w')
		 && r_name[0] >= 'x' && r_name[0] <= 'z' && r_name[1] == 0)
          /* For the movw and addiw instructions, refer to registers x, y and z by name.  */
          op_mask = (r_name[0] - 'x') * 2 + 26;
        else
          {
            /* Numeric or symbolic constant register number.  */
            op_mask = avr_get_constant (old_str, 31);
            str = input_line_pointer;
          }
      }

      if (avr_mcu->mach == bfd_mach_avrtiny)
        {
          if (op_mask < 16 || op_mask > 31)
            {
              as_bad (_("register name or number from 16 to 31 required"));
              break;
            }
        }
      else if (op_mask > 31)
        {
          as_bad (_("register name or number from 0 to 31 required"));
          break;
        }

	  switch (*op)
	    {
	    case 'a':
	      if (op_mask < 16 || op_mask > 23)
		as_bad (_("register r16-r23 required"));
	      op_mask -= 16;
	      break;

	    case 'd':
	      if (op_mask < 16)
		as_bad (_("register number above 15 required"));
	      op_mask -= 16;
	      break;

	    case 'v':
	      if (op_mask & 1)
		as_bad (_("even register number required"));
	      op_mask >>= 1;
	      break;

	    case 'w':
	      if ((op_mask & 1) || op_mask < 24)
		as_bad (_("register r24, r26, r28 or r30 required"));
	      op_mask = (op_mask - 24) >> 1;
	      break;
	    }
	  break;

    case 'e':
      {
	char c;

	if (*str == '-')
	  {
	    str = skip_space (str + 1);
	    op_mask = 0x1002;
	  }
	c = TOLOWER (*str);
	if (c == 'x')
	  op_mask |= 0x100c;
	else if (c == 'y')
	  op_mask |= 0x8;
	else if (c != 'z')
	  as_bad (_("pointer register (X, Y or Z) required"));

	str = skip_space (str + 1);
	if (*str == '+')
	  {
	    ++str;
	    if (op_mask & 2)
	      as_bad (_("cannot both predecrement and postincrement"));
	    op_mask |= 0x1001;
	  }

	/* avr1 can do "ld r,Z" and "st Z,r" but no other pointer
	   registers, no predecrement, no postincrement.  */
	if (!avr_opt.all_opcodes && (op_mask & 0x100F)
	    && !(avr_mcu->isa & AVR_ISA_SRAM))
	  as_bad (_("addressing mode not supported"));
      }
      break;

    case 'z':
      if (*str == '-')
	as_bad (_("can't predecrement"));

      if (! (*str == 'z' || *str == 'Z'))
	as_bad (_("pointer register Z required"));

      str = skip_space (str + 1);

      if (*str == '+')
	{
	  ++str;
          const char *s;
          for (s = opcode->opcode; *s; ++s)
            {
              if (*s == '+')
                op_mask |= (1 << (15 - (s - opcode->opcode)));
            }
	}

      /* attiny26 can do "lpm" and "lpm r,Z" but not "lpm r,Z+".  */
      if (!avr_opt.all_opcodes
	  && (op_mask & 0x0001)
	  && !(avr_mcu->isa & AVR_ISA_MOVW))
	as_bad (_("postincrement not supported"));
      break;

    case 'b':
      {
	char c = TOLOWER (*str++);

	if (c == 'y')
	  op_mask |= 0x8;
	else if (c != 'z')
	  as_bad (_("pointer register (Y or Z) required"));
	str = skip_space (str);
	if (*str++ == '+')
	  {
	    input_line_pointer = str;
	    avr_offset_expression (& op_expr);
	    str = input_line_pointer;
	    fix_new_exp (frag_now, where, 3,
			 &op_expr, FALSE, BFD_RELOC_AVR_6);
	  }
      }
      break;

    case 'h':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		   &op_expr, FALSE, BFD_RELOC_AVR_CALL);
      break;

    case 'L':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		   &op_expr, TRUE, BFD_RELOC_AVR_13_PCREL);
      break;

    case 'l':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		   &op_expr, TRUE, BFD_RELOC_AVR_7_PCREL);
      break;

    case 'i':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where + 2, opcode->insn_size * 2,
		   &op_expr, FALSE, BFD_RELOC_16);
      break;

    case 'j':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		   &op_expr, FALSE, BFD_RELOC_AVR_LDS_STS_16);
      break;

    case 'M':
      {
	bfd_reloc_code_real_type r_type;

	input_line_pointer = str;
	r_type = avr_ldi_expression (&op_expr);
	str = input_line_pointer;
	fix_new_exp (frag_now, where, 3,
		     &op_expr, FALSE, r_type);
      }
      break;

    case 'n':
      {
	unsigned int x;

	x = ~avr_get_constant (str, 255);
	str = input_line_pointer;
	op_mask |= (x & 0xf) | ((x << 4) & 0xf00);
      }
      break;

    case 'K':
      input_line_pointer = str;
      avr_offset_expression (& op_expr);
      str = input_line_pointer;
      fix_new_exp (frag_now, where, 3,
		   & op_expr, FALSE, BFD_RELOC_AVR_6_ADIW);
      break;

    case 'S':
    case 's':
      {
	unsigned int x;

	x = avr_get_constant (str, 7);
	str = input_line_pointer;
	if (*op == 'S')
	  x <<= 4;
	op_mask |= x;
      }
      break;

    case 'P':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		     &op_expr, FALSE, BFD_RELOC_AVR_PORT6);
      break;

    case 'p':
      str = parse_exp (str, &op_expr);
      fix_new_exp (frag_now, where, opcode->insn_size * 2,
		     &op_expr, FALSE, BFD_RELOC_AVR_PORT5);
      break;

    case 'E':
      {
	unsigned int x;

	x = avr_get_constant (str, 15);
	str = input_line_pointer;
	op_mask |= (x << 4);
      }
      break;

    case '?':
      break;

    default:
      as_bad (_("unknown constraint `%c'"), *op);
    }

  *line = str;
  return op_mask;
}

/* Parse instruction operands.
   Return binary opcode.  */

static unsigned int
avr_operands (struct avr_opcodes_s *opcode, char **line)
{
  const char *op = opcode->constraints;
  unsigned int bin = opcode->bin_opcode;
  char *frag = frag_more (opcode->insn_size * 2);
  char *str = *line;
  int where = frag - frag_now->fr_literal;
  static unsigned int prev = 0;  /* Previous opcode.  */

  /* Opcode have operands.  */
  if (*op)
    {
      unsigned int reg1 = 0;
      unsigned int reg2 = 0;
      int reg1_present = 0;
      int reg2_present = 0;

      /* Parse first operand.  */
      if (REGISTER_P (*op))
	reg1_present = 1;
      reg1 = avr_operand (opcode, where, op, &str);
      ++op;

      /* Parse second operand.  */
      if (*op)
	{
	  if (*op == ',')
	    ++op;

	  if (*op == '=')
	    {
	      reg2 = reg1;
	      reg2_present = 1;
	    }
	  else
	    {
	      if (REGISTER_P (*op))
		reg2_present = 1;

	      str = skip_space (str);
	      if (*str++ != ',')
		as_bad (_("`,' required"));
	      str = skip_space (str);

	      reg2 = avr_operand (opcode, where, op, &str);
	    }

	  if (reg1_present && reg2_present)
	    reg2 = (reg2 & 0xf) | ((reg2 << 5) & 0x200);
	  else if (reg2_present)
	    reg2 <<= 4;
	}
      if (reg1_present)
	reg1 <<= 4;
      bin |= reg1 | reg2;
    }

  /* Detect undefined combinations (like ld r31,Z+).  */
  if (!avr_opt.all_opcodes && AVR_UNDEF_P (bin))
    as_warn (_("undefined combination of operands"));

  if (opcode->insn_size == 2)
    {
      /* Warn if the previous opcode was cpse/sbic/sbis/sbrc/sbrs
         (AVR core bug, fixed in the newer devices).  */
      if (!(avr_opt.no_skip_bug ||
            (avr_mcu->isa & (AVR_ISA_MUL | AVR_ISA_MOVW)))
	  && AVR_SKIP_P (prev))
	as_warn (_("skipping two-word instruction"));

      bfd_putl32 ((bfd_vma) bin, frag);
    }
  else
    bfd_putl16 ((bfd_vma) bin, frag);

  prev = bin;
  *line = str;
  return bin;
}

/* GAS will call this function for each section at the end of the assembly,
   to permit the CPU backend to adjust the alignment of a section.  */

valueT
md_section_align (asection *seg, valueT addr)
{
  int align = bfd_get_section_alignment (stdoutput, seg);
  return ((addr + (1 << align) - 1) & (-1UL << align));
}

/* If you define this macro, it should return the offset between the
   address of a PC relative fixup and the position from which the PC
   relative adjustment should be made.  On many processors, the base
   of a PC relative instruction is the next instruction, so this
   macro would return the length of an instruction.  */

long
md_pcrel_from_section (fixS *fixp, segT sec)
{
  if (fixp->fx_addsy != (symbolS *) NULL
      && (!S_IS_DEFINED (fixp->fx_addsy)
	  || (S_GET_SEGMENT (fixp->fx_addsy) != sec)))
    return 0;

  return fixp->fx_frag->fr_address + fixp->fx_where;
}

static bfd_boolean
relaxable_section (asection *sec)
{
  return ((sec->flags & SEC_DEBUGGING) == 0
          && (sec->flags & SEC_CODE) != 0
          && (sec->flags & SEC_ALLOC) != 0);
}

/* Does whatever the xtensa port does.  */
int
avr_validate_fix_sub (fixS *fix)
{
  segT add_symbol_segment, sub_symbol_segment;

  /* The difference of two symbols should be resolved by the assembler when
     linkrelax is not set.  If the linker may relax the section containing
     the symbols, then an Xtensa DIFF relocation must be generated so that
     the linker knows to adjust the difference value.  */
  if (!linkrelax || fix->fx_addsy == NULL)
    return 0;

  /* Make sure both symbols are in the same segment, and that segment is
     "normal" and relaxable.  If the segment is not "normal", then the
     fix is not valid.  If the segment is not "relaxable", then the fix
     should have been handled earlier.  */
  add_symbol_segment = S_GET_SEGMENT (fix->fx_addsy);
  if (! SEG_NORMAL (add_symbol_segment) ||
      ! relaxable_section (add_symbol_segment))
    return 0;

  sub_symbol_segment = S_GET_SEGMENT (fix->fx_subsy);
  return (sub_symbol_segment == add_symbol_segment);
}

/* TC_FORCE_RELOCATION hook */

/* If linkrelax is turned on, and the symbol to relocate
   against is in a relaxable segment, don't compute the value -
   generate a relocation instead.  */
int
avr_force_relocation (fixS *fix)
{
  if (linkrelax && fix->fx_addsy
      && relaxable_section (S_GET_SEGMENT (fix->fx_addsy)))
    return 1;

  return generic_force_reloc (fix);
}

/* GAS will call this for each fixup.  It should store the correct
   value in the object file.  */

void
md_apply_fix (fixS *fixP, valueT * valP, segT seg)
{
  unsigned char *where;
  unsigned long insn;
  long value = *valP;

  if (fixP->fx_addsy == (symbolS *) NULL)
    fixP->fx_done = 1;

  else if (fixP->fx_pcrel)
    {
      segT s = S_GET_SEGMENT (fixP->fx_addsy);

      if (s == seg || s == absolute_section)
	{
	  value += S_GET_VALUE (fixP->fx_addsy);
	  fixP->fx_done = 1;
	}
    }
  else if (linkrelax && fixP->fx_subsy)
    {
      /* For a subtraction relocation expression, generate one
         of the DIFF relocs, with the value being the difference.
         Note that a sym1 - sym2 expression is adjusted into a
         section_start_sym + sym4_offset_from_section_start - sym1
         expression. fixP->fx_addsy holds the section start symbol,
         fixP->fx_offset holds sym2's offset, and fixP->fx_subsy
         holds sym1. Calculate the current difference and write value,
         but leave fx_offset as is - during relaxation,
         fx_offset - value gives sym1's value.  */

       switch (fixP->fx_r_type)
         {
           case BFD_RELOC_8:
             fixP->fx_r_type = BFD_RELOC_AVR_DIFF8;
             break;
           case BFD_RELOC_16:
             fixP->fx_r_type = BFD_RELOC_AVR_DIFF16;
             break;
           case BFD_RELOC_32:
             fixP->fx_r_type = BFD_RELOC_AVR_DIFF32;
             break;
           default:
             as_bad_where (fixP->fx_file, fixP->fx_line, _("expression too complex"));
             break;
         }

      value = S_GET_VALUE (fixP->fx_addsy) +
          fixP->fx_offset - S_GET_VALUE (fixP->fx_subsy);
      *valP = value;

      fixP->fx_subsy = NULL;
  }
  /* We don't actually support subtracting a symbol.  */
  if (fixP->fx_subsy != (symbolS *) NULL)
    as_bad_where (fixP->fx_file, fixP->fx_line, _("expression too complex"));

  /* For the DIFF relocs, write the value into the object file while still
     keeping fx_done FALSE, as both the difference (recorded in the object file)
     and the sym offset (part of fixP) are needed at link relax time.  */
  where = (unsigned char *) fixP->fx_frag->fr_literal + fixP->fx_where;
  switch (fixP->fx_r_type)
    {
    default:
      fixP->fx_no_overflow = 1;
      break;
    case BFD_RELOC_AVR_7_PCREL:
    case BFD_RELOC_AVR_13_PCREL:
    case BFD_RELOC_32:
    case BFD_RELOC_16:
      break;
    case BFD_RELOC_AVR_DIFF8:
      *where = value;
	  break;
    case BFD_RELOC_AVR_DIFF16:
      bfd_putl16 ((bfd_vma) value, where);
      break;
    case BFD_RELOC_AVR_DIFF32:
      bfd_putl32 ((bfd_vma) value, where);
      break;
    case BFD_RELOC_AVR_CALL:
      break;
    }

  if (fixP->fx_done)
    {
      /* Fetch the instruction, insert the fully resolved operand
	 value, and stuff the instruction back again.  */
      where = (unsigned char *) fixP->fx_frag->fr_literal + fixP->fx_where;
      insn = bfd_getl16 (where);

      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_AVR_7_PCREL:
	  if (value & 1)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("odd address operand: %ld"), value);

	  /* Instruction addresses are always right-shifted by 1.  */
	  value >>= 1;
	  --value;			/* Correct PC.  */

	  if (value < -64 || value > 63)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  value = (value << 3) & 0x3f8;
	  bfd_putl16 ((bfd_vma) (value | insn), where);
	  break;

	case BFD_RELOC_AVR_13_PCREL:
	  if (value & 1)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("odd address operand: %ld"), value);

	  /* Instruction addresses are always right-shifted by 1.  */
	  value >>= 1;
	  --value;			/* Correct PC.  */

	  if (value < -2048 || value > 2047)
	    {
	      /* No wrap for devices with >8K of program memory.  */
	      if ((avr_mcu->isa & AVR_ISA_MEGA) || avr_opt.no_wrap)
		as_bad_where (fixP->fx_file, fixP->fx_line,
			      _("operand out of range: %ld"), value);
	    }

	  value &= 0xfff;
	  bfd_putl16 ((bfd_vma) (value | insn), where);
	  break;

	case BFD_RELOC_32:
	  bfd_putl32 ((bfd_vma) value, where);
	  break;

	case BFD_RELOC_16:
	  bfd_putl16 ((bfd_vma) value, where);
	  break;

	case BFD_RELOC_8:
          if (value > 255 || value < -128)
	    as_warn_where (fixP->fx_file, fixP->fx_line,
                           _("operand out of range: %ld"), value);
          *where = value;
	  break;

	case BFD_RELOC_AVR_16_PM:
	  bfd_putl16 ((bfd_vma) (value >> 1), where);
	  break;

	case BFD_RELOC_AVR_LDI:
	  if (value > 255)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value), where);
	  break;

	case BFD_RELOC_AVR_LDS_STS_16:
	  if ((value < 0x40) || (value > 0xBF))
	    as_warn_where (fixP->fx_file, fixP->fx_line,
			   _("operand out of range: 0x%lx"),
			   (unsigned long)value);
	  insn |= ((value & 0xF) | ((value & 0x30) << 5) | ((value & 0x40) << 2));
	  bfd_putl16 ((bfd_vma) insn, where);
	  break;

	case BFD_RELOC_AVR_6:
	  if ((value > 63) || (value < 0))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  bfd_putl16 ((bfd_vma) insn | ((value & 7) | ((value & (3 << 3)) << 7)
					| ((value & (1 << 5)) << 8)), where);
	  break;

	case BFD_RELOC_AVR_6_ADIW:
	  if ((value > 63) || (value < 0))
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  bfd_putl16 ((bfd_vma) insn | (value & 0xf) | ((value & 0x30) << 2), where);
	  break;

	case BFD_RELOC_AVR_LO8_LDI:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value), where);
	  break;

	case BFD_RELOC_AVR_HI8_LDI:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 8), where);
	  break;

	case BFD_RELOC_AVR_MS8_LDI:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 24), where);
	  break;

	case BFD_RELOC_AVR_HH8_LDI:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 16), where);
	  break;

	case BFD_RELOC_AVR_LO8_LDI_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value), where);
	  break;

	case BFD_RELOC_AVR_HI8_LDI_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 8), where);
	  break;

	case BFD_RELOC_AVR_MS8_LDI_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 24), where);
	  break;

	case BFD_RELOC_AVR_HH8_LDI_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 16), where);
	  break;

	case BFD_RELOC_AVR_LO8_LDI_PM:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 1), where);
	  break;

	case BFD_RELOC_AVR_HI8_LDI_PM:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 9), where);
	  break;

	case BFD_RELOC_AVR_HH8_LDI_PM:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (value >> 17), where);
	  break;

	case BFD_RELOC_AVR_LO8_LDI_PM_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 1), where);
	  break;

	case BFD_RELOC_AVR_HI8_LDI_PM_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 9), where);
	  break;

	case BFD_RELOC_AVR_HH8_LDI_PM_NEG:
	  bfd_putl16 ((bfd_vma) insn | LDI_IMMEDIATE (-value >> 17), where);
	  break;

	case BFD_RELOC_AVR_CALL:
	  {
	    unsigned long x;

	    x = bfd_getl16 (where);
	    if (value & 1)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("odd address operand: %ld"), value);
	    value >>= 1;
	    x |= ((value & 0x10000) | ((value << 3) & 0x1f00000)) >> 16;
	    bfd_putl16 ((bfd_vma) x, where);
	    bfd_putl16 ((bfd_vma) (value & 0xffff), where + 2);
	  }
	  break;

        case BFD_RELOC_AVR_8_LO:
          *where = 0xff & value;
          break;

        case BFD_RELOC_AVR_8_HI:
          *where = 0xff & (value >> 8);
          break;

        case BFD_RELOC_AVR_8_HLO:
          *where = 0xff & (value >> 16);
          break;

        default:
	  as_fatal (_("line %d: unknown relocation type: 0x%x"),
		    fixP->fx_line, fixP->fx_r_type);
	  break;

	case BFD_RELOC_AVR_PORT6:
	  if (value > 63)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  bfd_putl16 ((bfd_vma) insn | ((value & 0x30) << 5) | (value & 0x0f), where);
	  break;

	case BFD_RELOC_AVR_PORT5:
	  if (value > 31)
	    as_bad_where (fixP->fx_file, fixP->fx_line,
			  _("operand out of range: %ld"), value);
	  bfd_putl16 ((bfd_vma) insn | ((value & 0x1f) << 3), where);
	  break;
	}
    }
  else
    {
      switch ((int) fixP->fx_r_type)
	{
	case -BFD_RELOC_AVR_HI8_LDI_NEG:
	case -BFD_RELOC_AVR_HI8_LDI:
	case -BFD_RELOC_AVR_LO8_LDI_NEG:
	case -BFD_RELOC_AVR_LO8_LDI:
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("only constant expression allowed"));
	  fixP->fx_done = 1;
	  break;
	default:
	  break;
	}
    }
}

/* GAS will call this to generate a reloc, passing the resulting reloc
   to `bfd_install_relocation'.  This currently works poorly, as
   `bfd_install_relocation' often does the wrong thing, and instances of
   `tc_gen_reloc' have been written to work around the problems, which
   in turns makes it difficult to fix `bfd_install_relocation'.  */

/* If while processing a fixup, a reloc really needs to be created
   then it is done here.  */

arelent *
tc_gen_reloc (asection *seg ATTRIBUTE_UNUSED,
	      fixS *fixp)
{
  arelent *reloc;
  bfd_reloc_code_real_type code = fixp->fx_r_type;

  if (fixp->fx_subsy != NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line, _("expression too complex"));
      return NULL;
    }

  reloc = XNEW (arelent);

  reloc->sym_ptr_ptr = XNEW (asymbol *);
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);

  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;

  if ((fixp->fx_r_type == BFD_RELOC_32) && (fixp->fx_pcrel))
    {
      if (seg->use_rela_p)
        fixp->fx_offset -= md_pcrel_from_section (fixp, seg);
      else
        fixp->fx_offset = reloc->address;

      code = BFD_RELOC_32_PCREL;
    }

  reloc->addend = fixp->fx_offset;

  reloc->howto = bfd_reloc_type_lookup (stdoutput, code);

  if (reloc->howto == (reloc_howto_type *) NULL)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
		    _("reloc %d not supported by object file format"),
		    (int) fixp->fx_r_type);
      return NULL;
    }

  if (fixp->fx_r_type == BFD_RELOC_VTABLE_INHERIT
      || fixp->fx_r_type == BFD_RELOC_VTABLE_ENTRY)
    reloc->address = fixp->fx_offset;


  return reloc;
}

void
md_assemble (char *str)
{
  struct avr_opcodes_s *opcode;
  char op[11];

  str = skip_space (extract_word (str, op, sizeof (op)));

  if (!op[0])
    as_bad (_("can't find opcode "));

  opcode = (struct avr_opcodes_s *) hash_find (avr_hash, op);

  if (opcode && !avr_opt.all_opcodes)
    {
      /* Check if the instruction's ISA bit is ON in the ISA bits of the part
         specified by the user.  If not look for other instructions
	 specifications with same mnemonic who's ISA bits matches.

         This requires include/opcode/avr.h to have the instructions with
         same mnenomic to be specified in sequence.  */

      while ((opcode->isa & avr_mcu->isa) != opcode->isa)
        {
          opcode++;

          if (opcode->name && strcmp(op, opcode->name))
            {
              as_bad (_("illegal opcode %s for mcu %s"),
                      opcode->name, avr_mcu->name);
              return;
            }
        }
    }

  if (opcode == NULL)
    {
      as_bad (_("unknown opcode `%s'"), op);
      return;
    }

  /* Special case for opcodes with optional operands (lpm, elpm) -
     version with operands exists in avr_opcodes[] in the next entry.  */

  if (*str && *opcode->constraints == '?')
    ++opcode;

  dwarf2_emit_insn (0);

  /* We used to set input_line_pointer to the result of get_operands,
     but that is wrong.  Our caller assumes we don't change it.  */
  {
    char *t = input_line_pointer;

    avr_operands (opcode, &str);
    if (*skip_space (str))
      as_bad (_("garbage at end of line"));
    input_line_pointer = t;
  }
}

const exp_mod_data_t exp_mod_data[] =
{
  /* Default, must be first.  */
  { "", 0, BFD_RELOC_16, "" },
  /* Divides by 2 to get word address.  Generate Stub.  */
  { "gs", 2, BFD_RELOC_AVR_16_PM, "`gs' " },
  { "pm", 2, BFD_RELOC_AVR_16_PM, "`pm' " },
  /* The following are used together with avr-gcc's __memx address space
     in order to initialize a 24-bit pointer variable with a 24-bit address.
     For address in flash, hlo8 will contain the flash segment if the
     symbol is located in flash. If the symbol is located in RAM; hlo8
     will contain 0x80 which matches avr-gcc's notion of how 24-bit RAM/flash
     addresses linearize address space.  */
  { "lo8",  1, BFD_RELOC_AVR_8_LO,  "`lo8' "  },
  { "hi8",  1, BFD_RELOC_AVR_8_HI,  "`hi8' "  },
  { "hlo8", 1, BFD_RELOC_AVR_8_HLO, "`hlo8' " },
  { "hh8",  1, BFD_RELOC_AVR_8_HLO, "`hh8' "  },
};

/* Parse special CONS expression: pm (expression) or alternatively
   gs (expression).  These are used for addressing program memory.  Moreover,
   define lo8 (expression), hi8 (expression) and hlo8 (expression).  */

const exp_mod_data_t *
avr_parse_cons_expression (expressionS *exp, int nbytes)
{
  char *tmp;
  unsigned int i;

  tmp = input_line_pointer = skip_space (input_line_pointer);

  /* The first entry of exp_mod_data[] contains an entry if no
     expression modifier is present.  Skip it.  */

  for (i = 0; i < ARRAY_SIZE (exp_mod_data); i++)
    {
      const exp_mod_data_t *pexp = &exp_mod_data[i];
      int len = strlen (pexp->name);

      if (nbytes == pexp->nbytes
          && strncasecmp (input_line_pointer, pexp->name, len) == 0)
	{
	  input_line_pointer = skip_space (input_line_pointer + len);

	  if (*input_line_pointer == '(')
	    {
	      input_line_pointer = skip_space (input_line_pointer + 1);
	      expression (exp);

	      if (*input_line_pointer == ')')
		{
		  ++input_line_pointer;
		  return pexp;
		}
	      else
		{
		  as_bad (_("`)' required"));
		  return &exp_mod_data[0];
		}
	    }

	  input_line_pointer = tmp;

          break;
	}
    }

  expression (exp);
  return &exp_mod_data[0];
}

void
avr_cons_fix_new (fragS *frag,
		  int where,
		  int nbytes,
		  expressionS *exp,
		  const exp_mod_data_t *pexp_mod_data)
{
  int bad = 0;

  switch (pexp_mod_data->reloc)
    {
    default:
      if (nbytes == 1)
	fix_new_exp (frag, where, nbytes, exp, FALSE, BFD_RELOC_8);
      else if (nbytes == 2)
	fix_new_exp (frag, where, nbytes, exp, FALSE, BFD_RELOC_16);
      else if (nbytes == 4)
	fix_new_exp (frag, where, nbytes, exp, FALSE, BFD_RELOC_32);
      else
	bad = 1;
      break;

    case BFD_RELOC_AVR_16_PM:
    case BFD_RELOC_AVR_8_LO:
    case BFD_RELOC_AVR_8_HI:
    case BFD_RELOC_AVR_8_HLO:
      if (nbytes == pexp_mod_data->nbytes)
        fix_new_exp (frag, where, nbytes, exp, FALSE, pexp_mod_data->reloc);
      else
        bad = 1;
      break;
    }

  if (bad)
    as_bad (_("illegal %srelocation size: %d"), pexp_mod_data->error, nbytes);
}

static bfd_boolean
mcu_has_3_byte_pc (void)
{
  int mach = avr_mcu->mach;

  return mach == bfd_mach_avr6
    || mach == bfd_mach_avrxmega6
    || mach == bfd_mach_avrxmega7;
}

void
tc_cfi_frame_initial_instructions (void)
{
  /* AVR6 pushes 3 bytes for calls.  */
  int return_size = (mcu_has_3_byte_pc () ? 3 : 2);

  /* The CFA is the caller's stack location before the call insn.  */
  /* Note that the stack pointer is dwarf register number 32.  */
  cfi_add_CFA_def_cfa (32, return_size);

  /* Note that AVR consistently uses post-decrement, which means that things
     do not line up the same way as for targers that use pre-decrement.  */
  cfi_add_CFA_offset (DWARF2_DEFAULT_RETURN_COLUMN, 1-return_size);
}

bfd_boolean
avr_allow_local_subtract (expressionS * left,
			     expressionS * right,
			     segT section)
{
  /* If we are not in relaxation mode, subtraction is OK.  */
  if (!linkrelax)
    return TRUE;

  /* If the symbols are not in a code section then they are OK.  */
  if ((section->flags & SEC_CODE) == 0)
    return TRUE;

  if (left->X_add_symbol == right->X_add_symbol)
    return TRUE;

  /* We have to assume that there may be instructions between the
     two symbols and that relaxation may increase the distance between
     them.  */
  return FALSE;
}

void
avr_elf_final_processing (void)
{
  if (linkrelax)
    elf_elfheader (stdoutput)->e_flags |= EF_AVR_LINKRELAX_PREPARED;
}

/* Write out the header of a .avr.prop section into the area pointed to by
   DATA.  The RECORD_COUNT will be placed in the header as the number of
   records that are to follow.
   The area DATA must be big enough the receive the header, which is
   AVR_PROPERTY_SECTION_HEADER_SIZE bytes long.  */

static char *
avr_output_property_section_header (char *data,
                                    unsigned int record_count)
{
  char *orig_data = data;

  md_number_to_chars (data, AVR_PROPERTY_RECORDS_VERSION, 1);
  data++;
  /* There's space for a single byte flags field, but right now there's
     nothing to go in here, so just set the value to zero.  */
  md_number_to_chars (data, 0, 1);
  data++;
  md_number_to_chars (data, record_count, 2);
  data+=2;

  gas_assert (data - orig_data == AVR_PROPERTY_SECTION_HEADER_SIZE);

  return data;
}

/* Return the number of bytes required to store RECORD into the .avr.prop
   section. The size returned is the compressed size that corresponds to
   how the record will be written out in AVR_OUTPUT_PROPERTY_RECORD.  */

static int
avr_record_size (const struct avr_property_record *record)
{
  /* The first 5 bytes are a 4-byte address, followed by a 1-byte type
     identifier.  */
  int size = 5;

  switch (record->type)
    {
    case RECORD_ORG:
      size += 0; /* No extra information.  */
      break;

    case RECORD_ORG_AND_FILL:
      size += 4; /* A 4-byte fill value.  */
      break;

    case RECORD_ALIGN:
      size += 4; /* A 4-byte alignment value.  */
      break;

    case RECORD_ALIGN_AND_FILL:
      size += 8; /* A 4-byte alignment, and 4-byte fill value.  */
      break;

    default:
      as_fatal (_("unknown record type %d (in %s)"),
                record->type, __PRETTY_FUNCTION__);
    }

  return size;
}

/* Write out RECORD.  FRAG_BASE points to the start of the data area setup
   to hold all of the .avr.prop content, FRAG_PTR points to the next
   writable location.  The data area must be big enough to hold all of the
   records.  The size of the data written out for this RECORD must match
   the size from AVR_RECORD_SIZE.  */

static char *
avr_output_property_record (char * const frag_base, char *frag_ptr,
                            const struct avr_property_record *record)
{
  fixS *fix;
  int where;
  char *init_frag_ptr = frag_ptr;

  where = frag_ptr - frag_base;
  fix = fix_new (frag_now, where, 4,
                 section_symbol (record->section),
                 record->offset, FALSE, BFD_RELOC_32);
  fix->fx_file = "<internal>";
  fix->fx_line = 0;
  frag_ptr += 4;

  md_number_to_chars (frag_ptr, (bfd_byte) record->type, 1);
  frag_ptr += 1;

  /* Write out the rest of the data.  */
  switch (record->type)
    {
    case RECORD_ORG:
      break;

    case RECORD_ORG_AND_FILL:
      md_number_to_chars (frag_ptr, record->data.org.fill, 4);
      frag_ptr += 4;
      break;

    case RECORD_ALIGN:
      md_number_to_chars (frag_ptr, record->data.align.bytes, 4);
      frag_ptr += 4;
      break;

    case RECORD_ALIGN_AND_FILL:
      md_number_to_chars (frag_ptr, record->data.align.bytes, 4);
      md_number_to_chars (frag_ptr + 4, record->data.align.fill, 4);
      frag_ptr += 8;
      break;

    default:
      as_fatal (_("unknown record type %d (in %s)"),
                record->type, __PRETTY_FUNCTION__);
    }

  gas_assert (frag_ptr - init_frag_ptr == avr_record_size (record));

  return frag_ptr;
}

/* Create the section to hold the AVR property information.  Return the
   section.  */

static asection *
avr_create_property_section (void)
{
  asection *sec;
  flagword flags = (SEC_RELOC | SEC_HAS_CONTENTS | SEC_READONLY);
  const char *section_name = AVR_PROPERTY_RECORD_SECTION_NAME;

  sec = bfd_make_section (stdoutput, section_name);
  if (sec == NULL)
    as_fatal (_("Failed to create property section `%s'\n"), section_name);
  bfd_set_section_flags (stdoutput, sec, flags);
  sec->output_section = sec;
  return sec;
}

/* This hook is called when alignment is performed, and allows us to
   capture the details of both .org and .align directives.  */

void
avr_handle_align (fragS *fragP)
{
  if (linkrelax)
    {
      /* Ignore alignment requests at FR_ADDRESS 0, these are at the very
         start of a section, and will be handled by the standard section
         alignment mechanism.  */
      if ((fragP->fr_type == rs_align
           || fragP->fr_type == rs_align_code)
          && fragP->fr_offset > 0)
        {
          char *p = fragP->fr_literal + fragP->fr_fix;

          fragP->tc_frag_data.is_align = TRUE;
          fragP->tc_frag_data.alignment = fragP->fr_offset;
          fragP->tc_frag_data.fill = *p;
          fragP->tc_frag_data.has_fill = (fragP->tc_frag_data.fill != 0);
        }

      if (fragP->fr_type == rs_org && fragP->fr_offset > 0)
        {
          char *p = fragP->fr_literal + fragP->fr_fix;

          fragP->tc_frag_data.is_org = TRUE;
          fragP->tc_frag_data.fill = *p;
          fragP->tc_frag_data.has_fill = (fragP->tc_frag_data.fill != 0);
        }
    }
}

/* Return TRUE if this section is not one for which we need to record
   information in the avr property section.  */

static bfd_boolean
exclude_section_from_property_tables (segT sec)
{
  /* Only generate property information for sections on which linker
     relaxation could be performed.  */
  return !relaxable_section (sec);
}

/* Create a property record for fragment FRAGP from section SEC and place
   it into an AVR_PROPERTY_RECORD_LINK structure, which can then formed
   into a linked list by the caller.  */

static struct avr_property_record_link *
create_record_for_frag (segT sec, fragS *fragP)
{
  struct avr_property_record_link *prop_rec_link;

  prop_rec_link = XCNEW (struct avr_property_record_link);
  gas_assert (fragP->fr_next != NULL);

  if (fragP->tc_frag_data.is_org)
    {
      prop_rec_link->record.offset = fragP->fr_next->fr_address;
      prop_rec_link->record.section = sec;

      if (fragP->tc_frag_data.has_fill)
        {
          prop_rec_link->record.data.org.fill = fragP->tc_frag_data.fill;
          prop_rec_link->record.type = RECORD_ORG_AND_FILL;
        }
      else
        prop_rec_link->record.type = RECORD_ORG;
    }
  else
    {
      prop_rec_link->record.offset = fragP->fr_next->fr_address;
      prop_rec_link->record.section = sec;

      gas_assert (fragP->tc_frag_data.is_align);
      if (fragP->tc_frag_data.has_fill)
        {
          prop_rec_link->record.data.align.fill = fragP->tc_frag_data.fill;
          prop_rec_link->record.type = RECORD_ALIGN_AND_FILL;
        }
      else
        prop_rec_link->record.type = RECORD_ALIGN;
      prop_rec_link->record.data.align.bytes = fragP->tc_frag_data.alignment;
    }

  return prop_rec_link;
}

/* Build a list of AVR_PROPERTY_RECORD_LINK structures for section SEC, and
   merged them onto the list pointed to by NEXT_PTR.  Return a pointer to
   the last list item created.  */

static struct avr_property_record_link **
append_records_for_section (segT sec,
                            struct avr_property_record_link **next_ptr)
{
  segment_info_type *seginfo = seg_info (sec);
  fragS *fragP;

  if (seginfo && seginfo->frchainP)
    {
      for (fragP = seginfo->frchainP->frch_root;
           fragP;
           fragP = fragP->fr_next)
	{
          if (fragP->tc_frag_data.is_align
              || fragP->tc_frag_data.is_org)
            {
              /* Create a single new entry.  */
              struct avr_property_record_link *new_link
                = create_record_for_frag (sec, fragP);

              *next_ptr = new_link;
              next_ptr = &new_link->next;
            }
	}
    }

  return next_ptr;
}

/* Create the AVR property section and fill it with records of .org and
   .align directives that were used.  The section is only created if it
   will actually have any content.  */

static void
avr_create_and_fill_property_section (void)
{
  segT *seclist;
  asection *prop_sec;
  struct avr_property_record_link *r_list, **next_ptr;
  char *frag_ptr, *frag_base;
  bfd_size_type sec_size;
  struct avr_property_record_link *rec;
  unsigned int record_count;

  /* First walk over all sections.  For sections on which linker
     relaxation could be applied, extend the record list.  The record list
     holds information that the linker will need to know.  */

  prop_sec = NULL;
  r_list = NULL;
  next_ptr = &r_list;
  for (seclist = &stdoutput->sections;
       seclist && *seclist;
       seclist = &(*seclist)->next)
    {
      segT sec = *seclist;

      if (exclude_section_from_property_tables (sec))
	continue;

      next_ptr = append_records_for_section (sec, next_ptr);
    }

  /* Create property section and ensure the size is correct.  We've already
     passed the point where gas could size this for us.  */
  sec_size = AVR_PROPERTY_SECTION_HEADER_SIZE;
  record_count = 0;
  for (rec = r_list; rec != NULL; rec = rec->next)
    {
      record_count++;
      sec_size += avr_record_size (&rec->record);
    }

  if (record_count == 0)
    return;

  prop_sec = avr_create_property_section ();
  bfd_set_section_size (stdoutput, prop_sec, sec_size);

  subseg_set (prop_sec, 0);
  frag_base = frag_more (sec_size);

  frag_ptr =
    avr_output_property_section_header (frag_base, record_count);

  for (rec = r_list; rec != NULL; rec = rec->next)
    frag_ptr = avr_output_property_record (frag_base, frag_ptr, &rec->record);

  frag_wane (frag_now);
  frag_new (0);
  frag_wane (frag_now);
}

/* We're using this hook to build up the AVR property section.  It's called
   late in the assembly process which suits our needs.  */
void
avr_post_relax_hook (void)
{
  avr_create_and_fill_property_section ();
}
