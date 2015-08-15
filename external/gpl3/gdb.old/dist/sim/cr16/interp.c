/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2014 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License 
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "bfd.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"

#include "cr16_sim.h"
#include "gdb/sim-cr16.h"
#include "gdb/signals.h"
#include "opcode/cr16.h"

static char *myname;
static SIM_OPEN_KIND sim_kind;
int cr16_debug;

/* Set this to true to get the previous segment layout. */

int old_segment_mapping;

host_callback *cr16_callback;
unsigned long ins_type_counters[ (int)INS_MAX ];

uint32 OP[4];
uint32 sign_flag;

static int init_text_p = 0;
/* non-zero if we opened prog_bfd */
static int prog_bfd_was_opened_p;
bfd *prog_bfd;
asection *text;
bfd_vma text_start;
bfd_vma text_end;

static struct hash_entry *lookup_hash (uint64 ins, int size);
static void get_operands (operand_desc *s, uint64 mcode, int isize, int nops);
static int do_run (uint64 mc);
static char *add_commas (char *buf, int sizeof_buf, unsigned long value);
extern void sim_set_profile (int n);
extern void sim_set_profile_size (int n);
static INLINE uint8 *map_memory (unsigned phys_addr);

#ifdef NEED_UI_LOOP_HOOK
/* How often to run the ui_loop update, when in use */
#define UI_LOOP_POLL_INTERVAL 0x14000

/* Counter for the ui_loop_hook update */
static long ui_loop_hook_counter = UI_LOOP_POLL_INTERVAL;

/* Actual hook to call to run through gdb's gui event loop */
extern int (*deprecated_ui_loop_hook) (int signo);
#endif /* NEED_UI_LOOP_HOOK */

#ifndef INLINE
#if defined(__GNUC__) && defined(__OPTIMIZE__)
#define INLINE __inline__
#else
#define INLINE
#endif
#endif
#define MAX_HASH  16

struct hash_entry
{
  struct hash_entry *next;
  uint32 opcode;
  uint32 mask;
  int format;
  int size;
  struct simops *ops;
};

struct hash_entry hash_table[MAX_HASH+1];

INLINE static long
hash(unsigned long long insn, int format)
{ 
  unsigned int i = 4, tmp;
  if (format)
    {
      while ((insn >> i) != 0) i +=4;

      return ((insn >> (i-4)) & 0xf); /* Use last 4 bits as hask key.  */
    }
  return ((insn & 0xF)); /* Use last 4 bits as hask key.  */
}


INLINE static struct hash_entry *
lookup_hash (uint64 ins, int size)
{
  uint32 mask;
  struct hash_entry *h;

  h = &hash_table[hash(ins,1)];


  mask = (((1 << (32 - h->mask)) -1) << h->mask);

 /* Adjuest mask for branch with 2 word instructions.  */
  if ((h->ops->mnimonic != NULL) &&
      ((streq(h->ops->mnimonic,"b") && h->size == 2)))
    mask = 0xff0f0000;


  while ((ins & mask) != (BIN(h->opcode, h->mask)))
    {
      if (h->next == NULL)
        {
          State.exception = SIGILL;
          State.pc_changed = 1; /* Don't increment the PC. */
          return NULL;
        }
      h = h->next;

      mask = (((1 << (32 - h->mask)) -1) << h->mask);
     /* Adjuest mask for branch with 2 word instructions.  */
     if ((streq(h->ops->mnimonic,"b")) && h->size == 2)
       mask = 0xff0f0000;

     }
   return (h);
}

