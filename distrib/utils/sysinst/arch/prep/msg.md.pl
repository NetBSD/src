/*	$NetBSD: msg.md.pl,v 1.5 2003/05/18 18:54:08 dsl Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.1 2001/01/16 16:54:01 nonaka Exp */

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

/* MD Message catalog -- Polish, prep version */

message md_hello
{
}

message wdtype
{Jakim rodzajem dysku jest %s?}

message sectforward
{Czy twoj dysk przesuwa AUTOMATYCZNIE sektory?}

/* the %s's will expand into three character strings */
message dobad144
{Instalowanie tablicy zlych blokow ...
}

message dobootblks
{Instalowanie bootblokow na %s....
}

message askfsroot1
{Bede pytal o informacje o partycjach.

Najpierw partycja glowna. Masz %d %s wolnego miejsca na dysku.
}

message askfsroot2
{Rozmiar partycji glownej? }

message askfsswap1
{
Nastepnie partycja wymiany. Masz %d %s wolnego miejsca na dysku.
}

message askfsswap2
{Rozmiar partycji wymiany? }

message otherparts
{Nadal masz wolna przestrzen na dysku. Podaj rozmiary i punkty montazu
dla ponizszych partycji.

}

message askfspart1
{Nastepna partycja jest /dev/%s%c. Masz %d %s wolnego miejsca na dysku.
}

message askfspart2
{Rozmiar partycji? }

message cyl1024
{Disklabel (zestaw partycji) ktory skonfigurowales ma glowna partycje, ktora
konczy sie poza 1024 cylindrem BIOS. Aby byc pewnym, ze system bedzie
mogl sie zawsze uruchomic, cala glowna partycja powinna znajdowac sie ponizej
tego ograniczenia. Mozesz ponadto: }

message pickdisk
{Wybierz dysk: }

message wmbrfail
{Nadpisanie MBR nie powiodlo sie. Nie moge kontynuowac.}

message partabovechs
{Czesc dysku NetBSD lezy poza obszarem, ktory BIOS w twojej maszynie moze
zaadresowac. Nie mozliwe bedzie bootowanie z tego dysku. Jestes pewnien, ze
chcesz to zrobic?

(Odpowiedz 'nie' zabierze cie spowrotem do menu edycji partycji.)}
