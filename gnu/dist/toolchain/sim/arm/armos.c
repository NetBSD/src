/*  armos.c -- ARMulator OS interface:  ARM6 Instruction Emulator.
    Copyright (C) 1994 Advanced RISC Machines Ltd.
 
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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file contains a model of Demon, ARM Ltd's Debug Monitor,
including all the SWI's required to support the C library. The code in
it is not really for the faint-hearted (especially the abort handling
code), but it is a complete example. Defining NOOS will disable all the
fun, and definign VAILDATE will define SWI 1 to enter SVC mode, and SWI
0x11 to halt the emulator. */

#include "config.h"
#include "ansidecl.h"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_RDWR
#define O_RDWR   2
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __STDC__
#define unlink(s) remove(s)
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>		/* For SEEK_SET etc */
#endif

#ifdef __riscos
extern int _fisatty (FILE *);
#define isatty_(f) _fisatty(f)
#else
#ifdef __ZTC__
#include <io.h>
#define isatty_(f) isatty((f)->_file)
#else
#ifdef macintosh
#include <ioctl.h>
#define isatty_(f) (~ioctl ((f)->_file, FIOINTERACTIVE, NULL))
#else
#define isatty_(f) isatty (fileno (f))
#endif
#endif
#endif

#include "armdefs.h"
#include "armos.h"
#ifndef NOOS
#ifndef VALIDATE
/* #ifndef ASIM */
#include "armfpe.h"
/* #endif */
#endif
#endif

/* For RDIError_BreakpointReached.  */
#include "dbg_rdi.h"

extern unsigned ARMul_OSInit (ARMul_State * state);
extern void ARMul_OSExit (ARMul_State * state);
extern unsigned ARMul_OSHandleSWI (ARMul_State * state, ARMword number);
extern unsigned ARMul_OSException (ARMul_State * state, ARMword vector,
				   ARMword pc);
extern ARMword ARMul_OSLastErrorP (ARMul_State * state);
extern ARMword ARMul_Debug (ARMul_State * state, ARMword pc, ARMword instr);

#define BUFFERSIZE 4096
#ifndef FOPEN_MAX
#define FOPEN_MAX 64
#endif
#define UNIQUETEMPS 256

/***************************************************************************\
*                          OS private Information                           *
\***************************************************************************/

struct OSblock
{
  ARMword Time0;
  ARMword ErrorP;
  ARMword ErrorNo;
  FILE *FileTable[FOPEN_MAX];
  char FileFlags[FOPEN_MAX];
  char *tempnames[UNIQUETEMPS];
};

#define NOOP 0
#define BINARY 1
#define READOP 2
#define WRITEOP 4

#ifdef macintosh
#define FIXCRLF(t,c) ((t & BINARY) ? \
                      c : \
                      ((c == '\n' || c == '\r' ) ? (c ^ 7) : c) \
                     )
#else
#define FIXCRLF(t,c) c
#endif

static ARMword softvectorcode[] =
{	/* basic: swi tidyexception + event; mov pc, lr;
	   ldmia r11,{r11,pc}; swi generateexception  + event.  */
  0xef000090, 0xe1a0e00f, 0xe89b8800, 0xef000080,	/*Reset */
  0xef000091, 0xe1a0e00f, 0xe89b8800, 0xef000081,	/*Undef */
  0xef000092, 0xe1a0e00f, 0xe89b8800, 0xef000082,	/*SWI  */
  0xef000093, 0xe1a0e00f, 0xe89b8800, 0xef000083,	/*Prefetch abort */
  0xef000094, 0xe1a0e00f, 0xe89b8800, 0xef000084,	/*Data abort */
  0xef000095, 0xe1a0e00f, 0xe89b8800, 0xef000085,	/*Address exception */
  0xef000096, 0xe1a0e00f, 0xe89b8800, 0xef000086, /*IRQ*/
  0xef000097, 0xe1a0e00f, 0xe89b8800, 0xef000087, /*FIQ*/
  0xef000098, 0xe1a0e00f, 0xe89b8800, 0xef000088,	/*Error */
  0xe1a0f00e			/* default handler */
};

