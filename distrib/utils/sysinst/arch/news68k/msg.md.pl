/*	$NetBSD: msg.md.pl,v 1.3 2003/06/03 11:54:53 dsl Exp $	*/
/* Based on english version: */
/*	NetBSD: msg.md.en,v 1.4 2002/03/16 17:25:56 tsutsui Exp */

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

/* news68k machine dependent messages, Polish */


message md_hello
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.

}

message fullpart
{Zainstalujemy teraz NetBSD na dysku %s. Mozesz wybrac czy chcesz
zainstalowac NetBSD na calym dysku, czy tylko na jego czesci.
Ktora instalacje chcesz zrobic?
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

message emulbackup 
{Albo /emul/aout albo /emul w twoim systemie byl symbolicznym linkiem
wskazujacym na niezamontowany system. Zostalo mu dodane rozszerzenie '.old'.
Kiedy juz uruchomisz swoj zaktualizowany system, mozliwe ze bedziesz musial
zajac sie polaczeniem nowo utworzonego /emul/aout ze starym. 
}
