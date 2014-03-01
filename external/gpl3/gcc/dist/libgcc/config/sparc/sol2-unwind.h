/* DWARF2 EH unwinding support for SPARC Solaris.
   Copyright (C) 2009-2013 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

/* Do code reading to identify a signal frame, and set the frame
   state data appropriately.  See unwind-dw2.c for the structs.  */

#include <ucontext.h>
#include <sys/frame.h>
#include <sys/stack.h>

#ifdef __arch64__

#define IS_SIGHANDLER sparc64_is_sighandler

static int
sparc64_is_sighandler (unsigned int *pc, void *cfa, int *nframes)
{
  if (/* Solaris 9 - single-threaded
	----------------------------
	The pattern changes slightly in different versions of the
	operating system, so we skip the comparison against pc[-6] for
	Solaris 9.

	<sigacthandler+24>:  sra  %i0, 0, %l1

	Solaris 9 5/02:
	<sigacthandler+28>:  ldx  [ %o2 + 0xf68 ], %g5
	Solaris 9 9/05:
	<sigacthandler+28>:  ldx  [ %o2 + 0xe50 ], %g5

	<sigacthandler+32>:  sllx  %l1, 3, %g4
	<sigacthandler+36>:  mov  %l1, %o0
	<sigacthandler+40>:  ldx  [ %g4 + %g5 ], %l0
	<sigacthandler+44>:  call  %l0
	<sigacthandler+48>:  mov  %i2, %o2
	<sigacthandler+52>:  cmp  %l1, 8	<--- PC  */
      (   pc[-7] == 0xa33e2000
       /* skip pc[-6] */
       && pc[-5] == 0x892c7003
       && pc[-4] == 0x90100011
       && pc[-3] == 0xe0590005
       && pc[-2] == 0x9fc40000
       && pc[-1] == 0x9410001a
       && pc[ 0] == 0x80a46008))
    {
      /* We need to move up one frame:

		<signal handler>	<-- context->cfa
		sigacthandler
		<kernel>
      */
      *nframes = 1;
      return 1;
    }

  if (/* Solaris 8+ - multi-threaded
	----------------------------
	<__sighndlr>:        save  %sp, -176, %sp
	<__sighndlr+4>:      mov  %i0, %o0
	<__sighndlr+8>:      mov  %i1, %o1
	<__sighndlr+12>:     call  %i3
	<__sighndlr+16>:     mov  %i2, %o2
	<__sighndlr+20>:     ret 		<--- PC
	<__sighndlr+24>:     restore  */
         pc[-5] == 0x9de3bf50
      && pc[-4] == 0x90100018
      && pc[-3] == 0x92100019
      && pc[-2] == 0x9fc6c000
      && pc[-1] == 0x9410001a
      && pc[ 0] == 0x81c7e008
      && pc[ 1] == 0x81e80000)
    {
      /* We have observed different calling frames among different
	 versions of the operating system, so that we need to
	 discriminate using the upper frame.  We look for the return
	 address of the caller frame (there is an offset of 15 double
	 words between the frame address and the place where this return
	 address is stored) in order to do some more pattern matching.  */
      unsigned int cuh_pattern
	= *(unsigned int *)(*(unsigned long *)(cfa + 15*8) - 4);

      if (cuh_pattern == 0x92100019)
	/* This matches the call_user_handler pattern for Solaris 11.
	   This is the same setup as for Solaris 9, see below.  */
	*nframes = 3;

      else if (cuh_pattern == 0xd25fa7ef)
	{
	  /* This matches the call_user_handler pattern for Solaris 10.
	     There are 2 cases so we look for the return address of the
	     caller's caller frame in order to do more pattern matching.  */
	  unsigned long sah_address = *(unsigned long *)(cfa + 176 + 15*8);

          if (sah_address && *(unsigned int *)(sah_address - 4) == 0x92100019)
	    /* This is the same setup as for Solaris 9, see below.  */
	    *nframes = 3;
	  else
	    /* The sigacthandler frame isn't present in the chain.
	       We need to move up two frames:

		<signal handler>	<-- context->cfa
		__sighndlr
		call_user_handler frame
		<kernel>
	    */
	    *nframes = 2;
	}

      else if (cuh_pattern == 0x9410001a || cuh_pattern == 0x94100013)
	/* This matches the call_user_handler pattern for Solaris 9.
	   We need to move up three frames:

		<signal handler>	<-- context->cfa
		__sighndlr
		call_user_handler
		sigacthandler
		<kernel>
	*/
	*nframes = 3;

      return 1;
    }

  return 0;
}

#define MD_FALLBACK_FRAME_STATE_FOR sparc64_fallback_frame_state

#define MD_FROB_UPDATE_CONTEXT sparc64_frob_update_context