INLINE static void
get_operands (operand_desc *s, uint64 ins, int isize, int nops)
{
  uint32 i, opn = 0, start_bit = 0, op_type = 0; 
  int32 op_size = 0, mask = 0;

  if (isize == 1) /* Trunkcate the extra 16 bits of INS.  */
    ins = ins >> 16;

  for (i=0; i < 4; ++i,++opn)
    {
      if (s[opn].op_type == dummy) break;

      op_type = s[opn].op_type;
      start_bit = s[opn].shift;
      op_size = cr16_optab[op_type].bit_size;

      switch (op_type)
        {
          case imm3: case imm4: case imm5: case imm6:
            {
             if (isize == 1)
               OP[i] = ((ins >> 4) & ((1 << op_size) -1));
             else
               OP[i] = ((ins >> (32 - start_bit)) & ((1 << op_size) -1));

             if (OP[i] & ((long)1 << (op_size -1))) 
               {
                 sign_flag = 1;
                 OP[i] = ~(OP[i]) + 1;
               }
             OP[i] = (unsigned long int)(OP[i] & (((long)1 << op_size) -1));
            }
            break;

          case uimm3: case uimm3_1: case uimm4_1:
             switch (isize)
               {
              case 1:
               OP[i] = ((ins >> 4) & ((1 << op_size) -1)); break;
              case 2:
               OP[i] = ((ins >> (32 - start_bit)) & ((1 << op_size) -1));break;
              default: /* for case 3.  */
               OP[i] = ((ins >> (16 + start_bit)) & ((1 << op_size) -1)); break;
               break;
               }
            break;

          case uimm4:
            switch (isize)
              {
              case 1:
                 if (start_bit == 20)
                   OP[i] = ((ins >> 4) & ((1 << op_size) -1));
                 else
                   OP[i] = (ins & ((1 << op_size) -1));
                 break;
              case 2:
                 OP[i] = ((ins >> start_bit) & ((1 << op_size) -1));
                 break;
              case 3:
                 OP[i] = ((ins >> (start_bit + 16)) & ((1 << op_size) -1));
                 break;
              default:
                 OP[i] = ((ins >> start_bit) & ((1 << op_size) -1));
                 break;
              }
            break;

          case imm16: case uimm16:
            OP[i] = ins & 0xFFFF;
            break;

          case uimm20: case imm20:
            OP[i] = ins & (((long)1 << op_size) - 1);
            break;

          case imm32: case uimm32:
            OP[i] = ins & 0xFFFFFFFF;
            break;

          case uimm5: break; /*NOT USED.  */
            OP[i] = ins & ((1 << op_size) - 1); break;

          case disps5: 
            OP[i] = (ins >> 4) & ((1 << 4) - 1); 
            OP[i] = (OP[i] * 2) + 2;
            if (OP[i] & ((long)1 << 5)) 
              {
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0x1F);
              }
            break;

          case dispe9: 
            OP[i] = ((((ins >> 8) & 0xf) << 4) | (ins & 0xf)); 
            OP[i] <<= 1;
            if (OP[i] & ((long)1 << 8)) 
              {
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFF);
              }
            break;

          case disps17: 
            OP[i] = (ins & 0xFFFF);
            if (OP[i] & 1) 
              {
                OP[i] = (OP[i] & 0xFFFE);
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFFFF);
              }
            break;

          case disps25: 
            if (isize == 2)
              OP[i] = (ins & 0xFFFFFF);
            else 
              OP[i] = (ins & 0xFFFF) | (((ins >> 24) & 0xf) << 16) |
                      (((ins >> 16) & 0xf) << 20);

            if (OP[i] & 1) 
              {
                OP[i] = (OP[i] & 0xFFFFFE);
                sign_flag = 1;
                OP[i] = ~(OP[i]) + 1;
                OP[i] = (unsigned long int)(OP[i] & 0xFFFFFF);
              }
            break;

          case abs20:
            if (isize == 3)
              OP[i] = (ins) & 0xFFFFF; 
            else
              OP[i] = (ins >> start_bit) & 0xFFFFF;
            break;
          case abs24:
            if (isize == 3)
              OP[i] = ((ins & 0xFFFF) | (((ins >> 16) & 0xf) << 20)
                       | (((ins >> 24) & 0xf) << 16));
            else
              OP[i] = (ins >> 16) & 0xFFFFFF;
            break;

          case rra:
          case rbase: break; /* NOT USED.  */
          case rbase_disps20:  case rbase_dispe20:
          case rpbase_disps20: case rpindex_disps20:
            OP[i] = ((((ins >> 24)&0xf) << 16)|((ins) & 0xFFFF));
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
            break;
          case rpbase_disps0:
            OP[i] = 0;                       /* 4 bit disp const.  */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_dispe4:
            OP[i] = ((ins >> 8) & 0xF) * 2;  /* 4 bit disp const.   */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_disps4:
            OP[i] = ((ins >> 8) & 0xF);      /* 4 bit disp const.  */
            OP[++i] = (ins) & 0xF;           /* get 4 bit for reg.  */
            break;
          case rpbase_disps16:
            OP[i] = (ins) & 0xFFFF;
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
            break;
          case rpindex_disps0:
            OP[i] = 0;
            OP[++i] = (ins >> 4) & 0xF;      /* get 4 bit for reg.  */
            OP[++i] = (ins >> 8) & 0x1;      /* get 1 bit for index-reg.  */
            break;
          case rpindex_disps14:
            OP[i] = (ins) & 0x3FFF;
            OP[++i] = (ins >> 14) & 0x1;     /* get 1 bit for index-reg.  */
            OP[++i] = (ins >> 16) & 0xF;     /* get 4 bit for reg.  */
          case rindex7_abs20:
          case rindex8_abs20:
            OP[i] = (ins) & 0xFFFFF;
            OP[++i] = (ins >> 24) & 0x1;     /* get 1 bit for index-reg.  */
            OP[++i] = (ins >> 20) & 0xF;     /* get 4 bit for reg.  */
            break;
          case regr: case regp: case pregr: case pregrp:
              switch(isize)
                {
                  case 1: 
                    if (start_bit == 20) OP[i] = (ins >> 4) & 0xF;
                    else if (start_bit == 16) OP[i] = ins & 0xF;
                    break;
                  case 2: OP[i] = (ins >>  start_bit) & 0xF; break;
                  case 3: OP[i] = (ins >> (start_bit + 16)) & 0xF; break;
                }
               break;
          case cc: 
            {
              if (isize == 1) OP[i] = (ins >> 4) & 0xF;
              else if (isize == 2)  OP[i] = (ins >> start_bit)  & 0xF;
              else  OP[i] = (ins >> (start_bit + 16)) & 0xF; 
              break;
            }
          default: break;
        }
     
      /* For ESC on uimm4_1 operand.  */
      if (op_type == uimm4_1)
        if (OP[i] == 9)
           OP[i] = -1;

      /* For increment by 1.  */
      if ((op_type == pregr) || (op_type == pregrp))
          OP[i] += 1;
   }
  /* FIXME: for tracing, update values that need to be updated each
            instruction decode cycle */
  State.trace.psw = PSR;
}

bfd_vma
decode_pc ()
{
  asection *s;
  if (!init_text_p && prog_bfd != NULL)
    {
      init_text_p = 1;
      for (s = prog_bfd->sections; s; s = s->next)
        if (strcmp (bfd_get_section_name (prog_bfd, s), ".text") == 0)
          {
            text = s;
            text_start = bfd_get_section_vma (prog_bfd, s);
            text_end = text_start + bfd_section_size (prog_bfd, s);
            break;
           }
     }

  return (PC) + text_start;
}



