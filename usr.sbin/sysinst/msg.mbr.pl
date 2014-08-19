/*	$NetBSD: msg.mbr.pl,v 1.2.6.2 2014/08/20 00:05:13 tls Exp $	*/

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* MBR Message catalog -- Polish, i386 version */

message fullpart
{Zainstalujemy teraz NetBSD na dysku %s. Mozesz wybrac, czy chcesz
zainstalowac NetBSD na calym dysku, czy tylko na jego czesci.

Instalacja na czesci dysku, tworzy partycje, lub 'plaster', dla NetBSD
w tablicy partycji MBR twojego dysku. Instalacja na calym dysku jest
`zdecydowanie polecana': zabiera ona caly MBR. Spowoduje to calkowita
utrate danych na dysku. Uniemozliwia ona take pozniejsza instalacje kilku
systemow na tym dysku (chyba, ze nadpiszesz NetBSD i przeinstalujesz uzywajac
tylko czesci dysku).

Ktora instalacje chcesz zrobic?
}

message Select_your_choice
{Wybierz}
message Use_only_part_of_the_disk
{Uzyj tylko czesci dysku}
message Use_the_entire_disk
{Uzyj calego dysku}

/* the %s's will expand into three character strings */
message part_header
{   Calkowity rozmiar dysku %lu %s.

.if BOOTSEL
    Pocz(%3s)  Rozm(%3s) Flg Rodzaj                  Wpis menu
   ---------- ---------- --- ----------------------- ---------
.else
    Pocz(%3s)  Rozm(%3s) Flg Rodzaj
   ---------- ---------- --- ------
.endif
}

message part_row_used
{%10d %10d %c%c%c}

message noactivepart
{Nie zaznaczyles aktywnej partycji. Moze to spowodowac, ze twoj system nie
uruchomi sie prawidlowo. Czy partycja NetBSD ma zostac zaznaczona jako aktywna?}

message setbiosgeom
{
Zostaniesz poproszony o podanie geometrii dysku:
Podaj ilosc sektorow na sciezce (maksimum 63) oraz
ilosc glowic (maksimum 256) ktore BIOS ustalil jako
wlasciwe parametry dysku.
Ilosc cylindrow zostanie obliczona automatycznie na
podstawie podanych danych i rozmiaru dysku.
}

message nobiosgeom
{Sysinst nie mogl automatycznie rozpoznac geometrii dysku z BIOS. 
Fizyczna geometria to %d cylindrow %d sektorow %d glowic\n}

message biosguess
{Uzywajac informacji z dysku, najlepsze parametry geometrii dysku z BIOS to
%d cylindrow %d sektorow %d glowic\n}

message realgeom
{praw. geo: %d cyl, %d glowic, %d sek  (tylko dla porownania)\n}

message biosgeom
{BIOS geom: %d cyl, %d glowic, %d sek\n}

message ovrwrite
{Twoj dysk aktualnie posiada partycje nie-NetBSD. Czy napewno chcesz ja
nadpisac z NetBSD?
}

message Partition_OK
{Partycje OK}

message ptn_type
{    rodzaj: %s}
message ptn_start
{  poczatek: %d %s}
message ptn_size
{   rozmiar: %d %s}
message ptn_end
{    koniec: %d %s}
message ptn_active
{   aktywna: %s}
message ptn_install
{ do instalacji: %s}
.if BOOTSEL
message bootmenu
{w menu startowym: %s}
message boot_dflt
{   domyslna: %s}
.endif

message get_ptn_size {%swielkosc (maksimum %d %s)}
message Invalid_numeric {Nieprawidlowa wartosc: }
message Too_large {Zbyt duza wartosc: }
message Space_allocated {Wykorzystane miejsce: }
message ptn_starts {Miejsce od %d..%d %s (rozmiar %d %s)\n}
message get_ptn_start {%s%sStart (od %s)}
message get_ptn_id {Typ partycji (0..255)}
message No_free_space {Brak wolnego miejsca}
message Only_one_extended_ptn {Moze byc tylko jedna partycja typu rozszerzonego}


message editparttable
{Wyedytuj DOSowa tablice partycji. Tablica partycji wyglada tak:

}

message Partition_table_ok
{Tablica partycji jest poprawna}

message Dont_change
{Nie zmieniaj}
message Other_kind
{Inny typ, podaj identyfikator liczbowy}


message reeditpart
{

Czy chcesz zmienic tablice partycji (MBR)? Brak zgody przerwie instalacje.
}

message nobsdpart
{Nie ma partycji NetBSD w tablicy partycji MBR.}

message multbsdpart
{W tablicy partycji MBR znajduje sie kilka partycji NetBSD.
 Powinienies oznaczyc jedna z nich jako przeznaczona do instalacji.
}

message dofdisk
{Konfigurowanie DOSowej tablicy partycji ...
}

message wmbrfail
{Nadpisanie MBR nie powiodlo sie. Nie moge kontynuowac.}

.if 0
.if BOOTSEL
message Set_timeout_value
{Ustaw opoznienie}

message bootseltimeout
{Opoznienie bootmenu: %d\n}

.endif
.endif