/***************************************************************************\
*            Time for the Operating System to initialise itself.            *
\***************************************************************************/

unsigned
ARMul_OSInit (ARMul_State * state)
{
#ifndef NOOS
#ifndef VALIDATE
  ARMword instr, i, j;
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;

  if (state->OSptr == NULL)
    {
      state->OSptr = (unsigned char *) malloc (sizeof (struct OSblock));
      if (state->OSptr == NULL)
	{
	  perror ("OS Memory");
	  exit (15);
	}
    }
  OSptr = (struct OSblock *) state->OSptr;
  OSptr->ErrorP = 0;
  state->Reg[13] = ADDRSUPERSTACK;	/* set up a stack for the current mode */
  ARMul_SetReg (state, SVC32MODE, 13, ADDRSUPERSTACK);	/* and for supervisor mode */
  ARMul_SetReg (state, ABORT32MODE, 13, ADDRSUPERSTACK);	/* and for abort 32 mode */
  ARMul_SetReg (state, UNDEF32MODE, 13, ADDRSUPERSTACK);	/* and for undef 32 mode */
  instr = 0xe59ff000 | (ADDRSOFTVECTORS - 8);	/* load pc from soft vector */
  for (i = ARMul_ResetV; i <= ARMFIQV; i += 4)
    ARMul_WriteWord (state, i, instr);	/* write hardware vectors */
  for (i = ARMul_ResetV; i <= ARMFIQV + 4; i += 4)
    {
      ARMul_WriteWord (state, ADDRSOFTVECTORS + i, SOFTVECTORCODE + i * 4);
      ARMul_WriteWord (state, ADDRSOFHANDLERS + 2 * i + 4L,
		       SOFTVECTORCODE + sizeof (softvectorcode) - 4L);
    }
  for (i = 0; i < sizeof (softvectorcode); i += 4)
    ARMul_WriteWord (state, SOFTVECTORCODE + i, softvectorcode[i / 4]);
  for (i = 0; i < FOPEN_MAX; i++)
    OSptr->FileTable[i] = NULL;
  for (i = 0; i < UNIQUETEMPS; i++)
    OSptr->tempnames[i] = NULL;
  ARMul_ConsolePrint (state, ", Demon 1.01");

/* #ifndef ASIM */

  /* install fpe */
  for (i = 0; i < fpesize; i += 4)	/* copy the code */
    ARMul_WriteWord (state, FPESTART + i, fpecode[i >> 2]);
  for (i = FPESTART + fpesize;; i -= 4)
    {				/* reverse the error strings */
      if ((j = ARMul_ReadWord (state, i)) == 0xffffffff)
	break;
      if (state->bigendSig && j < 0x80000000)
	{			/* it's part of the string so swap it */
	  j = ((j >> 0x18) & 0x000000ff) |
	    ((j >> 0x08) & 0x0000ff00) |
	    ((j << 0x08) & 0x00ff0000) | ((j << 0x18) & 0xff000000);
	  ARMul_WriteWord (state, i, j);
	}
    }
  ARMul_WriteWord (state, FPEOLDVECT, ARMul_ReadWord (state, 4));	/* copy old illegal instr vector */
  ARMul_WriteWord (state, 4, FPENEWVECT (ARMul_ReadWord (state, i - 4)));	/* install new vector */
  ARMul_ConsolePrint (state, ", FPE");

/* #endif  ASIM */
#endif /* VALIDATE */
#endif /* NOOS */

  return (TRUE);
}

void
ARMul_OSExit (ARMul_State * state)
{
  free ((char *) state->OSptr);
}


/***************************************************************************\
*                  Return the last Operating System Error.                  *
\***************************************************************************/

ARMword ARMul_OSLastErrorP (ARMul_State * state)
{
  return ((struct OSblock *) state->OSptr)->ErrorP;
}