static int
do_run(uint64 mcode)
{
  struct simops *s= Simops;
  struct hash_entry *h;
  char func[12]="\0";
  uint8 *iaddr;
#ifdef DEBUG
  if ((cr16_debug & DEBUG_INSTRUCTION) != 0)
    (*cr16_callback->printf_filtered) (cr16_callback, "do_long 0x%x\n", mcode);
#endif
  
   h =  lookup_hash(mcode, 1);

   if ((h == NULL) || (h->opcode == NULL)) return 0;

   if (h->size == 3)
    {
      iaddr = imem_addr ((uint32)PC + 2);
       mcode = (mcode << 16) | get_longword( iaddr );
    }

  /* Re-set OP list.  */
  OP[0] = OP[1] = OP[2] = OP[3] = sign_flag = 0;

  /* for push/pop/pushrtn with RA instructions. */
  if ((h->format & REG_LIST) && (mcode & 0x800000))
    OP[2] = 1; /* Set 1 for RA operand.  */

  /* numops == 0 means, no operands.  */
  if (((h->ops) != NULL) && (((h->ops)->numops) != 0))
    get_operands ((h->ops)->operands, mcode, h->size, (h->ops)->numops);

  //State.ins_type = h->flags;

  (h->ops->func)();

  return h->size;
}

static char *
add_commas(char *buf, int sizeof_buf, unsigned long value)
{
  int comma = 3;
  char *endbuf = buf + sizeof_buf - 1;

  *--endbuf = '\0';
  do {
    if (comma-- == 0)
      {
        *--endbuf = ',';
        comma = 2;
      }

    *--endbuf = (value % 10) + '0';
  } while ((value /= 10) != 0);

  return endbuf;
}

void
sim_size (int power)
{
  int i;
  for (i = 0; i < IMEM_SEGMENTS; i++)
    {
      if (State.mem.insn[i])
        free (State.mem.insn[i]);
    }
  for (i = 0; i < DMEM_SEGMENTS; i++)
    {
      if (State.mem.data[i])
        free (State.mem.data[i]);
    }
  for (i = 0; i < UMEM_SEGMENTS; i++)
    {
      if (State.mem.unif[i])
        free (State.mem.unif[i]);
    }
  /* Always allocate dmem segment 0.  This contains the IMAP and DMAP
     registers. */
  State.mem.data[0] = calloc (1, SEGMENT_SIZE);
}

/* For tracing - leave info on last access around. */
static char *last_segname = "invalid";
static char *last_from = "invalid";
static char *last_to = "invalid";

enum
  {
    IMAP0_OFFSET = 0xff00,
    DMAP0_OFFSET = 0xff08,
    DMAP2_SHADDOW = 0xff04,
    DMAP2_OFFSET = 0xff0c
  };

static void
set_dmap_register (int reg_nr, unsigned long value)
{
  uint8 *raw = map_memory (SIM_CR16_MEMORY_DATA
                           + DMAP0_OFFSET + 2 * reg_nr);
  WRITE_16 (raw, value);
#ifdef DEBUG
  if ((cr16_debug & DEBUG_MEMORY))
    {
      (*cr16_callback->printf_filtered)
        (cr16_callback, "mem: dmap%d=0x%04lx\n", reg_nr, value);
    }
#endif
}

static unsigned long
dmap_register (void *regcache, int reg_nr)
{
  uint8 *raw = map_memory (SIM_CR16_MEMORY_DATA
                           + DMAP0_OFFSET + 2 * reg_nr);
  return READ_16 (raw);
}

static void
set_imap_register (int reg_nr, unsigned long value)
{
  uint8 *raw = map_memory (SIM_CR16_MEMORY_DATA
                           + IMAP0_OFFSET + 2 * reg_nr);
  WRITE_16 (raw, value);
#ifdef DEBUG
  if ((cr16_debug & DEBUG_MEMORY))
    {
      (*cr16_callback->printf_filtered)
        (cr16_callback, "mem: imap%d=0x%04lx\n", reg_nr, value);
    }
#endif
}

static unsigned long
imap_register (void *regcache, int reg_nr)
{
  uint8 *raw = map_memory (SIM_CR16_MEMORY_DATA
                           + IMAP0_OFFSET + 2 * reg_nr);
  return READ_16 (raw);
}

enum
  {
    HELD_SPI_IDX = 0,
    HELD_SPU_IDX = 1
  };

static unsigned long
spu_register (void)
{
    return GPR (SP_IDX);
}

static unsigned long
spi_register (void)
{
    return GPR (SP_IDX);
}

static void
set_spi_register (unsigned long value)
{
    SET_GPR (SP_IDX, value);
}

static void
set_spu_register  (unsigned long value)
{
    SET_GPR (SP_IDX, value);
}

/* Given a virtual address in the DMAP address space, translate it
   into a physical address. */

unsigned long
sim_cr16_translate_dmap_addr (unsigned long offset,
                              int nr_bytes,
                              unsigned long *phys,
                              void *regcache,
                              unsigned long (*dmap_register) (void *regcache,
                                                              int reg_nr))
{
  short map;
  int regno;
  last_from = "logical-data";
  if (offset >= DMAP_BLOCK_SIZE * SIM_CR16_NR_DMAP_REGS)
    {
      /* Logical address out side of data segments, not supported */
      return 0;
    }
  regno = (offset / DMAP_BLOCK_SIZE);
  offset = (offset % DMAP_BLOCK_SIZE);

#if 1
  if ((offset % DMAP_BLOCK_SIZE) + nr_bytes > DMAP_BLOCK_SIZE)
    {
      /* Don't cross a BLOCK boundary */
      nr_bytes = DMAP_BLOCK_SIZE - (offset % DMAP_BLOCK_SIZE);
    }
  map = dmap_register (regcache, regno);
  if (regno == 3)
    {
      /* Always maps to data memory */
      int iospi = (offset / 0x1000) % 4;
      int iosp = (map >> (4 * (3 - iospi))) % 0x10;
      last_to = "io-space";
      *phys = (SIM_CR16_MEMORY_DATA + (iosp * 0x10000) + 0xc000 + offset);
    }
  else
    {
      int sp = ((map & 0x3000) >> 12);
      int segno = (map & 0x3ff);
      switch (sp)
        {
        case 0: /* 00: Unified memory */
          *phys = SIM_CR16_MEMORY_UNIFIED + (segno * DMAP_BLOCK_SIZE) + offset;
          last_to = "unified";
          break;
        case 1: /* 01: Instruction Memory */
          *phys = SIM_CR16_MEMORY_INSN + (segno * DMAP_BLOCK_SIZE) + offset;
          last_to = "chip-insn";
          break;
        case 2: /* 10: Internal data memory */
          *phys = SIM_CR16_MEMORY_DATA + (segno << 16) + (regno * DMAP_BLOCK_SIZE) + offset;
          last_to = "chip-data";
          break;
        case 3: /* 11: Reserved */
          return 0;
        }
    }
#endif
  return nr_bytes;
}