static void
sparc64_frob_update_context (struct _Unwind_Context *context,
			     _Unwind_FrameState *fs)
{
  /* The column of %sp contains the old CFA, not the old value of %sp.
     The CFA offset already comprises the stack bias so, when %sp is the
     CFA register, we must avoid counting the stack bias twice.  Do not
     do that for signal frames as the offset is artificial for them.  */
  if (fs->regs.cfa_reg == __builtin_dwarf_sp_column ()
      && fs->regs.cfa_how == CFA_REG_OFFSET
      && fs->regs.cfa_offset != 0
      && !fs->signal_frame)
    {
      long i;

      context->cfa -= STACK_BIAS;

      for (i = 0; i < DWARF_FRAME_REGISTERS + 1; ++i)
	if (fs->regs.reg[i].how == REG_SAVED_OFFSET)
	  _Unwind_SetGRPtr (context, i,
			    _Unwind_GetGRPtr (context, i) - STACK_BIAS);
    }
}

#else

#define IS_SIGHANDLER sparc_is_sighandler

static int
sparc_is_sighandler (unsigned int *pc, void *cfa, int *nframes)
{
  if (/* Solaris 9 - single-threaded
        ----------------------------
	The pattern changes slightly in different versions of the operating
	system, so we skip the comparison against pc[-6].

	<sigacthandler+16>:  add  %o1, %o7, %o3
	<sigacthandler+20>:  mov  %i1, %o1

	<sigacthandler+24>:  ld  [ %o3 + <offset> ], %o2

	<sigacthandler+28>:  sll  %i0, 2, %o0
	<sigacthandler+32>:  ld  [ %o0 + %o2 ], %l0
	<sigacthandler+36>:  mov  %i0, %o0
	<sigacthandler+40>:  call  %l0
	<sigacthandler+44>:  mov  %i2, %o2
	<sigacthandler+48>:  cmp  %i0, 8	<--- PC  */
         pc[-8] == 0x9602400f
      && pc[-7] == 0x92100019
      /* skip pc[-6] */
      && pc[-5] == 0x912e2002
      && pc[-4] == 0xe002000a
      && pc[-3] == 0x90100018
      && pc[-2] == 0x9fc40000
      && pc[-1] == 0x9410001a
      && pc[ 0] == 0x80a62008)
    {
      /* We need to move up one frame:

		<signal handler>	<-- context->cfa
		sigacthandler
		<kernel>
      */
      *nframes = 1;
      return 1;
    }

  if(/* Solaris 8+ - multi-threaded
       ----------------------------
       <__sighndlr>:	save  %sp, -96, %sp
       <__sighndlr+4>:	mov  %i0, %o0
       <__sighndlr+8>:	mov  %i1, %o1
       <__sighndlr+12>:	call  %i3
       <__sighndlr+16>:	mov  %i2, %o2
       <__sighndlr+20>:	ret 		<--- PC
       <__sighndlr+24>:	restore  */
        pc[-5] == 0x9de3bfa0
     && pc[-4] == 0x90100018
     && pc[-3] == 0x92100019
     && pc[-2] == 0x9fc6c000
     && pc[-1] == 0x9410001a
     && pc[ 0] == 0x81c7e008
     && pc[ 1] == 0x81e80000)
    {
      /* We have observed different calling frames among different
	 versions of the operating system, so that we need to
	 discriminate using the upper frame.  We look for the return
	 address of the caller frame (there is an offset of 15 words
	 between the frame address and the place where this return
	 address is stored) in order to do some more pattern matching.  */
      unsigned int cuh_pattern
	= *(unsigned int *)(*(unsigned int *)(cfa + 15*4) - 4);

      if (cuh_pattern == 0x92100019)
	/* This matches the call_user_handler pattern for Solaris 11.
	   This is the same setup as for Solaris 9, see below.  */
	*nframes = 3;

      else if (cuh_pattern == 0xd407a04c)
	{
	  /* This matches the call_user_handler pattern for Solaris 10.
	     There are 2 cases so we look for the return address of the
	     caller's caller frame in order to do more pattern matching.  */
	  unsigned int sah_address = *(unsigned int *)(cfa + 96 + 15*4);

          if (sah_address && *(unsigned int *)(sah_address - 4) == 0x92100019)
	    /* This is the same setup as for Solaris 9, see below.  */
	    *nframes = 3;
	  else
	    /* The sigacthandler frame isn't present in the chain.
	       We need to move up two frames:

		<signal handler>	<-- context->cfa
		__sighndlr
		call_user_handler frame
		<kernel>
	    */
	    *nframes = 2;
	}

      else if (cuh_pattern == 0x9410001a || cuh_pattern == 0x9410001b)
	/* This matches the call_user_handler pattern for Solaris 9.
	   We need to move up three frames:

		<signal handler>	<-- context->cfa
		__sighndlr
		call_user_handler
		sigacthandler
		<kernel>
	*/
	*nframes = 3;

      return 1;
    }

  return 0;
}