static int translate_open_mode[] = {
  O_RDONLY,			/* "r"   */
  O_RDONLY + O_BINARY,		/* "rb"  */
  O_RDWR,			/* "r+"  */
  O_RDWR + O_BINARY,		/* "r+b" */
  O_WRONLY + O_CREAT + O_TRUNC,	/* "w"   */
  O_WRONLY + O_BINARY + O_CREAT + O_TRUNC,	/* "wb"  */
  O_RDWR + O_CREAT + O_TRUNC,	/* "w+"  */
  O_RDWR + O_BINARY + O_CREAT + O_TRUNC,	/* "w+b" */
  O_WRONLY + O_APPEND + O_CREAT,	/* "a"   */
  O_WRONLY + O_BINARY + O_APPEND + O_CREAT,	/* "ab"  */
  O_RDWR + O_APPEND + O_CREAT,	/* "a+"  */
  O_RDWR + O_BINARY + O_APPEND + O_CREAT	/* "a+b" */
};

static void
SWIWrite0 (ARMul_State * state, ARMword addr)
{
  ARMword temp;
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;

  while ((temp = ARMul_ReadByte (state, addr++)) != 0)
    (void) fputc ((char) temp, stdout);

  OSptr->ErrorNo = errno;
}

static void
WriteCommandLineTo (ARMul_State * state, ARMword addr)
{
  ARMword temp;
  char *cptr = state->CommandLine;
  if (cptr == NULL)
    cptr = "\0";
  do
    {
      temp = (ARMword) * cptr++;
      ARMul_WriteByte (state, addr++, temp);
    }
  while (temp != 0);
}

static void
SWIopen (ARMul_State * state, ARMword name, ARMword SWIflags)
{
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;
  char dummy[2000];
  int flags;
  int i;

  for (i = 0; (dummy[i] = ARMul_ReadByte (state, name + i)); i++)
    ;

  /* Now we need to decode the Demon open mode */
  flags = translate_open_mode[SWIflags];

  /* Filename ":tt" is special: it denotes stdin/out */
  if (strcmp (dummy, ":tt") == 0)
    {
      if (flags == O_RDONLY)	/* opening tty "r" */
	state->Reg[0] = 0;	/* stdin */
      else
	state->Reg[0] = 1;	/* stdout */
    }
  else
    {
      state->Reg[0] = (int) open (dummy, flags, 0666);
      OSptr->ErrorNo = errno;
    }
}

static void
SWIread (ARMul_State * state, ARMword f, ARMword ptr, ARMword len)
{
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;
  int res;
  int i;
  char *local = malloc (len);

  if (local == NULL)
    {
      fprintf (stderr, "sim: Unable to read 0x%ulx bytes - out of memory\n",
	       len);
      return;
    }

  res = read (f, local, len);
  if (res > 0)
    for (i = 0; i < res; i++)
      ARMul_WriteByte (state, ptr + i, local[i]);
  free (local);
  state->Reg[0] = res == -1 ? -1 : len - res;
  OSptr->ErrorNo = errno;
}

static void
SWIwrite (ARMul_State * state, ARMword f, ARMword ptr, ARMword len)
{
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;
  int res;
  ARMword i;
  char *local = malloc (len);

  if (local == NULL)
    {
      fprintf (stderr, "sim: Unable to write 0x%lx bytes - out of memory\n",
	       (long) len);
      return;
    }

  for (i = 0; i < len; i++)
    local[i] = ARMul_ReadByte (state, ptr + i);

  res = write (f, local, len);
  state->Reg[0] = res == -1 ? -1 : len - res;
  free (local);
  OSptr->ErrorNo = errno;
}

static void
SWIflen (ARMul_State * state, ARMword fh)
{
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;
  ARMword addr;

  if (fh == 0 || fh > FOPEN_MAX)
    {
      OSptr->ErrorNo = EBADF;
      state->Reg[0] = -1L;
      return;
    }

  addr = lseek (fh, 0, SEEK_CUR);

  state->Reg[0] = lseek (fh, 0L, SEEK_END);
  (void) lseek (fh, addr, SEEK_SET);

  OSptr->ErrorNo = errno;
}

/***************************************************************************\
* The emulator calls this routine when a SWI instruction is encuntered. The *
* parameter passed is the SWI number (lower 24 bits of the instruction).    *
\***************************************************************************/

