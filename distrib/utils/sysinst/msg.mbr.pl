/*	$NetBSD: msg.mbr.pl,v 1.1 2003/05/16 19:15:01 dsl Exp $	*/

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
{   Calkowity rozmiar dysku %d %s.

   Pocz(%3s)  Koniec(%3s) Rozm(%3s)  Rodzaj
   ---------- ----------- ---------- ----
}

message part_row_start_unused
{%-1d:                                 }

message part_row_start_used
{%-1d: %-10d %-11d %-10d}

message part_row_end
{ %s\n}

message setbiosgeom
{Zostaniesz poproszony o podanie geometrii. Podaj wartosci jakie chcesz.
Ilosc cylindrow powinna byc <= 1024 a ilosc sektorow <= 63. Jesli twoj
BIOS jest ustawiony aby obslugiwac > 1024 cylindry po prostu zmniejsz
tutaj ta ilosc do 1024; NetBSD rozpozna reszte cylindrow.

}

.if 0
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
.endif

message realgeom
{praw. geo: %d cyl, %d glowic, %d sek  (tylko dla porownania)\n}

message biosgeom
{BIOS geom: %d cyl, %d glowic, %d sek\n}

.if 0
message reentergeom
{Wartosci podane dla geometrii sa nieprawidlowe. Sprawdz i podaj
je jeszcze raz.
}
.endif

message ovrwrite
{Twoj dysk aktualnie posiada partycje nie-NetBSD. Czy napewno chcesz ja
nadpisac z NetBSD?
}

message parttable
{Aktualnie tablica partycji na twoim dysku wyglada tak:
}

message editpart
{Edytujesz partycje %d. Podswietlona partycja to ta, ktora edytujesz.

}

message Select_to_change
{Wybierz aby zmienic}
message Kind
{Rodzaj}
message Start_and_size
{Poczatek i rozmiar}
message Set_active
{Ustaw aktywna}
message Partition_OK
{Partycje OK}

message editparttable
{Wyedytuj DOSowa tablice partycji. Podswietlona partycja jest aktualnie
aktywna. Tablica partycji wyglada tak:

}

message Choose_your_partition
{Wybierz swoje partycje}
message Edit_partition_0
{Edytuj partycje 0}
message Edit_partition_1
{Edytuj partycje 1}
message Edit_partition_2
{Edytuj partycje 2}
message Edit_partition_3
{Edytuj partycje 3}
message Reselect_size_specification
{Zmien specyfikator rozmiaru}

message Partition_Kind
{Rodzaj partycji?}
message NetBSD
{NetBSD}
message DOS_LT_32_Meg
{DOS < 32 Meg}
message DOS_GT_32_Meg
{DOS > 32 Meg}
message unused
{nie uzywana}
message dont_change
{nie zmieniaj}

message mbrpart_start_special
{
  Specjalne wartosci, ktore moga byc podane jako wartosc poczatkowa:
 -N:    zacznij na koncu partycji N
  0:    zacznij na poczatku dysku
}

message mbrpart_size_special
{
  Specjalne wartoscki, ktore moga byc podane jako wartosc rozmiaru:
 -N:    rozciagnij partycje, az do partycji N
  0:    rozciagnij partycje, az do konca dysku
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

.if BOOTSEL
message installbootsel
{Wyglada na to, ze masz wiecej niz jeden system operacyjny zainstalowany
na dysku. Czy chcesz zainstalowac program pozwalajacy na wybranie, ktory
system ma sie uruchomic kiedy wlaczasz/restartujesz komputer?}

message installmbr
{Poczatek dysku NetBSD lezy poza zakresem, ktory BIOS moze zaadresowac.
Inicjujacy bootcode w MBR musi miec mozliwosc korzystania z rozszerzonego
interfejsu  BIOS aby  uruchomic system z tej partycji.  Czy  chcesz
zainstalowac bootcode NetBSD do MBR aby bylo to mozliwe? Pamietaj, ze
taka operacja nadpisze istniejacy kod w MBR,  np. bootselector.}

message installnormalmbr
{Wybrales aby nie instalowac bootselectora. Jesli zrobiles to poniewaz
masz juz taki program zainstalowany, nic wiecej nie musisz robic.
Jakkolwiek, jesli nie masz bootselectora, normalny bootcode musi byc
uzyty, aby system mogl sie prawidlowo uruchomic. Czy chcesz uzyc normalnego
bootcode NetBSD?}

message configbootsel
{Skonfiguruj rozne opcje bootselectora. Mozesz zmienic podstawowe wpisy
menu do odpowiednich partycji, ktore sa wyswietlane kiedy system sie
uruchamia. Mozesz takze ustawic opoznienie czasowe oraz domyslny system
do uruchomienia (jesli nic nie wybierzesz przy starcie w bootmenu).\n
}

message Change_a_bootmenu_item
{Zmien bootmenu}
message Edit_menu_entry_0
{Edytuj wpis 0}
message Edit_menu_entry_1
{Edytuj wpis 1}
message Edit_menu_entry_2
{Edytuj wpis 2}
message Edit_menu_entry_3
{Edytuj wpis 3}
message Set_timeout_value
{Ustaw opoznienie}
message Set_default_option
{Ustaw domyslna opcje}

message Pick_a_default_partition_or_disk_to_boot
{Wybierz domyslna partycje/dysk do uruchomiania}
message First_active_partition
{Pierwsza aktywna partycja}
message Partition_0
{Partycja 0}
message Partition_1
{Partycja 1}
message Partition_2
{Partycja 2}
message Partition_3
{Partycja 3}
message Harddisk_0
{Dysk twardy 0}
message Harddisk_1
{Dysk twardy 1}
message Harddisk_2
{Dysk twardy 2}
message Harddisk_3
{Dysk twardy 3}
message Harddisk_4
{Dysk twardy 4}
message Harddisk_5
{Dysk twardy 5}

message bootseltimeout
{Opoznienie bootmenu: %d\n}

message defbootselopt
{Domyslna akcja bootmenu: }

message defbootseloptactive
{uruchom pierwsza aktywna partycje.\n}

message defbootseloptpart
{uruchom partycje %d.\n}

message defbootseloptdisk
{uruchom twardy dysk %d.\n}

message bootselitemname
{Podaj nazwe dla opcji}

message bootseltimeoutval
{Opoznienie w sekundach (0-3600)}

message bootsel_header
{Numer  Typ                             Wpis Menu
------ -------------------------------- ----------
}

message bootsel_row
{%-6d %-32s %s\n}
.endif