#define MD_FALLBACK_FRAME_STATE_FOR sparc_fallback_frame_state

#endif

static _Unwind_Reason_Code
MD_FALLBACK_FRAME_STATE_FOR (struct _Unwind_Context *context,
			     _Unwind_FrameState *fs)
{
  void *pc = context->ra;
  struct frame *fp = (struct frame *) context->cfa;
  int nframes;
  void *this_cfa = context->cfa;
  long new_cfa;
  void *ra_location, *shifted_ra_location;
  mcontext_t *mctx;
  int i;

  /* Deal with frame-less function from which a signal was raised.  */
  if (_Unwind_IsSignalFrame (context))
    {
      /* The CFA is by definition unmodified in this case.  */
      fs->regs.cfa_how = CFA_REG_OFFSET;
      fs->regs.cfa_reg = __builtin_dwarf_sp_column ();
      fs->regs.cfa_offset = 0;

      /* This is the canonical RA column.  */
      fs->retaddr_column = 15;

      return _URC_NO_REASON;
    }

  if (IS_SIGHANDLER (pc, this_cfa, &nframes))
    {
      struct handler_args {
	struct frame frwin;
	ucontext_t ucontext;
      } *handler_args;
      ucontext_t *ucp;

      /* context->cfa points into the frame after the saved frame pointer and
         saved pc (struct frame).

         The ucontext_t structure is in the kernel frame after a struct
         frame.  Since the frame sizes vary even within OS releases, we
         need to walk the stack to get there.  */

      for (i = 0; i < nframes; i++)
	fp = (struct frame *) ((char *)fp->fr_savfp + STACK_BIAS);

      handler_args = (struct handler_args *) fp;
      ucp = &handler_args->ucontext;
      mctx = &ucp->uc_mcontext;
    }

  /* Exit if the pattern at the return address does not match the
     previous three patterns.  */
  else
    return _URC_END_OF_STACK;

  new_cfa = mctx->gregs[REG_SP];
  /* The frame address is %sp + STACK_BIAS in 64-bit mode.  */
  new_cfa += STACK_BIAS;

  fs->regs.cfa_how = CFA_REG_OFFSET;
  fs->regs.cfa_reg = __builtin_dwarf_sp_column ();
  fs->regs.cfa_offset = new_cfa - (long) this_cfa;

  /* Restore global and out registers (in this order) from the
     ucontext_t structure, uc_mcontext.gregs field.  */
  for (i = 1; i < 16; i++)
    {
      /* We never restore %sp as everything is purely CFA-based.  */
      if ((unsigned int) i == __builtin_dwarf_sp_column ())
	continue;

      /* First the global registers and then the out registers.  */
      fs->regs.reg[i].how = REG_SAVED_OFFSET;
      fs->regs.reg[i].loc.offset = (long)&mctx->gregs[REG_Y + i] - new_cfa;
    }

  /* Just above the stack pointer there are 16 extended words in which
     the register window (in and local registers) was saved.  */
  for (i = 0; i < 16; i++)
    {
      fs->regs.reg[i + 16].how = REG_SAVED_OFFSET;
      fs->regs.reg[i + 16].loc.offset = i*sizeof(long);
    }

  /* Check whether we need to restore FPU registers.  */
  if (mctx->fpregs.fpu_qcnt)
    {
      for (i = 0; i < 32; i++)
	{
	  fs->regs.reg[i + 32].how = REG_SAVED_OFFSET;
	  fs->regs.reg[i + 32].loc.offset
	    = (long)&mctx->fpregs.fpu_fr.fpu_regs[i] - new_cfa;
	}

#ifdef __arch64__
      /* For 64-bit, fpu_fr.fpu_dregs contains 32 instead of 16 doubles.  */
      for (i = 32; i < 64; i++)
	{
	  if (i > 32 && (i & 1))
	    continue;

	  fs->regs.reg[i + 32].how = REG_SAVED_OFFSET;
	  fs->regs.reg[i + 32].loc.offset
	    = (long)&mctx->fpregs.fpu_fr.fpu_dregs[i/2] - new_cfa;
	}
#endif
    }

  /* State the rules to find the kernel's code "return address", which is
     the address of the active instruction when the signal was caught.
     On the SPARC, since RETURN_ADDR_OFFSET (essentially 8) is defined, we
     need to preventively subtract it from the purported return address.  */
  ra_location = &mctx->gregs[REG_PC];
  shifted_ra_location = &mctx->gregs[REG_Y];
  *(void **)shifted_ra_location = *(void **)ra_location - 8;
  fs->retaddr_column = 0;
  fs->regs.reg[0].how = REG_SAVED_OFFSET;
  fs->regs.reg[0].loc.offset = (long)shifted_ra_location - new_cfa;
  fs->signal_frame = 1;

  return _URC_NO_REASON;
}