unsigned
ARMul_OSHandleSWI (ARMul_State * state, ARMword number)
{
  ARMword addr, temp;
  struct OSblock *OSptr = (struct OSblock *) state->OSptr;

  switch (number)
    {
    case SWI_Read:
      SWIread (state, state->Reg[0], state->Reg[1], state->Reg[2]);
      return TRUE;

    case SWI_Write:
      SWIwrite (state, state->Reg[0], state->Reg[1], state->Reg[2]);
      return TRUE;

    case SWI_Open:
      SWIopen (state, state->Reg[0], state->Reg[1]);
      return TRUE;

    case SWI_Clock:
      /* return number of centi-seconds... */
      state->Reg[0] =
#ifdef CLOCKS_PER_SEC
	(CLOCKS_PER_SEC >= 100)
	? (ARMword) (clock () / (CLOCKS_PER_SEC / 100))
	: (ARMword) ((clock () * 100) / CLOCKS_PER_SEC);
#else
	/* presume unix... clock() returns microseconds */
	(ARMword) (clock () / 10000);
#endif
      OSptr->ErrorNo = errno;
      return (TRUE);

    case SWI_Time:
      state->Reg[0] = (ARMword) time (NULL);
      OSptr->ErrorNo = errno;
      return (TRUE);

    case SWI_Close:
      state->Reg[0] = close (state->Reg[0]);
      OSptr->ErrorNo = errno;
      return TRUE;

    case SWI_Flen:
      SWIflen (state, state->Reg[0]);
      return (TRUE);

    case SWI_Exit:
      state->Emulate = FALSE;
      return TRUE;

    case SWI_Seek:
      {
	/* We must return non-zero for failure */
	state->Reg[0] = -1 >= lseek (state->Reg[0], state->Reg[1], SEEK_SET);
	OSptr->ErrorNo = errno;
	return TRUE;
      }

    case SWI_WriteC:
      (void) fputc ((int) state->Reg[0], stdout);
      OSptr->ErrorNo = errno;
      return (TRUE);

    case SWI_Write0:
      SWIWrite0 (state, state->Reg[0]);
      return (TRUE);

    case SWI_GetErrno:
      state->Reg[0] = OSptr->ErrorNo;
      return (TRUE);

    case SWI_Breakpoint:
      state->EndCondition = RDIError_BreakpointReached;
      state->Emulate = FALSE;
      return (TRUE);

    case SWI_GetEnv:
      state->Reg[0] = ADDRCMDLINE;
      if (state->MemSize)
	state->Reg[1] = state->MemSize;
      else
	state->Reg[1] = ADDRUSERSTACK;

      WriteCommandLineTo (state, state->Reg[0]);
      return (TRUE);

      /* Handle Angel SWIs as well as Demon ones */
    case AngelSWI_ARM:
    case AngelSWI_Thumb:
      /* R1 is almost always a parameter block */
      addr = state->Reg[1];
      /* R0 is a reason code */
      switch (state->Reg[0])
	{
	  /* Unimplemented reason codes */
	case AngelSWI_Reason_ReadC:
	case AngelSWI_Reason_IsTTY:
	case AngelSWI_Reason_TmpNam:
	case AngelSWI_Reason_Remove:
	case AngelSWI_Reason_Rename:
	case AngelSWI_Reason_System:
	case AngelSWI_Reason_EnterSVC:
	default:
	  state->Emulate = FALSE;
	  return (FALSE);

	case AngelSWI_Reason_Clock:
	  /* return number of centi-seconds... */
	  state->Reg[0] =
#ifdef CLOCKS_PER_SEC
	    (CLOCKS_PER_SEC >= 100)
	    ? (ARMword) (clock () / (CLOCKS_PER_SEC / 100))
	    : (ARMword) ((clock () * 100) / CLOCKS_PER_SEC);
#else
	    /* presume unix... clock() returns microseconds */
	    (ARMword) (clock () / 10000);
#endif
	  OSptr->ErrorNo = errno;
	  return (TRUE);

	case AngelSWI_Reason_Time:
	  state->Reg[0] = (ARMword) time (NULL);
	  OSptr->ErrorNo = errno;
	  return (TRUE);

	case AngelSWI_Reason_WriteC:
	  (void) fputc ((int) ARMul_ReadByte (state, addr), stdout);
	  OSptr->ErrorNo = errno;
	  return (TRUE);

	case AngelSWI_Reason_Write0:
	  SWIWrite0 (state, addr);
	  return (TRUE);

	case AngelSWI_Reason_Close:
	  state->Reg[0] = close (ARMul_ReadWord (state, addr));
	  OSptr->ErrorNo = errno;
	  return (TRUE);

	case AngelSWI_Reason_Seek:
	  state->Reg[0] = -1 >= lseek (ARMul_ReadWord (state, addr),
				       ARMul_ReadWord (state, addr + 4),
				       SEEK_SET);
	  OSptr->ErrorNo = errno;
	  return (TRUE);

	case AngelSWI_Reason_FLen:
	  SWIflen (state, ARMul_ReadWord (state, addr));
	  return (TRUE);

	case AngelSWI_Reason_GetCmdLine:
	  WriteCommandLineTo (state, ARMul_ReadWord (state, addr));
	  return (TRUE);

	case AngelSWI_Reason_HeapInfo:
	  /* R1 is a pointer to a pointer */
	  addr = ARMul_ReadWord (state, addr);

	  /* Pick up the right memory limit */
	  if (state->MemSize)
	    temp = state->MemSize;
	  else
	    temp = ADDRUSERSTACK;

	  ARMul_WriteWord (state, addr, 0);	/* Heap base */
	  ARMul_WriteWord (state, addr + 4, temp);	/* Heap limit */
	  ARMul_WriteWord (state, addr + 8, temp);	/* Stack base */
	  ARMul_WriteWord (state, addr + 12, temp);	/* Stack limit */
	  return (TRUE);

	case AngelSWI_Reason_ReportException:
	  if (state->Reg[1] == ADP_Stopped_ApplicationExit)
	    state->Reg[0] = 0;
	  else
	    state->Reg[0] = -1;
	  state->Emulate = FALSE;
	  return TRUE;

	case ADP_Stopped_ApplicationExit:
	  state->Reg[0] = 0;
	  state->Emulate = FALSE;
	  return (TRUE);

	case ADP_Stopped_RunTimeError:
	  state->Reg[0] = -1;
	  state->Emulate = FALSE;
	  return (TRUE);

	case AngelSWI_Reason_Errno:
	  state->Reg[0] = OSptr->ErrorNo;
	  return (TRUE);

	case AngelSWI_Reason_Open:
	  SWIopen (state,
		   ARMul_ReadWord (state, addr),
		   ARMul_ReadWord (state, addr + 4));
	  return TRUE;

	case AngelSWI_Reason_Read:
	  SWIread (state,
		   ARMul_ReadWord (state, addr),
		   ARMul_ReadWord (state, addr + 4),
		   ARMul_ReadWord (state, addr + 8));
	  return TRUE;

	case AngelSWI_Reason_Write:
	  SWIwrite (state,
		    ARMul_ReadWord (state, addr),
		    ARMul_ReadWord (state, addr + 4),
		    ARMul_ReadWord (state, addr + 8));
	  return TRUE;
	}

    default:
      state->Emulate = FALSE;
      return (FALSE);
    }
}

#ifndef NOOS
#ifndef ASIM

/***************************************************************************\
* The emulator calls this routine when an Exception occurs.  The second     *
* parameter is the address of the relevant exception vector.  Returning     *
* FALSE from this routine causes the trap to be taken, TRUE causes it to    *
* be ignored (so set state->Emulate to FALSE!).                             *
\***************************************************************************/

unsigned
ARMul_OSException (ARMul_State * state ATTRIBUTE_UNUSED, ARMword vector ATTRIBUTE_UNUSED, ARMword pc ATTRIBUTE_UNUSED)
{				/* don't use this here */
  return (FALSE);
}

#endif


#endif /* NOOS */
