/* Signal trampoline unwinder, for GDB the GNU Debugger.

   Copyright (C) 2004-2025 Free Software Foundation, Inc.

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

#include "tramp-frame.h"
#include "extract-store-integer.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "symtab.h"
#include "objfiles.h"
#include "target.h"
#include "trad-frame.h"
#include "frame-base.h"

struct frame_data
{
  const struct tramp_frame *tramp_frame;
};

struct tramp_frame_cache
{
  CORE_ADDR func;
  const struct tramp_frame *tramp_frame;
  struct trad_frame_cache *trad_cache;
};

static struct trad_frame_cache *
tramp_frame_cache (const frame_info_ptr &this_frame,
		   void **this_cache)
{
  struct tramp_frame_cache *tramp_cache
    = (struct tramp_frame_cache *) *this_cache;

  if (tramp_cache->trad_cache == NULL)
    {
      tramp_cache->trad_cache = trad_frame_cache_zalloc (this_frame);
      tramp_cache->tramp_frame->init (tramp_cache->tramp_frame,
				      this_frame,
				      tramp_cache->trad_cache,
				      tramp_cache->func);
    }
  return tramp_cache->trad_cache;
}

class frame_unwind_trampoline : public frame_unwind
{
public:
  frame_unwind_trampoline (enum frame_type type, const struct frame_data *data,
			   frame_prev_arch_ftype *prev_arch_func)
    : frame_unwind ("trampoline", type, FRAME_UNWIND_GDB, data),
      m_prev_arch (prev_arch_func)
  { }

  int sniff (const frame_info_ptr &this_frame,
	     void **this_prologue_cache) const override;

  void this_id (const frame_info_ptr &this_frame, void **this_prologue_cache,
		struct frame_id *id) const override;

  value *prev_register (const frame_info_ptr &this_frame,
			void **this_prologue_cache,
			int regnum) const override;

  struct gdbarch *prev_arch (const frame_info_ptr &this_frame,
			     void **this_prologue_cache) const override
  {
    if (m_prev_arch == nullptr)
      return frame_unwind::prev_arch (this_frame, this_prologue_cache);
    return m_prev_arch (this_frame, this_prologue_cache);
  }

private:
  frame_prev_arch_ftype *m_prev_arch;
};

void
frame_unwind_trampoline::this_id (const frame_info_ptr &this_frame,
				  void **this_cache,
				  struct frame_id *this_id) const
{
  struct trad_frame_cache *trad_cache
    = tramp_frame_cache (this_frame, this_cache);

  trad_frame_get_id (trad_cache, this_id);
}

struct value *
frame_unwind_trampoline::prev_register (const frame_info_ptr &this_frame,
					void **this_cache,
					int prev_regnum) const
{
  struct trad_frame_cache *trad_cache
    = tramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_register (trad_cache, this_frame, prev_regnum);
}

static CORE_ADDR
tramp_frame_start (const struct tramp_frame *tramp,
		   const frame_info_ptr &this_frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int ti;

  /* Check if we can use this trampoline.  */
  if (tramp->validate && !tramp->validate (tramp, this_frame, &pc))
    return 0;

  /* Search through the trampoline for one that matches the
     instruction sequence around PC.  */
  for (ti = 0; tramp->insn[ti].bytes != TRAMP_SENTINEL_INSN; ti++)
    {
      CORE_ADDR func = pc - tramp->insn_size * ti;
      int i;

      for (i = 0; 1; i++)
	{
	  gdb_byte buf[sizeof (tramp->insn[0])];
	  ULONGEST insn;
	  size_t insn_size = tramp->insn_size;

	  if (tramp->insn[i].bytes == TRAMP_SENTINEL_INSN)
	    return func;
	  if (!safe_frame_unwind_memory (this_frame,
					 func + i * insn_size,
					 {buf, insn_size}))
	    break;
	  insn = extract_unsigned_integer (buf, insn_size, byte_order);
	  if (tramp->insn[i].bytes != (insn & tramp->insn[i].mask))
	    break;
	}
    }
  /* Trampoline doesn't match.  */
  return 0;
}

int
frame_unwind_trampoline::sniff (const frame_info_ptr &this_frame,
				void **this_cache) const
{
  const struct tramp_frame *tramp = unwind_data ()->tramp_frame;
  CORE_ADDR pc = get_frame_pc (this_frame);
  CORE_ADDR func;
  struct tramp_frame_cache *tramp_cache;

  /* tausq/2004-12-12: We used to assume if pc has a name or is in a valid 
     section, then this is not a trampoline.  However, this assumption is
     false on HPUX which has a signal trampoline that has a name; it can
     also be false when using an alternative signal stack.  */
  func = tramp_frame_start (tramp, this_frame, pc);
  if (func == 0)
    return 0;
  tramp_cache = FRAME_OBSTACK_ZALLOC (struct tramp_frame_cache);
  tramp_cache->func = func;
  tramp_cache->tramp_frame = tramp;
  (*this_cache) = tramp_cache;
  return 1;
}

void
tramp_frame_prepend_unwinder (struct gdbarch *gdbarch,
			      const struct tramp_frame *tramp_frame)
{
  struct frame_data *data;
  struct frame_unwind *unwinder;
  int i;

  /* Check that the instruction sequence contains a sentinel.  */
  for (i = 0; i < ARRAY_SIZE (tramp_frame->insn); i++)
    {
      if (tramp_frame->insn[i].bytes == TRAMP_SENTINEL_INSN)
	break;
    }
  gdb_assert (i < ARRAY_SIZE (tramp_frame->insn));
  gdb_assert (tramp_frame->insn_size <= sizeof (tramp_frame->insn[0].bytes));

  data = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct frame_data);
  data->tramp_frame = tramp_frame;

  unwinder = obstack_new <frame_unwind_trampoline> (gdbarch_obstack (gdbarch),
						    tramp_frame->frame_type,
						    data,
						    tramp_frame->prev_arch);
  frame_unwind_prepend_unwinder (gdbarch, unwinder);
}
