/* 
 * Copyright (c) 1993 Philip A. Nelson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *	icode.c
 *
 *	$Id: icode.c,v 1.1.1.1 1993/09/09 23:53:48 phil Exp $
 */
/*
 * This is the "user" code for process 1 that execs /sbin/init.
 *
 *  Written in "high level code" for the fun of it!
 */
/* #define DEBUG */

#include <stdarg.h>

/* Make it look like a real C call! */
#define execve(a,b,c)	_do_syscall (59,a,b,c)
#define exit(a)		_do_syscall (1,a)

void i_main();

/* Set up the "module table". */
asm(".globl _icode");
asm("_icode: .long 0,0,0,0");

extern int icode();
extern int myerrno;

int _do_syscall(int sysnum,...);

void i_code()
{
  i_main();
}

/* Exec /sbin/init! */
void i_main()
{
  extern char i_arg1;
  char *argv[] = {"init", 0, 0};

#ifdef DEBUG
  int printf (char *fmt ,...)
    {
      va_list ap;
      va_start(ap, fmt);
      _do_syscall(-1,fmt,ap);
      va_end(ap);
    }

  printf ("ready to exec /sbin/init\n");
#endif

  argv[1] = &i_arg1;

  execve ("/sbin/init", argv, 0);

  printf ("Exec of /sbin/init failed! errno = %d\n", myerrno);
  exit(0);
}

asm(".globl _myerrno");
asm("__do_syscall:");
asm("  movqd	0, r0");
asm("  svc	");
asm("  bcs	not");
asm("  ret	0");
asm("not:  movd r0,_myerrno(pc)");
asm("  movd	-1, r0");
asm("  ret	0");

asm("_myerrno: .long 0");

/* Call it a function to get space and a good address in the text segment. */
i_arg1()
{}

int szicode = 0;

void calcszicode()
{
  szicode = (int)i_arg1 - (int)icode;
}