/* Given a virtual address in the IMAP address space, translate it
   into a physical address. */

unsigned long
sim_cr16_translate_imap_addr (unsigned long offset,
                              int nr_bytes,
                              unsigned long *phys,
                              void *regcache,
                              unsigned long (*imap_register) (void *regcache,
                                                              int reg_nr))
{
  short map;
  int regno;
  int sp;
  int segno;
  last_from = "logical-insn";
  if (offset >= (IMAP_BLOCK_SIZE * SIM_CR16_NR_IMAP_REGS))
    {
      /* Logical address outside of IMAP segments, not supported */
      return 0;
    }
  regno = (offset / IMAP_BLOCK_SIZE);
  offset = (offset % IMAP_BLOCK_SIZE);
  if (offset + nr_bytes > IMAP_BLOCK_SIZE)
    {
      /* Don't cross a BLOCK boundary */
      nr_bytes = IMAP_BLOCK_SIZE - offset;
    }
  map = imap_register (regcache, regno);
  sp = (map & 0x3000) >> 12;
  segno = (map & 0x007f);
  switch (sp)
    {
    case 0: /* 00: unified memory */
      *phys = SIM_CR16_MEMORY_UNIFIED + (segno << 17) + offset;
      last_to = "unified";
      break;
    case 1: /* 01: instruction memory */
      *phys = SIM_CR16_MEMORY_INSN + (IMAP_BLOCK_SIZE * regno) + offset;
      last_to = "chip-insn";
      break;
    case 2: /*10*/
      /* Reserved. */
      return 0;
    case 3: /* 11: for testing  - instruction memory */
      offset = (offset % 0x800);
      *phys = SIM_CR16_MEMORY_INSN + offset;
      if (offset + nr_bytes > 0x800)
        /* don't cross VM boundary */
        nr_bytes = 0x800 - offset;
      last_to = "test-insn";
      break;
    }
  return nr_bytes;
}

unsigned long
sim_cr16_translate_addr (unsigned long memaddr, int nr_bytes,
                         unsigned long *targ_addr, void *regcache,
                         unsigned long (*dmap_register) (void *regcache,
                                                         int reg_nr),
                         unsigned long (*imap_register) (void *regcache,
                                                         int reg_nr))
{
  unsigned long phys;
  unsigned long seg;
  unsigned long off;

  last_from = "unknown";
  last_to = "unknown";

  seg = (memaddr >> 24);
  off = (memaddr & 0xffffffL);

  /* However, if we've asked to use the previous generation of segment
     mapping, rearrange the segments as follows. */

  if (old_segment_mapping)
    {
      switch (seg)
        {
        case 0x00: /* DMAP translated memory */
          seg = 0x10;
          break;
        case 0x01: /* IMAP translated memory */
          seg = 0x11;
          break;
        case 0x10: /* On-chip data memory */
          seg = 0x02;
          break;
        case 0x11: /* On-chip insn memory */
          seg = 0x01;
          break;
        case 0x12: /* Unified memory */
          seg = 0x00;
          break;
        }
    }

  switch (seg)
    {
    case 0x00:                        /* Physical unified memory */
      last_from = "phys-unified";
      last_to = "unified";
      phys = SIM_CR16_MEMORY_UNIFIED + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
        nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x01:                        /* Physical instruction memory */
      last_from = "phys-insn";
      last_to = "chip-insn";
      phys = SIM_CR16_MEMORY_INSN + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
        nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x02:                        /* Physical data memory segment */
      last_from = "phys-data";
      last_to = "chip-data";
      phys = SIM_CR16_MEMORY_DATA + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
        nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x10:                        /* in logical data address segment */
      nr_bytes = sim_cr16_translate_dmap_addr (off, nr_bytes, &phys, regcache,
                                               dmap_register);
      break;

    case 0x11:                        /* in logical instruction address segment */
      nr_bytes = sim_cr16_translate_imap_addr (off, nr_bytes, &phys, regcache,
                                               imap_register);
      break;

    default:
      return 0;
    }

  *targ_addr = phys;
  return nr_bytes;
}

/* Return a pointer into the raw buffer designated by phys_addr.  It
   is assumed that the client has already ensured that the access
   isn't going to cross a segment boundary. */

uint8 *
map_memory (unsigned phys_addr)
{
  uint8 **memory;
  uint8 *raw;
  unsigned offset;
  int segment = ((phys_addr >> 24) & 0xff);
  
  switch (segment)
    {
      
    case 0x00: /* Unified memory */
      {
        memory = &State.mem.unif[(phys_addr / SEGMENT_SIZE) % UMEM_SEGMENTS];
        last_segname = "umem";
        break;
      }
    
    case 0x01: /* On-chip insn memory */
      {
        memory = &State.mem.insn[(phys_addr / SEGMENT_SIZE) % IMEM_SEGMENTS];
        last_segname = "imem";
        break;
      }
    
    case 0x02: /* On-chip data memory */
      {
        if ((phys_addr & 0xff00) == 0xff00)
          {
            phys_addr = (phys_addr & 0xffff);
            if (phys_addr == DMAP2_SHADDOW)
              {
                phys_addr = DMAP2_OFFSET;
                last_segname = "dmap";
              }
            else
              last_segname = "reg";
          }
        else
          last_segname = "dmem";
        memory = &State.mem.data[(phys_addr / SEGMENT_SIZE) % DMEM_SEGMENTS];
        break;
      }
    
    default:
      /* OOPS! */
      last_segname = "scrap";
      return State.mem.fault;
    }
  
  if (*memory == NULL)
    {
      *memory = calloc (1, SEGMENT_SIZE);
      if (*memory == NULL)
        {
          (*cr16_callback->printf_filtered) (cr16_callback, "Malloc failed.\n");
          return State.mem.fault;
        }
    }
  
  offset = (phys_addr % SEGMENT_SIZE);
  raw = *memory + offset;
  return raw;
}
  
