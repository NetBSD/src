/*	$NetBSD: msg.md.pl,v 1.2 2002/04/09 19:48:22 hubertf Exp $	*/
/* Based on english version: */
/*	NetBSD: msg.md.en,v 1.2 2002/12/03 01:54:57 minoura Exp */

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

/* x68k machine dependent messages, Polish */


message md_hello
{Jesli dyskietka nie jest zablokowana, mozesz ja teraz wyciagnac.

}

message dobootblks
{Instalowanie bootblokow na %s....
}

message askfsroot
{Bede pytal o rozmiary partcyji, a na niektorych o punkty montazu.

Najpierw partcyja glowna. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji glownej? }

message askfsswap
{
Nastepnie partycja wymiany. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji wymiany? }

message askfsusr
{
Nastepnie partycja /usr.  Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji /usr? }

message otherparts
{Nadal masz wolna przestrzen na dysku. Podaj rozmiary i punkty montazu
dla ponizszych partycji.

}

message askfspart
{Nastepna partycja jest /dev/%s%c.  Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji? }

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

/* XXX: not yet implemented
message existing
{Do you want to preserve existing BSD partition(s)?}

message nofreepart
{%s does not have enough free partitions for NetBSD.
It must have at least 2 free partitions (for root file system and swap).
}

message notfirst
{NetBSD/x68k must be installed in the first part of the boot disk.
The first part of %s is not free.
}
*/
