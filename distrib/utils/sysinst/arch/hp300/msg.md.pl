/*	$NetBSD: msg.md.pl,v 1.5 2003/07/07 12:30:24 dsl Exp $	*/
/* Based on english version: */
/*	NetBSD: msg.md.en,v 1.2 2002/03/15 05:26:37 gmcgarry Exp */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* hp300 machine dependent messages, Polish */


message md_hello
{Jesli masz mniej niz 4 MB ram, sysinst nie bedzie pracowal prawidlowo

}

message dobootblks
{Instalowanie bootblokow na %s....
}

message newdisk
{Wyglada na to, ze twoj dysk %s, nie ma znaku X68K. sysinst wpisuje taki
znak.
Pamietaj, ze jesli chcesz uzywac czesci dysku %s z Human68k, powinienes
tu przerwac i zformatowac dysk za pomoca format.x z Human68k.
}

message ordering
{Uklad partycji %c jest bledny. Edytowac jeszcze raz?}

message emptypart
{Istnieje poprawna partycja %c po pustych partycjach.
Wyedytuj jeszcze raz tablice partycji.}

message set_kernel_1
{Kernel (GENERIC)}