/* Transfer data to/from simulated memory.  Since a bug in either the
   simulated program or in gdb or the simulator itself may cause a
   bogus address to be passed in, we need to do some sanity checking
   on addresses to make sure they are within bounds.  When an address
   fails the bounds check, treat it as a zero length read/write rather
   than aborting the entire run. */

static int
xfer_mem (SIM_ADDR virt,
          unsigned char *buffer,
          int size,
          int write_p)
{
  uint8 *memory;
  unsigned long phys;
  int phys_size;
  phys_size = sim_cr16_translate_addr (virt, size, &phys, NULL,
                                       dmap_register, imap_register);
  if (phys_size == 0)
    return 0;

  memory = map_memory (phys);

#ifdef DEBUG
  if ((cr16_debug & DEBUG_INSTRUCTION) != 0)
    {
      (*cr16_callback->printf_filtered)
        (cr16_callback,
         "sim_%s %d bytes: 0x%08lx (%s) -> 0x%08lx (%s) -> 0x%08lx (%s)\n",
             (write_p ? "write" : "read"),
         phys_size, virt, last_from,
         phys, last_to,
         (long) memory, last_segname);
    }
#endif

  if (write_p)
    {
      memcpy (memory, buffer, phys_size);
    }
  else
    {
      memcpy (buffer, memory, phys_size);
    }
  
  return phys_size;
}


int
sim_write (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     const unsigned char *buffer;
     int size;
{
  /* FIXME: this should be performing a virtual transfer */
  return xfer_mem( addr, buffer, size, 1);
}

int
sim_read (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  /* FIXME: this should be performing a virtual transfer */
  return xfer_mem( addr, buffer, size, 0);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, struct host_callback_struct *callback, struct bfd *abfd, char **argv)
{
  struct simops *s;
  struct hash_entry *h;
  static int init_p = 0;
  char **p;

  sim_kind = kind;
  cr16_callback = callback;
  myname = argv[0];
  old_segment_mapping = 0;

  /* NOTE: This argument parsing is only effective when this function
     is called by GDB. Standalone argument parsing is handled by
     sim/common/run.c. */
#if 0
  for (p = argv + 1; *p; ++p)
    {
      if (strcmp (*p, "-oldseg") == 0)
        old_segment_mapping = 1;
#ifdef DEBUG
      else if (strcmp (*p, "-t") == 0)
        cr16_debug = DEBUG;
      else if (strncmp (*p, "-t", 2) == 0)
        cr16_debug = atoi (*p + 2);
#endif
      else
        (*cr16_callback->printf_filtered) (cr16_callback, "ERROR: unsupported option(s): %s\n",*p);
    }
#endif
  
  /* put all the opcodes in the hash table.  */
  if (!init_p++)
    {
      for (s = Simops; s->func; s++)
        {
          switch(32 - s->mask)
            {
            case 0x4:
               h = &hash_table[hash(s->opcode, 0)]; 
               break;

            case 0x7:
               if (((s->opcode << 1) >> 4) != 0)
                  h = &hash_table[hash((s->opcode << 1) >> 4, 0)];
               else
                  h = &hash_table[hash((s->opcode << 1), 0)];
               break;

            case 0x8:
               if ((s->opcode >> 4) != 0)
                  h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                  h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0x9:
               if (((s->opcode  >> 1) >> 4) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 4, 0)]; 
               else 
                 h = &hash_table[hash((s->opcode >> 1), 0)]; 
               break;

            case 0xa:
               if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                 h = &hash_table[hash(s->opcode, 0)]; 
               break;

            case 0xc:
               if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else
                 h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0xd:
               if (((s->opcode >> 1) >> 8) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 8, 0)];
               else if (((s->opcode >> 1) >> 4) != 0)
                 h = &hash_table[hash((s->opcode >>1) >> 4, 0)];
               else
                 h = &hash_table[hash((s->opcode >>1), 0)];
               break;

            case 0x10:
               if ((s->opcode >> 0xc) != 0)
                 h = &hash_table[hash(s->opcode >> 12, 0)]; 
               else if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else 
                 h = &hash_table[hash(s->opcode, 0)];
               break;

            case 0x14:
               if ((s->opcode >> 16) != 0)
                 h = &hash_table[hash(s->opcode >> 16, 0)];
               else if ((s->opcode >> 12) != 0)
                 h = &hash_table[hash(s->opcode >> 12, 0)];
               else if ((s->opcode >> 8) != 0)
                 h = &hash_table[hash(s->opcode >> 8, 0)];
               else if ((s->opcode >> 4) != 0)
                 h = &hash_table[hash(s->opcode >> 4, 0)];
               else 
                 h = &hash_table[hash(s->opcode, 0)];
               break;
            default:
              break;
            }
      
          /* go to the last entry in the chain.  */
          while (h->next)
            h = h->next;

          if (h->ops)
            {
              h->next = (struct hash_entry *) calloc(1,sizeof(struct hash_entry));
              if (!h->next)
                perror ("malloc failure");

              h = h->next;
            }
          h->ops = s;
          h->mask = s->mask;
          h->opcode = s->opcode;
          h->format = s->format;
          h->size = s->size;
        }
    }

  /* reset the processor state */
  if (!State.mem.data[0])
    sim_size (1);
  sim_create_inferior ((SIM_DESC) 1, NULL, NULL, NULL);

  /* Fudge our descriptor.  */
  return (SIM_DESC) 1;
}


