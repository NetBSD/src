/*	$NetBSD: msg.md.pl,v 1.2 2002/04/09 19:48:03 hubertf Exp $	*/
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
message part_header
{   Calkowity rozmiar dysku %d sektorow.

    Pocz(%3s) Rozm(%3s)  Koniec(%3s)Rodzaj
   ---------- ---------- ---------- ----
}

message part_row_start_unused
{%-1d:                                 }

message part_row_start_used
{%-1d: %-10d %-10d %-10d}

message part_row_end
{ %s\n}

message setbiosgeom
{Zostaniesz poproszony o podanie geometrii. Podaj wartosci jakie chcesz.
Ilosc cylindrow powinna byc <= 1024 a ilosc sektorow <= 63. Jesli twoj
BIOS jest ustawiony aby obslugiwac > 1024 cylindry po prostu zmniejsz
tutaj ta ilosc do 1024; NetBSD rozpozna reszte cylindrow.

}

message confirmbiosgeom
{Sprawdz czy geometria dysku z BIOS ponizej jest poprawna. Mozliwe ze
ilosc cylindrow zostala zmniejszona do 1024. Jest to w porzadku o ile
reszta parametrow jest poprawna; tylko 1024 cylindry moga byc podane
w MBR, reszta zostanie odnaleziona przez NetBSD w inny sposob.

Jesli poprawiles wartosci, upewnij sie ze sa one poprawne i odpowiadaja
tym uzywanym przez inne systemy na tym dysku. Wartosci, ktore sa nie poprawne
moga spowodowac utrate danych.

}

message badgeom
{Aktualne wartosci dla geometrii twojego dysku to:

}

message realgeom
{praw. geo: %d cyl, %d glowic, %d sek  (tylko dla porownania)\n}

message biosgeom
{BIOS geom: %d cyl, %d glowic, %d sek\n}

message reentergeom
{Wartosci podane dla geometrii sa nieprawidlowe. Sprawdz i podaj
je jeszcze raz.
}

message ovrwrite
{Twoj dysk aktualnie posiada partycje nie-NetBSD. Czy napewno chcesz ja
nadpisac z NetBSD?
}

message parttable
{Aktualnie tablica partycji na twoim dysku wyglada tak:
}

message editpart
{Partycja %d wyglada tak:\n\n}

message editparttable
{Wyedytuj DOSowa tablice partycji. Podswietlona partycja jest aktualnie
aktywna. Tablica partycji wyglada tak:

}

message reeditpart
{Partycje MBR sie nakladaja, lub jest wiecej niz jedna partycja NetBSD.
Powinienes zrekonfigurowac tablice partycji MBR.

Czy chcesz ja przekonfigurowac?
}

message nobsdpart
{Nie ma partycji NetBSD w tablicy partycji MBR.}

message multbsdpart
{W tablicy partycji MBR znajduje sie kilka partycji NetBSD.
Zostanie uzyta partycja %d.}

message dofdisk
{Konfigurowanie DOSowej tablicy partycji ...
}

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
