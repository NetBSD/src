/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: macros.h,v 1.1 1994/08/02 20:20:36 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		

/* Here general macros are supposed to be stored */

#define PRINT_HEXF(x)  (((x)<0x10)       ? "0000000" : \
			((x)<0x100)      ? "000000"  : \
			((x)<0x1000)     ? "00000"   : \
			((x)<0x10000)    ? "0000"    : \
			((x)<0x100000)   ? "000"     : \
			((x)<0x1000000)  ? "00"      : \
			((x)<0x10000000) ? "0"       : \
			                   ""        )

#define PRINT_BOOL(x) ((x)?"TRUE":"FALSE")

#define PRINT_PROT(x) (((x)==0) ? "NONE"           : \
		       ((x)==1) ? "READ"           : \
		       ((x)==2) ? "WRITE"          : \
		       ((x)==3) ? "READ/WRITE"     : \
		       ((x)==4) ? "EXECUTE"        : \
		       ((x)==5) ? "EXECUTE/READ"   : \
		       ((x)==6) ? "EXECUTE/WRITE"  : \
 		                  "EXECUTE/READ/WRITE")