void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  if (prog_bfd != NULL && prog_bfd_was_opened_p)
    {
      bfd_close (prog_bfd);
      prog_bfd = NULL;
      prog_bfd_was_opened_p = 0;
    }
}

void
sim_set_profile (int n)
{
  (*cr16_callback->printf_filtered) (cr16_callback, "sim_set_profile %d\n",n);
}

void
sim_set_profile_size (int n)
{
  (*cr16_callback->printf_filtered) (cr16_callback, "sim_set_profile_size %d\n",n);
}

uint8 *
dmem_addr (uint32 offset)
{
  unsigned long phys;
  uint8 *mem;
  int phys_size;

  /* Note: DMEM address range is 0..0x10000. Calling code can compute
     things like ``0xfffe + 0x0e60 == 0x10e5d''.  Since offset's type
     is uint16 this is modulo'ed onto 0x0e5d. */

  phys_size = sim_cr16_translate_dmap_addr (offset, 1, &phys, NULL,
                                            dmap_register);
  if (phys_size == 0)
    {
      mem = State.mem.fault;
    }
  else
    mem = map_memory (phys);
#ifdef DEBUG
  if ((cr16_debug & DEBUG_MEMORY))
    {
      (*cr16_callback->printf_filtered)
        (cr16_callback,
         "mem: 0x%08x (%s) -> 0x%08lx %d (%s) -> 0x%08lx (%s)\n",
         offset, last_from,
         phys, phys_size, last_to,
         (long) mem, last_segname);
    }
#endif
  return mem;
}

uint8 *
imem_addr (uint32 offset)
{
  unsigned long phys;
  uint8 *mem;
  int phys_size = sim_cr16_translate_imap_addr (offset, 1, &phys, NULL,
                                                imap_register);
  if (phys_size == 0)
    {
      return State.mem.fault;
    }
  mem = map_memory (phys); 
#ifdef DEBUG
  if ((cr16_debug & DEBUG_MEMORY))
    {
      (*cr16_callback->printf_filtered)
        (cr16_callback,
         "mem: 0x%08x (%s) -> 0x%08lx %d (%s) -> 0x%08lx (%s)\n",
         offset, last_from,
         phys, phys_size, last_to,
         (long) mem, last_segname);
    }
#endif
  return mem;
}

static int stop_simulator = 0;

int
sim_stop (sd)
     SIM_DESC sd;
{
  stop_simulator = 1;
  return 1;
}


