/*	$NetBSD: msg.md.pl,v 1.1 2014/07/26 19:30:46 dholland Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.10 2001/07/26 22:47:34 wiz Exp */

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

/* MD Message catalog -- Polish, mac68k version */

message md_hello
{
}

message md_may_remove_boot_medium
{
}

message fullpart
{Zainstalujemy teraz NetBSD na dysku %s. Mozesz wybrac, czy chcesz
zainstalowac NetBSD na calym dysku, czy tylko na jego czesci.

Ktora instalacje chcesz zrobic?
}

message nodiskmap
{Wyglada na to, ze twoj dysk %s, nie zostal sformatowany i zainicjalizowany
do uzycia z MacOS. Ten program pobiera strukture twojego dysku poprzez
czytanie Mapy Partycji Dysku ktora jest zapisana przez program, ktory
przygotowal dysk do uzycia z systemem Macintosh. Twoje mozliwosci to:

* Przerwac instalacje. Mozesz sformatowac dysk i zdefiniowac strukture
partycji z jakimkolwiek programem. Partycje nie musza byc zdefiniowane
jako A/UX; Proces instalacji NetBSD pozwoli ci na ich zredefiniowanie
jesli bedzie to potrzebne.

* Pozwolic aby sysinst zainicjowal Mape Partycji Dysku dla ciebie.
Spowoduje to, ze dysk nie bedzie bootowalny ani uzyteczny pod MacOS.
}

message okwritediskmap
{Sysinst sprobuje zainicjalizowac twoj dysk z nowa Mapa Partycji Dysku
uzywajac wartosci pobranych ze sterownika dysku. Ta domyslna Mapa
bedzie zawierala minimalna definicje Bloku0, Mape Partycji pozwalajaca
na maskymalnie 15 partycji dyskowych, oraz pre-definiowane partycje
dla Mapy, sterownikow dysku, oraz minimalna partycje HFS MacOS. Reszta
dysku zostanie oznaczona jako dostepna do uzycia. Partycje sterownikow dysku
nie zostana zainicjowane, wiec wolumin nie bedzie bootowalny dla MacOS.
Jesli bedziesz kontynuowal, bedziesz mogl wyedytowac nowa mape partycji
aby odpowiadala ona twoim potrzebom dla NetBSD. Mapa nie zostanie zapisana
na dysk, dopoki nie ukonczysz ustawiania partycji.

Kontynuowac?}

message part_header
{Part    przesun    rozmiar FStype uzycie    punk montazu (nazwa)
---- ---------- ---------- ------ --------- ------------------\n}

message part_row
{%3s%c %-10d %-10d %-6s %-9s %s\n}

message ovrwrite
{Twoj dysk posiada aktualnie przynajmniej jedna partycje HFS MacOS.
Nadpisywanie calego dysku spowoduje usuniecie partycji, co moze
spowodowac ze dysk stanie sie nie uzyteczny pod MacOS. Powinienes
rozwazyc utworzenie osobnej malej partycji HFS MacOS, aby upewnic
sie, ze bedziesz mogl zamontowac dysk pod MacOS. Byloby to dobre
miejsce na trzymanie kopii bootloadera NetBSD/mac68k. Pewnosc, ze
dysk moze byc zamontowany pod MacOS moze przydac sie przy probie
uzyskania dostepu do czesci dysku za pomoca aplikacji MacOS, takich
jak np stary Instalator.

Czy napewno chcesz nadpisac partycje MacOS HFS?
}

message editparttable
{Edycja Mapy Partycji Dysku: Mapa na dysku zostala przeszukana pod katem
wszystkich partycji uzytkownika, ale tylko te uzyteczne w NetBSD, zostaly
wyswietlone.
Aktualna tablica partycji wyglada tak:

}

