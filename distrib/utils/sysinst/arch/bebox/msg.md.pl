/*	$NetBSD: msg.md.pl,v 1.5 2003/05/18 18:54:07 dsl Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.11 2001/04/12 03:48:13 briggs Exp */

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

/* MD Message catalog -- Polish, bebox version */

message md_hello
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.

}

message diagcyl
{Niektore stare BIOSy lub programy diagnostyczne moga uzywac ostatniego
cylindra dysku w celach testowych. Nie powinno to byc problemem w nowoczesnych
systemach. Jesli masz stary system i nie jestes pewnien czy moze on tak dzialac,
mozesz wylaczyc ostatni cylinder dysku z uzycia.

Jesli to zrobisz, zarowno geometria rzeczywista jak i BIOS zostanie dopasowana, 
nie musisz jej ponownie wprowadzac.

Czy chcesz wylaczyc ostatni cylinder z uzycia?

}

message wdtype
{Jakim rodzajem dysku jest %s?}

message sectforward
{Czy twoj dysk przesuwa AUTOMATYCZNIE sektory?}

message dlgeom
{Wyglada na to, ze twoj dysk, %s, zostal juz skonfigurowany za pomoca
BSD disklabel i disklabel raportuje, ze geometria jest inna od prawdziwej.
Te dwie geometrie to:

disklabel:		%d cylindrow, %d glowic, %d sektorow 
prawdziwa geometria:	%d cylindrow, %d glowic, %d sektorow 
}

/* the %s's will expand into three character strings */
message dobad144
{Instalowanie tablicy zlych blokow ...
}

message dobootblks
{Instalowanie bootblokow na %s....
}

message askfsroot
{Bede pytal o informacje o partycjach.

Najpierw partycja glowna. Masz %d %s wolnego miejsca na dysku.

Rozmiar partycji glownej? }

message askfsswap
{
Nastepnie partycja wymiany. Masz %d %s wolnego miejsca na dysku.

Rozmiar partycji wymiany? }

message otherparts
{Nadal masz wolna przestrzen na dysku. Podaj rozmiary i punkty montazu
dla ponizszych partycji.

}

message askfspart
{Nastepna partycja jest /dev/%s%c. Masz %d %s wolnego miejsca na dysku.

Rozmiar partycji? }

message cyl1024
{Disklabel (zestaw partycji) ktory skonfigurowales ma glowna partycje, ktora
konczy sie poza 1024 cylindrem BIOS. Aby byc pewnym, ze system bedzie
mogl sie zawsze uruchomic, cala glowna partycja powinna znajdowac sie ponizej
tego ograniczenia. Mozesz ponadto: }

message wmbrfail
{Nadpisanie MBR nie powiodlo sie. Nie moge kontynuowac.}