/* Run (or resume) the program.  */
void
sim_resume (SIM_DESC sd, int step, int siggnal)
{
  uint32 curr_ins_size = 0;
  uint64 mcode = 0;
  uint8 *iaddr;

#ifdef DEBUG
//  (*cr16_callback->printf_filtered) (cr16_callback, "sim_resume (%d,%d)  PC=0x%x\n",step,siggnal,PC); 
#endif

  State.exception = 0;
  if (step)
    sim_stop (sd);

  switch (siggnal)
    {
    case 0:
      break;
#ifdef SIGBUS
    case SIGBUS:
#endif
    case SIGSEGV:
      SET_PC (PC);
      SET_PSR (PSR);
      JMP (AE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    case SIGILL:
      SET_PC (PC);
      SET_PSR (PSR);
      SET_HW_PSR ((PSR & (PSR_C_BIT)));
      JMP (RIE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    default:
      /* just ignore it */
      break;
    }

  do
    {
      iaddr = imem_addr ((uint32)PC);
      if (iaddr == State.mem.fault)
        {
#ifdef SIGBUS
          State.exception = SIGBUS;
#else
          State.exception = SIGSEGV;
#endif
          break;
        }
 
      mcode = get_longword( iaddr ); 
 
      State.pc_changed = 0;
      
      curr_ins_size = do_run(mcode);

#if CR16_DEBUG
 (*cr16_callback->printf_filtered) (cr16_callback, "INS: PC=0x%X, mcode=0x%X\n",PC,mcode); 
#endif

      if (!State.pc_changed)
        {
          if (curr_ins_size == 0) 
           {
             State.exception = SIG_CR16_EXIT; /* exit trap */
             break;
           }
          else
           SET_PC (PC + (curr_ins_size * 2)); /* For word instructions.  */
        }

#if 0
      /* Check for a breakpoint trap on this instruction.  This
         overrides any pending branches or loops */
      if (PSR_DB && PC == DBS)
        {
          SET_BPC (PC);
          SET_BPSR (PSR);
          SET_PC (SDBT_VECTOR_START);
        }
#endif

      /* Writeback all the DATA / PC changes */
      SLOT_FLUSH ();

#ifdef NEED_UI_LOOP_HOOK
      if (deprecated_ui_loop_hook != NULL && ui_loop_hook_counter-- < 0)
        {
          ui_loop_hook_counter = UI_LOOP_POLL_INTERVAL;
          deprecated_ui_loop_hook (0);
        }
#endif /* NEED_UI_LOOP_HOOK */
    }
  while ( !State.exception && !stop_simulator);
  
  if (step && !State.exception)
    State.exception = SIGTRAP;
}

void
sim_set_trace (void)
{
#ifdef DEBUG
  cr16_debug = DEBUG;
#endif
}

void
sim_info (SIM_DESC sd, int verbose)
{
  char buf1[40];
  char buf2[40];
  char buf3[40];
  char buf4[40];
  char buf5[40];
#if 0
  unsigned long left                = ins_type_counters[ (int)INS_LEFT ] + ins_type_counters[ (int)INS_LEFT_COND_EXE ];
  unsigned long left_nops        = ins_type_counters[ (int)INS_LEFT_NOPS ];
  unsigned long left_parallel        = ins_type_counters[ (int)INS_LEFT_PARALLEL ];
  unsigned long left_cond        = ins_type_counters[ (int)INS_LEFT_COND_TEST ];
  unsigned long left_total        = left + left_parallel + left_cond + left_nops;

  unsigned long right                = ins_type_counters[ (int)INS_RIGHT ] + ins_type_counters[ (int)INS_RIGHT_COND_EXE ];
  unsigned long right_nops        = ins_type_counters[ (int)INS_RIGHT_NOPS ];
  unsigned long right_parallel        = ins_type_counters[ (int)INS_RIGHT_PARALLEL ];
  unsigned long right_cond        = ins_type_counters[ (int)INS_RIGHT_COND_TEST ];
  unsigned long right_total        = right + right_parallel + right_cond + right_nops;

  unsigned long unknown                = ins_type_counters[ (int)INS_UNKNOWN ];
  unsigned long ins_long        = ins_type_counters[ (int)INS_LONG ];
  unsigned long parallel        = ins_type_counters[ (int)INS_PARALLEL ];
  unsigned long leftright        = ins_type_counters[ (int)INS_LEFTRIGHT ];
  unsigned long rightleft        = ins_type_counters[ (int)INS_RIGHTLEFT ];
  unsigned long cond_true        = ins_type_counters[ (int)INS_COND_TRUE ];
  unsigned long cond_false        = ins_type_counters[ (int)INS_COND_FALSE ];
  unsigned long cond_jump        = ins_type_counters[ (int)INS_COND_JUMP ];
  unsigned long cycles                = ins_type_counters[ (int)INS_CYCLES ];
  unsigned long total                = (unknown + left_total + right_total + ins_long);

  int size                        = strlen (add_commas (buf1, sizeof (buf1), total));
  int parallel_size                = strlen (add_commas (buf1, sizeof (buf1),
                                                      (left_parallel > right_parallel) ? left_parallel : right_parallel));
  int cond_size                        = strlen (add_commas (buf1, sizeof (buf1), (left_cond > right_cond) ? left_cond : right_cond));
  int nop_size                        = strlen (add_commas (buf1, sizeof (buf1), (left_nops > right_nops) ? left_nops : right_nops));
  int normal_size                = strlen (add_commas (buf1, sizeof (buf1), (left > right) ? left : right));

  (*cr16_callback->printf_filtered) (cr16_callback,
                                     "executed %*s left  instruction(s), %*s normal, %*s parallel, %*s EXExxx, %*s nops\n",
                                     size, add_commas (buf1, sizeof (buf1), left_total),
                                     normal_size, add_commas (buf2, sizeof (buf2), left),
                                     parallel_size, add_commas (buf3, sizeof (buf3), left_parallel),
                                     cond_size, add_commas (buf4, sizeof (buf4), left_cond),
                                     nop_size, add_commas (buf5, sizeof (buf5), left_nops));

  (*cr16_callback->printf_filtered) (cr16_callback,
                                     "executed %*s right instruction(s), %*s normal, %*s parallel, %*s EXExxx, %*s nops\n",
                                     size, add_commas (buf1, sizeof (buf1), right_total),
                                     normal_size, add_commas (buf2, sizeof (buf2), right),
                                     parallel_size, add_commas (buf3, sizeof (buf3), right_parallel),
                                     cond_size, add_commas (buf4, sizeof (buf4), right_cond),
                                     nop_size, add_commas (buf5, sizeof (buf5), right_nops));

  if (ins_long)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s long instruction(s)\n",
                                       size, add_commas (buf1, sizeof (buf1), ins_long));

  if (parallel)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s parallel instruction(s)\n",
                                       size, add_commas (buf1, sizeof (buf1), parallel));

  if (leftright)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s instruction(s) encoded L->R\n",
                                       size, add_commas (buf1, sizeof (buf1), leftright));

  if (rightleft)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s instruction(s) encoded R->L\n",
                                       size, add_commas (buf1, sizeof (buf1), rightleft));

  if (unknown)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s unknown instruction(s)\n",
                                       size, add_commas (buf1, sizeof (buf1), unknown));

  if (cond_true)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "executed %*s instruction(s) due to EXExxx condition being true\n",
                                       size, add_commas (buf1, sizeof (buf1), cond_true));

  if (cond_false)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "skipped  %*s instruction(s) due to EXExxx condition being false\n",
                                       size, add_commas (buf1, sizeof (buf1), cond_false));

  if (cond_jump)
    (*cr16_callback->printf_filtered) (cr16_callback,
                                       "skipped  %*s instruction(s) due to conditional branch succeeding\n",
                                       size, add_commas (buf1, sizeof (buf1), cond_jump));

  (*cr16_callback->printf_filtered) (cr16_callback,
                                     "executed %*s cycle(s)\n",
                                     size, add_commas (buf1, sizeof (buf1), cycles));

  (*cr16_callback->printf_filtered) (cr16_callback,
                                     "executed %*s total instructions\n",
                                     size, add_commas (buf1, sizeof (buf1), total));
#endif
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd, char **argv, char **env)
{
  bfd_vma start_address;

  /* reset all state information */
  memset (&State.regs, 0, (int)&State.mem - (int)&State.regs);

  /* There was a hack here to copy the values of argc and argv into r0
     and r1.  The values were also saved into some high memory that
     won't be overwritten by the stack (0x7C00).  The reason for doing
     this was to allow the 'run' program to accept arguments.  Without
     the hack, this is not possible anymore.  If the simulator is run
     from the debugger, arguments cannot be passed in, so this makes
     no difference.  */

  /* set PC */
  if (abfd != NULL)
    start_address = bfd_get_start_address (abfd);
  else
    start_address = 0x0;
#ifdef DEBUG
  if (cr16_debug)
    (*cr16_callback->printf_filtered) (cr16_callback, "sim_create_inferior:  PC=0x%lx\n", (long) start_address);
#endif
  SET_CREG (PC_CR, start_address);

  SLOT_FLUSH ();
  return SIM_RC_OK;
}


