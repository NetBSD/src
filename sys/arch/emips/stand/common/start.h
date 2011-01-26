/*	$NetBSD: start.h,v 1.1 2011/01/26 01:18:54 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

extern int PutWord(uint32_t Word);
extern int Puts(char *String);
extern int GetChar(void);
extern  int PutChar(uint8_t v);
extern void Delay(uint32_t count);
extern uint32_t GetPsr(void);
extern void SetPsr(uint32_t Psr);
extern uint32_t GetCause(void);
extern uint32_t GetEpc(void);
extern volatile void Stop(void);
extern void switch_stack_and_call(void *arg, void (*function)(void *));

typedef struct  _CXTINFO {
    /* Structure must match with mips_asm.h etc. */
    uint32_t gpr[32];
    uint32_t pc;
    uint32_t sr;           /* c0_status */
    uint32_t hi;
    uint32_t lo;
} CXTINFO, *PCXTINFO;

typedef PCXTINFO (*USER_EXCEPTION_HANDLER)(PCXTINFO pContext);
extern char ExceptionHandler[];
extern char ExceptionHandlerEnd[];
extern USER_EXCEPTION_HANDLER *UserInterruptHandler;

extern char start[];
extern char _end[]; /* ..and beginning of free memory */
