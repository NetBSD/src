/*	$NetBSD: msg.md.pl,v 1.2 2014/08/03 16:09:40 martin Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.1 2001/10/15 16:22:52 uch Exp */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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

/* MD Message catalog -- Polish, playstation2 version */

message md_hello
{
}

message md_may_remove_boot_medium
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.
}

message dobad144
{Instalowanie tablicy zlych blokow ...
}

message dobootblks
{Instalowanie bootblokow na %s....
}

message onebiosmatch
{Ten dysk odpowiada ponizszemu dyskowi BIOS:

}

message onebiosmatch_header
{BIOS # cylindry  glowice sektory
------ ---------- ------- -------
}

message onebiosmatch_row
{%-6x %-10d %-7d %d\n}

message biosmultmatch
{Ten dysk odpowiada ponizszym dyskom BIOS:

}

message biosmultmatch_header
{   BIOS # cylindry  glowice sektory
   ------ ---------- ------- -------
}

message biosmultmatch_row
{%-1d: %-6x %-10d %-7d %d\n}

message pickdisk
{Wybierz dysk: }

message partabovechs
{Czesc NetBSD dysku lezy poza obszarem, ktory BIOS w twojej maszynie moze
zaadresowac. Nie mozliwe bedzie bootowanie z tego dysku. Jestes pewien, ze
chcesz to zrobic?

(Odpowiedz 'nie' zabierze cie spowrotem do menu edycji partycji.)}

message set_kernel_1
{Kernel (GENERIC)}