void
sim_set_callbacks (p)
     host_callback *p;
{
  cr16_callback = p;
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd;
     enum sim_stop *reason;
     int *sigrc;
{
/*   (*cr16_callback->printf_filtered) (cr16_callback, "sim_stop_reason:  PC=0x%x\n",PC<<2); */

  switch (State.exception)
    {
    case SIG_CR16_STOP:                        /* stop instruction */
      *reason = sim_stopped;
      *sigrc = 0;
      break;

    case SIG_CR16_EXIT:                        /* exit trap */
      *reason = sim_exited;
      *sigrc = GPR (2);
      break;

    case SIG_CR16_BUS:
      *reason = sim_stopped;
      *sigrc = GDB_SIGNAL_BUS;
      break;
//
//    case SIG_CR16_IAD:
//      *reason = sim_stopped;
//      *sigrc = GDB_SIGNAL_IAD;
//      break;

    default:                                /* some signal */
      *reason = sim_stopped;
      if (stop_simulator && !State.exception)
        *sigrc = GDB_SIGNAL_INT;
      else
        *sigrc = State.exception;
      break;
    }

  stop_simulator = 0;
}

int
sim_fetch_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char *memory;
     int length;
{
  int size;
  switch ((enum sim_cr16_regs) rn)
    {
    case SIM_CR16_R0_REGNUM:
    case SIM_CR16_R1_REGNUM:
    case SIM_CR16_R2_REGNUM:
    case SIM_CR16_R3_REGNUM:
    case SIM_CR16_R4_REGNUM:
    case SIM_CR16_R5_REGNUM:
    case SIM_CR16_R6_REGNUM:
    case SIM_CR16_R7_REGNUM:
    case SIM_CR16_R8_REGNUM:
    case SIM_CR16_R9_REGNUM:
    case SIM_CR16_R10_REGNUM:
    case SIM_CR16_R11_REGNUM:
      WRITE_16 (memory, GPR (rn - SIM_CR16_R0_REGNUM));
      size = 2;
      break;
    case SIM_CR16_R12_REGNUM:
    case SIM_CR16_R13_REGNUM:
    case SIM_CR16_R14_REGNUM:
    case SIM_CR16_R15_REGNUM:
      //WRITE_32 (memory, GPR (rn - SIM_CR16_R0_REGNUM));
      write_longword (memory, GPR (rn - SIM_CR16_R0_REGNUM));
      size = 4;
      break;
    case SIM_CR16_PC_REGNUM:
    case SIM_CR16_ISP_REGNUM:
    case SIM_CR16_USP_REGNUM:
    case SIM_CR16_INTBASE_REGNUM:
    case SIM_CR16_PSR_REGNUM:
    case SIM_CR16_CFG_REGNUM:
    case SIM_CR16_DBS_REGNUM:
    case SIM_CR16_DCR_REGNUM:
    case SIM_CR16_DSR_REGNUM:
    case SIM_CR16_CAR0_REGNUM:
    case SIM_CR16_CAR1_REGNUM:
      //WRITE_32 (memory, CREG (rn - SIM_CR16_PC_REGNUM));
      write_longword (memory, CREG (rn - SIM_CR16_PC_REGNUM));
      size = 4;
      break;
    default:
      size = 0;
      break;
    }
  return size;
}
 
int
sim_store_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char *memory;
     int length;
{
  int size;
  switch ((enum sim_cr16_regs) rn)
    {
    case SIM_CR16_R0_REGNUM:
    case SIM_CR16_R1_REGNUM:
    case SIM_CR16_R2_REGNUM:
    case SIM_CR16_R3_REGNUM:
    case SIM_CR16_R4_REGNUM:
    case SIM_CR16_R5_REGNUM:
    case SIM_CR16_R6_REGNUM:
    case SIM_CR16_R7_REGNUM:
    case SIM_CR16_R8_REGNUM:
    case SIM_CR16_R9_REGNUM:
    case SIM_CR16_R10_REGNUM:
    case SIM_CR16_R11_REGNUM:
      SET_GPR (rn - SIM_CR16_R0_REGNUM, READ_16 (memory));
      size = 2;
      break;
    case SIM_CR16_R12_REGNUM:
    case SIM_CR16_R13_REGNUM:
    case SIM_CR16_R14_REGNUM:
    case SIM_CR16_R15_REGNUM:
      SET_GPR32 (rn - SIM_CR16_R0_REGNUM, get_longword (memory));
      size = 4;
      break;
    case SIM_CR16_PC_REGNUM:
    case SIM_CR16_ISP_REGNUM:
    case SIM_CR16_USP_REGNUM:
    case SIM_CR16_INTBASE_REGNUM:
    case SIM_CR16_PSR_REGNUM:
    case SIM_CR16_CFG_REGNUM:
    case SIM_CR16_DBS_REGNUM:
    case SIM_CR16_DCR_REGNUM:
    case SIM_CR16_DSR_REGNUM:
    case SIM_CR16_CAR0_REGNUM:
    case SIM_CR16_CAR1_REGNUM:
      SET_CREG (rn - SIM_CR16_PC_REGNUM, get_longword (memory));
      size = 4;
      break;
    default:
      size = 0;
      break;
    }
  SLOT_FLUSH ();
  return size;
}

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
  return NULL;
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{ 
  (*cr16_callback->printf_filtered) (cr16_callback, "sim_do_command: %s\n",cmd);
}

SIM_RC
sim_load (SIM_DESC sd, char *prog, struct bfd *abfd, int from_tty)
{
  extern bfd *sim_load_file (); /* ??? Don't know where this should live.  */

  if (prog_bfd != NULL && prog_bfd_was_opened_p)
    {
      bfd_close (prog_bfd);
      prog_bfd_was_opened_p = 0;
    }
  prog_bfd = sim_load_file (sd, myname, cr16_callback, prog, abfd,
                            sim_kind == SIM_OPEN_DEBUG,
                            1/*LMA*/, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;
  prog_bfd_was_opened_p = abfd == NULL;
  return SIM_RC_OK;
} 