message split_part
{Partycje, ktore wybrales zostana podzielone na dwie partycje.
Jedna bedzie partycja Apple_Scratch, a druga Apple_Free.
Jesli czesc Apple_Free fizycznie sasiaduje z inna partycja ktora jest
take typu Apple_Free, obie zostana automatycznie polaczone w jedna
typu Apple_Free. Mozesz pozniej zmienic ja w partycje innego typu.

Wybrana partycja ma %s blokow. Wprowadz ilosc blokow, ktore chcialbys
zaalokowac dla partycji Apple_Scratch. Jesli podasz 0 lub wartosc wieksza
niz dozwolona, partycje pozostanie bez zmian.

}

message scratch_size
{Rozmiar podzielonej czesci Apple_Scratch}

message diskfull
{Probowales podzielic istniejaca partycje na dwie czesci, ale nie ma na to
miejsca w Mapie Partycji Dysku. Jest to prawdopodobnie spowodowane tym, ze
twoj program formatujacy nie zarezerwowal dodatkowej przestrzeni w
oryginalnej Mapie Partycji Dysku na przyszle modyfikacje takie jak ta.
Mozesz teraz:

* Porzucic nadzieje na podzielenie tej partycji, z aktualna
Mapa Partycji Dysku, lub
* Pozwolic aby sysinst zainstalowal Nowa Mape na dysku. Spowoduje to
utrate wszystkich danych na wszystkich partycjach. UZYWAJ Z UWAGA!

Czy chcesz zrezygnowac z podzialu tej partycji?}

message custom_mount_point
{Podaj Punkt Montazu dla aktualnie wybranej partycji. Powinna byc to
unikalna nazwa, zaczynajaca sie od "/", ktora nie jest juz uzywana
przez inna partycje.

}

message sanity_check
{W trakcie konfigurowania twojego dysku zostaly znalezione nastepujace
anomalie. Mozesz je zignorowac i kontynuowac, ale zrobienie tego moze
spowodowac nie powodzenie instalacji. Oto problemy, ktore wystapily:

}

message dodiskmap
{Konfigurowanie Mapy Partycji Dysku ...
}

message label_error
{Nowa etykieta partycji na dysku nie zgadza sie z obecna etykieta w kernelu.
Kazda proba kontynuacji prawdopodobnie skonczy sie uszkodzeniem istniejacych
wszesniej partycji. Ale nowa mapa partycji dyskowych zostala zapisana na
dysk i bedzie dostepna przy nastepnym uruchomieniu NetBSD. Prosze
natyczmiast uruchomic ponownie komputer i wznowic proces instalacji.
}

.if debug
message mapdebug
{Mapa partycji:
HFS count: %d
Root count: %d
Swap count: %d
Usr count: %d
Usable count: %d
}

message dldebug
{Disklabel:
bsize: %d
dlcyl: %d
dlhead: %d
dlsec: %d
dlsize: %d
}

message newfsdebug
{Uruchamianie newfs na partycji: %s\n
}

message geomdebug
{Wywolywanie: %s %s\n
}

message geomdebug2
{Wywolywanie: %s %d\n"
}
.endif

message partdebug
{Partycja %s%c:
Przesuniecie: %d
Rozmiar: %d
}

message disksetup_no_root
{* Nie zdefiniowano partycji dla Root!\n}

message disksetup_multiple_roots
{* Zdefiniowano kilka partycji dla Root\n}

message disksetup_no_swap
{* Nie zdefiniowano partycji dla SWAP!\n}

message disksetup_multiple_swaps
{* Zdefiniowano kilka partycji dla SWAP\n}

message disksetup_part_beginning
{* Partycja %s%c zaczyna sie poza koncem dysku\n}

message disksetup_part_size
{* Partycja %s%c wykracza poza koniec dysku\n}

message disksetup_noerrors
{** Nie znaleziono bledow lub anomali na dyskach.\n}

message parttable_fix_fixing
{Naprawianie partycji %s%c\n}

message parttable_fix_fine
{Partycja %s%c jest w porzadku!\n}

message dump_line
{%s\n}

message set_kernel_1
{Kernel (GENERIC)}
message set_kernel_2
{Kernel (GENERICSBC)}
