/*	$NetBSD: msg.mi.pl,v 1.89 2013/03/19 22:16:54 garbled Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.mi.pl,v 1.36 2004/04/17 18:55:35 atatat Exp       */

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

/* MI Message catalog -- polish, machine independent */

message usage
{uzycie: sysinst [-D] [-f plik_definicja] [-r wersja]
}

/*
 * We can not use non ascii characters in this message - it is displayed
 * before the locale is set up!
 */
message sysinst_message_language
{Komunikaty instalacyjne w jezyku polskim}

message sysinst_message_locale
{pl_PL.ISO8859-2}

message Yes {Tak}
message No {Nie}
message All {Wszystkie}
message Some {Niektore}
message None {Zadne}
message none {zadne}
message ok {ok}
message OK {OK}
message unchanged {niezmienione}
message On {Wlaczone}
message Off {Wylaczone}
message Delete {Usunac?}

message install
{zainstalowac}

message reinstall
{przeinstaluj pakiety dla}

message upgrade
{zaktualizowac}

message mount_failed
{Montowanie %s nie powiodlo sie. Kontynuowac?
}

message hello
{Witaj w sysinst, systemie instalacyjnym NetBSD-@@VERSION@@.
To, oparte na menu, narzedzie zostalo stworzone aby pomoc ci zainstalowac
NetBSD na twardym dysku, lub zaktualizowac istniejacy system NetBSD,
zuzywajac minimum czasu.  W ponizszych menu mozesz zmienic aktualne
ustawienia poprzez naciskanie klawiszy (a, b, c, ...). Klawisze strzalek
takze moga dzialac.  Aktywujesz ustawienie poprzez nacisniecie ENTER.
}

message thanks
{Dziekujemy za uzywanie NetBSD!
}

message installusure
{Zdecydowales sie zainstalowac NetBSD na twardym dysku. Spowoduje to zmiane
informacji na twoim dysku. Powinienes zrobic pelny backup danych przed
rozpoczeciem tej procedury!  Zostana wykonane nastepujace czynnosci:
	a) Podzial dysku twardego
	b) Stworzenie nowych systemow plikow BSD
	c) Wgranie i zainstalowanie pakietow dystrybucji

(Po wprowadzeniu informacji o partycjach, ale jeszcze zanim twoj dysk
zostanie zmieniony, bedziesz mial mozliwosc przerwac te procedure.)

Czy kontynuowac?
}

message upgradeusure
{Ok, zaktualizujmy NetBSD na twoim dysku.  Jak zawsze, spowoduje to
zmiane informacji na twoim dysku. Powinienes zrobic pelny backup danych
przed rozpoczeciem!  Czy napewno chcesz zaktualizowac NetBSD?
(Jest to ostatnie ostrzezenie zanim zacznie sie modyfikacja danych na
twoich dyskach.)
}

message reinstallusure
{Ok, rozpakujmy pakiety dystrybucyjne NetBSD na bootowalny twardy dysk.
Ta procedura tylko sciaga i rozpakowuje pakiety na pre-partycjonowany
bootowalny dysk. Nie nazywa dyskow, aktualizuje bootblokow, lub zapisuje
istniejacej konfiguracji.   (Wyjdz i wybierz `instaluj' lub
`aktualizuj' jesli chcesz to zrobic.) Powinienes wykonac `instaluj' lub
`aktualizuj' przed rozpoczeciem tej procedury!

Czy napewno chcesz przeinstalowac pakiety dystrybucjne NetBSD?
(Jest to ostatnie ostrzezenie zanim zacznie sie modyfikacja danych na
twoich dyskach.)
}

message nodisk
{Nie moge znalezc zadnych twardych dyskow do uzycia z NetBSD. Zostaniesz
przeniesiony do menu glownego.
}

message onedisk
{Znalazlem tylko jeden dysk, %s.  Stad przyjmuje, ze chcesz %s
NetBSD wlasnie na nim.
}

message ask_disk
{Na ktorym z nich chcesz %s NetBSD? }

message Available_disks
{Dostepne dyski}

message heads
{glowice}

message sectors
{sektory}

message fs_isize
{sredni rozmiar pliku (bajty)}

message mountpoint
{punkt montowania (lub 'zaden')}

message cylname
{cyl}

message secname
{sek}

message megname
{MB}

message layout
{NetBSD uzywa BSD disklabel aby pociac czesc dysku NetBSD na kilka
partycji BSD.  Musisz teraz skonfigurowac BSD disklabel.
Masz kilka mozliwosci. Sa one opisane ponizej. 
-- Standard: partycje BSD disklabel sa ustawiane przez ten program. 
-- Uzyj istniejacych: Uzywa aktualnych partycji. Musisz je zamontowac.

Dysk NetBSD to %d Megabajtow. 
Standard wymaga przynajmniej %d Megabajtow.
Standard z X Window System wymaga przynajmniej %d Megabajtow.
}

message Choose_your_size_specifier
{Wybranie Megabajtow nada partycji rozmiar bliski twojemu wyborowi,
ale dopasowany do granic cylindrow.  Wybranie sektorow pozwoli ci na
bardziej dokladne okreslenie rozmiarow.  Na nowych dyskach ZBR rozmiar
cylindra zmienia sie w zaleznosci od miejsca na dysku, jest wiec
niewielka korzysc z dopasowania cylindrow.  Na starszych dyskach
bardziej wydajne jest podawanie rozmiaru dysku, ktore sa
wielokrotnoscia aktualnego rozmiaru cylindra.

Wybierz specyfikator rozmiaru}

message ptnsizes
{Mozesz teraz zmienic rozmiary partycji systemowych. Domyslne ustawienia
alokuja cala przestrzen na glowny system plikow, aczkolwiek mozesz zdefiniowac
osobne partycje /usr (dodatkowe pliki systemowe), /var (dane systemowe i logi)
lub /home (katalogi domowe uzytkownikow).

Wolna przestrzen zostanie dodana do partycji oznaczonej '+'.
}

message ptnheaders
{
       MB         Cylindry    Sektory   System plikow
}

message askfsmount
{Punkt montowania?}

message askfssize
{Rozmiar dla %s w %s?}

message askunits
{Zmien jednostki wejsciowe (sektory/cylindry/MB)}

message NetBSD_partition_cant_change
{partycja NetBSD}

message Whole_disk_cant_change
{Caly dysk}

message Boot_partition_cant_change
{partycja uruchomic}

message add_another_ptn
{Dodaj partycje zdefiniowana przez uzytkownika}

message fssizesok
{Zaakceptuj rozmiary partycji. Wolne miejsce %d %s, %d wolnych partycji.}

message fssizesbad
{Zmniejsz rozmiary partycji o %d %s (%u sektorow).}

message startoutsidedisk
{Wartosc poczatkowa ktora podales jest poza koncem dysku.
}

message endoutsidedisk
{Przy tej wartosci, koniec partycji znajduje sie poza koncem dysku. Rozmiar
twojej partycji zostal zmniejszony do %d %s.
}

message toobigdisklabel
{
Ten dysk jest zbyt duzy dla tablicy partycji disklabel i dlatego
nie moze zostac uzyty jako dysk starowy ani nie moze przechowywac
glownej partycji.
}

message fspart
{Mamy teraz twoje partycje BSD-disklabel jako:

}

message fspart_header	/* XXX abbreviations (or change fspart_row below) */
{    Rozm %3s  Prze %3s Koniec %3s Typ SP     Ochrona Montowana Jako
   --------- --------- ---------- ---------- ------- ----- ----------
}

message fspart_row
{%9lu %9lu %10lu %-10s %-7s %-9s %s}

message show_all_unused_partitions
{Pokaz wszystkie nieuzywane partycje}

message partition_sizes_ok
{Rozmiary partycji w porzadku}

message edfspart
{Powinienes najpierw ustawic rodzaj systemu plikow (SP). 
Pozniej inne wartosci.

Aktualne wartosci dla partycji %c:

                          MB cylinders   sectors
                     ------- --------- ---------
}

message fstype_fmt
{ Typ systemu plikow: %9s}

message start_fmt
{           poczatek: %9u %8u%c %9u}

message size_fmt
{            rozmiar: %9u %8u%c %9u}

message end_fmt
{             koniec: %9u %8u%c %9u}

message bsize_fmt
{      rozmiar bloku: %9d bajtow}

message fsize_fmt
{  rozmiar fragmentu: %9d bajtow}

message isize_fmt
{ Sredni rozm. pliku: %9d bajtow}
message isize_fmt_dflt
{ Sredni rozm. pliku:         4 fragmenty}

message newfs_fmt
{              newfs: %9s}

message mount_fmt
{         montowanie: %9s}

message mount_options_fmt
{   opcje montowania: }

message mountpt_fmt
{   punkt montowania: %9s}

message toggle
{Przelacz}

message restore
{Odzyskaj oryginalne wartosci}

message Select_the_type
{Wybierz typ}

message other_types
{inne typy}

message label_size
{%s
Specjalne wartosci, ktore moga byc podane jako wartosci rozmiaru:
    -1:   az do konca czesci dysku NetBSD
   a-%c:   zakoncz ta partycje tam gdzie partycja X sie zaczyna

rozmiar (%s)}

message label_offset
{%s
Specjalne wartosci, ktore moga byc podane jako wartosci przesuniecia:
    -1:   zacznij na poczatku czesci dysku NetBSD
   a-%c:   zacznij na koncu partycji X  (a, b, ... %c)

poczatek (%s)}

message invalid_sector_number
{Zle uformowany numer sektora
}

message Select_file_system_block_size
{Wybierz rozmiar bloku dla systemu plikow}

message Select_file_system_fragment_size
{Wybierz rozmiar fragmentu dla systemu plikow}

message packname
{Podaj nazwe dla swojego dysku NetBSD}

message lastchance
{Ok, jestesmy teraz gotowi zainstalowac NetBSD na twoim dysku (%s). Nic
nie zostalo jeszcze zapisane. Masz teraz ostatnia szanse na przerwanie tego
procesu poki nic nie zostalo jeszcze zmienione.

Czy kontynuowac?
}

message disksetupdone
{Okej, pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk doceloway, oraz utworzyl system plikow i sprawdzil
nowe partycje, ktore podales jako docelowe.
}

message disksetupdoneupdate
{Okej, pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk docelowy, oraz sprawdzil nowe partycje, ktore
podales jako docelowe.
}

message openfail
{Nie moglem otworzyc %s, blad: %s.
}

message mountfail
{zamontowanie urzadzenia /dev/%s%c na %s nie powiodlo sie.
}

message extractcomplete
{Rozpakowywanie wybranych pakietow dla NetBSD-@@VERSION@@ zakonczone.
System moze sie teraz uruchomic z wybranego twardego dysku. Aby zakonczyc
instalacje, sysinst da ci mozliwosc skonfigurowania kilku istotnych rzeczy.
}

message instcomplete
{Instalacja NetBSD-@@VERSION@@ zostala zakonczona. System powinien
uruchomic sie z twardego dysku. Wykonaj polecenia zawarte w pliku
INSTALL o koncowej konfiguracji systemu.

Przynajmniej powinienes wyedytowac /etc/rc.conf aby odpowiadal twoim
potrzebom. Przegladnij /etc/defaults/rc.conf aby poznac domyslne wartosci.
}

message upgrcomplete
{Aktualizacja NetBSD-@@VERSION@@ zostala zakonczona. Bedziesz teraz
musial wykonac polecenia zawarte w pliku INSTALL, aby uzyskac system
odpowiadajacy twoim potrzebom.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika, oraz skopiowac spowrotem
pliki z haslami (biorac pod uwage nowe konta systemowe ktore mogly
zostac utworzone dla tej wersji), jesli uzywales lokalnych plikow hasel.
}


message unpackcomplete
{Rozpakowywanie dodatkowych pakietow NetBSD-@@VERSION@@ zostalo zakonczone. 
Musisz teraz wykonac
polecenia zawarte w pliku INSTALL aby przekonfigurowac system do swoich
potrzeb.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika.
}

message distmedium
{Twoj dysk jest teraz gotowy na zainstalowanie jadra oraz pakietow
dystrybucyjnych. Jak napisano w pliku INSTALL masz terz kilka opcji. Dla
ftp lub nfs, musisz byc podlaczony do sieci z dostepem do odpowidnich maszyn.

Pakietow wybranych %d, zainstalowanych %d. Nastepnym pakietem jest %s.

}

message distset
{Dystrybucja NetBSD jest rozbita w kolekcje pakietow dystrybucyjnych.
Czesc z nich to pakiety podstawowe wymagane przez wszystkie instalacje,
a czesc nie jest przez wszystkie wymagana. Mozesz zainstalowac je
wszystkie (Pelna instalacja) lub wybrac z opcjonalnych pakietow.
} /* XXX add 'minimal installation' */

message ftpsource
{Ponizej masz site %s, katalog, uzytkownika, oraz haslo gotowe do uzycia.
Jesli "uzytkownik" to "ftp", wtedy haslo nie jest wymagane.

}

message email
{adres e-mail}

message dev
{urzadzenie}

message nfssource
{Wprowadz hosta NFS oraz katalog gdzie znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki .tgz oraz, ze musi byc
dostepny przez NFS.

}

message floppysource
{Podaj urzadzenie bedace stacja dyskietek oraz katalog pomocniczy
w docelowym systemie plikow. Pliki z pakietami instalacyjnymi musza
znajdowac sie w glownym katalogu dyskietki.

}

message cdromsource
{Podaj urzadzenie CDROM oraz katalog na CDROMie, w ktorym znajduje sie
dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki .tgz.

}

message Available_cds
{Dostepne CD}

message ask_cd
{Znaleziono kilka CD, prosze wybrac CD zawierajcy instalacje.}

message cd_path_not_found
{Zbiory instalacyjne nie zostaly znalezione w domyslnym polozeniu na tym
CD. Prosze sprawdzic urzadzenie i sciezke.}

message localfssource
{Podaj niezamountowane lokalne urzadzenie oraz katalog na nim, gdzie
znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki .tgz.

}

message localdir
{Podaj aktualnie zamountowany lokalny katalog, gdzie znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki .tgz.

}

message filesys
{system plikow}

message nonet
{Nie znalazlem zadnych interfejsow sieciowych do uzycia z NetBSD.
Zostaniesz przeniesiony do glownego menu.
}

message netup
{Nastepujace interfejsy sieciowe sa aktywne: %s
Czy ktorys z nich jest podlaczony do serwera?}

message asknetdev
{Znalazlem nastepujace interfejsy sieciowe : %s
\nKtorego urzadzenia mam uzyc?}

message badnet
{Nie wybrales zadnego z podanych urzadzen sieciowych. Sprobuj jeszcze raz.
Nastepujace urzadzenie sieciowe sa dostepne: %s
\nKtorego urzadzenia mam uzyc?}

message netinfo
{Aby mozna bylo uzywac sieci, potrzebujemy odpowiedzi na ponizsze pytania:

}

message net_domain
{Twoja domena DNS}

message net_host
{Twoja nazwa hosta}

message net_ip
{Twoj adres IPv4}

message net_srv_ip
{Server IPv4 number}

message net_mask
{Maska podsieci IPv4}

message net_namesrv6
{Serwer nazw IPv6}

message net_namesrv
{Serwer nazw IPv4}

message net_defroute
{bramka IPv4}

message net_media
{Typ medium sieciowego}

message netok
{Ponizej sa wartosci, ktore wprowadziles.

Domena DNS:		%s 
Nazwa hosta:		%s 
Podstawowy interfejs:	%s 
Twoj adres IP:		%s 
Maska podsieci:		%s 
Serwer nazw IPv4:	%s 
Bramka IPv4:		%s 
Medium sieciowe:	%s
}

message netok_slip
{Ponizej sa wartosci, ktore wprowadziles. Czy sa poprawne?

Domena DNS:		%s 
Nazwa hosta:		%s 
Podstawowy interfejs:	%s 
Twoj adres IP:		%s 
Adres IP serwera:	%s
Maska podsieci:		%s 
Serwer nazw IPv4:	%s 
Bramka IPv4:		%s 
Medium sieciowe:	%s
}

message netokv6
{Autkonfiguracja IPv6:	%s 
Serwer nazw IPv6:	%s 
}

message netok_ok
{Czy sa poprawne?}

message slattach {
Podaja parametry dla polecenia 'slattach'
}

message netagain
{Wprowadz jeszcze raz informacje o twojej sieci. Twoje ostatnie odpowiedzi
beda domyslnymi wartosciami.

}

message wait_network
{
Poczekaj, az interfejs sieciowy zostanie uaktywniony.
}

message resolv
{Nie moglem utworzyc /etc/resolv.conf.  Instalacja przerwana.
}

message realdir
{Nie moglem przejsc do katalogu %s: %s.  Instalacja przerwana.
}

message delete_xfer_file
{Usun po zakonczeniu instalacji}

message notarfile
{Pakiet %s nie istnieje.}

message endtarok
{Wszystkie wybrane pakiety dystrybucji zostaly rozpakowane.}

message endtar
{Wystapil blad w trackie rozpakowywania pakietow.
Twoja instalacja jest niekompletna.

Wybrales %d pakietow dystrybucyjnych.  %d pakiety nie zostaly znalezione
i %d zostalo pominietych z powodu bledow. Z  %d wyprobowanych,
%d rozpakowalo sie bez bledow i %d z bledami.

Instalacja zostala przerwana. Sprawdz zrodlo swojej dystrybucji i rozwaz
reinstalacje pakietow z glownego menu.}

message abort
{Wybrane przez ciebie opcje spowodowaly, ze zainstalowanie NetBSD jest
nie mozliwe. Instalacja zostala przerwana.
}

message abortinst
{Dystrybucja nie zostala pomyslnie wgrana. Bedziesz musial zrobic to recznie.
Instalacja zostala przerwana.
}

message abortupgr
{Dystrybucja nie zostala pomyslnie wgrana. Bedziesz musial zrobic to recznie.
Aktualizacja zostala przerwana.
}

message abortunpack
{Rozpakowanie dodatkowych pakietow nie udalo sie. Bedziesz musial
to zrobic recznie, albo wybierz inne zrodlo pakietow i sprobuj ponownie.
}

message createfstab
{Pojawil sie powazny problem! Nie mozna utworzyc /mnt/etc/fstab. Spadamy!
}


message noetcfstab
{Pomocy! Na dysku docelowym %s nie ma /etc/fstab. Przerywamy aktualizacje. 
}

message badetcfstab
{Pomocy! Nie moge przeczytac /etc/fstab na dysku %s. Przerywamy aktualizacje.
}

message X_oldexists
{Nie moge zapisac %s/bin/X jako %s/bin/X.old, poniewaz
na docelowym dysku jest juz %s/bin/X.old. Napraw to przed kontynuacja.

Jedyny sposob to uruchomic powloke z menu Narzedziowego i sprawdzic
docelowe %s/bin/X oraz %s/bin/X.old. Jesli
%s/bin/X.old pochodzi z zakonczonej aktualizacji, mozesz usunac
%s/bin/X.old i zrobic restart. Albo jesli %s/bin/X.old
pochodzi z aktualnej niekompletnej aktualizacji, mozesz usunac
%s/bin/X i przeniesc %s/bin/X.old na %s/bin/X.

Przerywamy aktualizacje.}

message netnotup
{Pojawil sie problem z konfiguracja twojej sieci. Albo twoja bramka
albo serwer nazw nie byl osiagalny przez ping. Czy chcesz skonfigurowac
siec jeszcze raz? (Nie pozwala ci kontynuowac lub przerwac instalacje.)
}

message netnotup_continueanyway
{Czy chcesz kontynuowac proces instalacji i zalozyc, ze twoja siec dziala?
(Nie przerywa procesu instalacji.)
}

message makedev
{Tworzenie plikow urzadzen ...
}

/* XXX: Translate:
* -not successful.  The upgrade has been aborted.  (Error number %d.)
* +not successful (Error number %d.). Try mounting it anyway?
*/
message badfs
{Wyglada na to, ze /dev/%s%c nie jest systemem plikow BSD ewentualnie
fsck dysku nie powiodlo sie. Zamontowac dysk pomimo tego? (Numer
bledu %d.)}

message rootmissing
{ docelowy / jest zagubiony %s.
}

message badroot
{Kompletny nowy system plikow nie przeszedl podstawowych testow.
 Jestes pewien, ze zainstalowales wszystkie wymagane pakiety? 
}

message fd_type
{System plikow na dyskietce}

message fdnotfound
{Nie moglem znalezc pliku na dysku.
}

message fdremount
{Dyskietka nie zostala pomyslnie zamountowana.
}

message fdmount
{Wloz dyskietke zawierajaca plik "%s.%s".

Jezeli nie masz juz wiecej dyskietek, wybierz "Pakiet kompletny"
aby rozpoczac proces jego instalacji. Wybierz "Przerwij pobieranie"
zeby wybrac inne zrodlo oprogramowania.
}

message mntnetconfig
{Czy informacje o sieci, ktore podales sa prawidlowe dla tej maszyny
w reguralnej pracy i czy chcesz aby je zapisac w /etc? }

message cur_distsets
{Ponizej jest lista pakietow dystrybucyjnych, ktore zostana uzyte.

}

message cur_distsets_header
{Pakiet dystryb.                   Uzyc?
--------------------------------- -----
}

message set_base
{Base}

message set_system
{System (/etc)}

message set_compiler
{Narzedzia Kompilacyjne}

message set_games
{Gry}

message set_man_pages
{Strony Podrecznika}

message set_misc
{Inne}

message set_modules
{Moduly kernela}

message set_tests
{Programy testujace}

message set_text_tools
{Narzedzia Przetwarzania Tekstu}

message set_X11
{Pakiety X11}

message set_X11_base
{X11 base oraz klienci}

message set_X11_etc
{Konfiguracja X11}

message set_X11_fonts
{Czcionki X11}

message set_X11_servers
{Serwery X11}

message set_X11_prog
{Programowanie X11}

message set_source
{Source sets}

message set_syssrc
{Kernel sources}

message set_src
{Base sources}

message set_sharesrc
{Share sources}

message set_gnusrc
{GNU sources}

message set_xsrc
{X11 sources}

message set_debug
{Debug sets}

message set_xdebug
{Debug X11 sets}

message cur_distsets_row
{%-30s %3s}

message select_all
{Wybierz wszystkie powyzsze pakiety}

message select_none
{Odznasz wszystkie powyzsze pakiety}

message install_selected_sets
{Zainstaluj wybrane pakiety}

message tarerror
{Pojawil sie blad w trakcie rozpakowywanie pliku %s. To znaczy, ze
pewne pliki nie zostaly prawidlowo rozpakowane i twoj system
nie bedzie kompletny.

Kontynuowac rozpakowywanie pakietow?}

message must_be_one_root
{Musi byc tylko jedna partycja do zamontowania pod '/'.}

message partitions_overlap
{partycje %c i %c pokrycia.}

message No_Bootcode
{Brak kodu startowego dla glownej partycji}

message cannot_ufs2_root
{Glowny system plikow nie moze byc FFSv2 poniewaz nie ma kodu startowego dla
tej platformy.}

message edit_partitions_again
{

Mozesz albo wyedytowac tablice partycji recznie, albo poddac sie
i powrocic do glownego menu.

Edytowac tablice partycji ponownie ?}

message config_open_error
{Nie moglem otworzyc pliku konfiguracyjnego %s\n}

message choose_timezone
{Wybierz strefe czasowa, ktora najlepiej ci odpowiada z ponizszej listy. 
Nacisnij ENTER aby wybrac. 
Nacisnij 'x' a potem ENTER aby wyjsc.

 Domyslna:	%s 
 Wybrana:	%s 
 Lokalny czas:	%s %s 
}

message tz_back
{Powroc do glownej listy stref}

message swapactive
{Dysk, ktory wybrales posiada partycje wymiany, ktora moze byc aktualnie
w uzyciu jesli twoj system ma malo pamieci. Poniewaz chcesz zmienic uklad
partycji, partycja wymiany zostanie teraz wylaczona. Moze to spowodowac
pojawienie sie bledow. Jesli zuwazysz takie bledy zrestartuj komputer,
a nastepnie sprobuj jeszcze raz.}

message swapdelfailed
{Sysinst nie mogl deaktywowac partycji wymiany na dysku, ktory wybrales
do instalacji. Zrestartuj komputer i sprobuj jeszcze raz.}

message rootpw
{Haslo root'a w nowo zainstalowanym systemie nie zostalo jeszcze ustawione,
i dlatego jest puste. Czy chcesz ustawic haslo dla root'a teraz?}

message rootsh
{Mozesz  teraz wybrac, ktorej powloki ma uzywac uzytkownik root. Domyslnie
jest to /bin/sh, ale moze preferujesz inna.}

message no_root_fs
{
Nie zdefiniowano glownego systemu plikow. Musisz zdefiniowac przynajmniej
jeden mountpoint z "/".

Nacisnij <enter> aby kontynuowac.
}

message Pick_an_option {Wybierz opcje aby je wlaczyc lub wylaczyc.}
message Scripting {Skrypty}
message Logging {Logowanie}

message Status  {   Status: }
message Command {Polecenie: }
message Running {Uruchamianie}
message Finished {Zakonczone}
message Command_failed {Polecenie nie powiodlo sie}
message Command_ended_on_signal {Polecenie zakonczylo sie sygnalem}

message NetBSD_VERSION_Install_System {System Instalacyjny NetBSD-@@VERSION@@}
message Exit_Install_System {Wyjdz z Systemu Instalacyjnego}
message Install_NetBSD_to_hard_disk {Zainstaluj NetBSD na twardym dysku}
message Upgrade_NetBSD_on_a_hard_disk {Zaktualizuj NetBSD na twardym dysku}
message Re_install_sets_or_install_additional_sets {Przeinstaluj albo zainstaluj dodatkowe pakiety}
message Reboot_the_computer {Zrestartuj komputer}
message Utility_menu {Menu Narzedziowe}
message Config_menu {Menu konfiguracji}
message exit_utility_menu {Exit}
message NetBSD_VERSION_Utilities {Narzedzia NetBSD-@@VERSION@@}
message Run_bin_sh {Uruchom /bin/sh}
message Set_timezone {Ustaw strefe czasowa}
message Configure_network {Skonfiguruj siec}
message Partition_a_disk {Skonfiguruj dysk}
message Logging_functions {Funkcje logowania}
message Halt_the_system {Zatrzymaj system}
message yes_or_no {tak lub nie?}
message Hit_enter_to_continue {Nacisnij enter aby kontynuowac}
message Choose_your_installation {Wybierz swoja instalacje}
message Set_Sizes {Ustaw rozmiary partycji NetBSD}
message Use_Existing {Uzyj istniejacych romiarow partycji}
message Megabytes {Megabajty}
message Cylinders {Cylindry}
message Sectors {Sektory}
message Select_medium {Wybierz medium}
message ftp {FTP}
message http {HTTP}
message nfs {NFS}
.if HAVE_INSTALL_IMAGE
message cdrom {CD-ROM / DVD / install image media}	/* XXX translation */
.else
message cdrom {CD-ROM / DVD}
.endif
message floppy {Dyskietka}
message local_fs {Niezamontowany SP}
message local_dir {Lokalny katalog}
message Select_your_distribution {Wybierz swoja dystrybucje}
message Full_installation {Pelna instalacja}
message Full_installation_nox {Instalacja bez X11}
message Minimal_installation {Minimalna instalacja}
message Custom_installation {Inna instalacja}
message hidden {** ukryte **}
message Host {Host}
message Base_dir {Katalog}
message Set_dir_src {Katalog pakiet binary} /* fix XLAT */
message Set_dir_bin {Katalog pakiet source} /* fix XLAT */
message Xfer_dir {Transfer Katalog} /* fix XLAT */
message User {Uzytkownik}
message Password {Haslo}
message Proxy {Proxy}
message Get_Distribution {Sciagnij Dystrybucje}
message Continue {Kontynuuj}
message What_do_you_want_to_do {Co chcesz zrobic?}
message Try_again {Sprobowac jeszcze raz}
message Set_finished {Pakiet kompletny}
message Skip_set {Pomin pakiet}
message Skip_group {Pomin grupe pakietow}
message Abandon {Przerwij instalacje}
message Abort_fetch {Przerwij pobieranie}
message Device {Urzadzenie}
message File_system {SystemPlikow}
message Select_IPv6_DNS_server {  Wybierz serwer nazw IPv6}
message other {inny  }
message Perform_IPv6_autoconfiguration {Wykonac autokonfiguracje IPv6?}
message Perform_DHCP_autoconfiguration {Wykonac autkonfiguracje DHCP?}
message Root_shell {Powloka root'a}
message User_shell {Powloka user'a}

.if AOUT2ELF
message aoutfail
{Katalog do ktorego stare a.out wspoldzielone biblioteki powinny byc
przeniesione nie moze zostac utworzony. Sproboj jeszcze raz procedury
aktualizacji i upewnij sie, ze zamountowales wszystkie systemy plikow.}

message emulbackup
{Albo /emul/aout albo /emul w twoim systemie byl symbolicznym linkiem
wskazujacym na niezamontowany system. Zostalo mu dodane rozszerzenie '.old'.
Kiedy juz uruchomisz swoj zaktualizowany system, mozliwe ze bedziesz musial
zajac sie polaczeniem nowo utworzonego /emul/aout ze starym.
}
.endif

message oldsendmail
{Sendmail nie jest dostepny w tym wydaniu NetBSD. Domyslnym MTA jest
postfix. Plik /etc/mailer.conf ciagle wskazuje usuniety program
sendmail. Chcesz automatycznie uaktualnic /etc/mailer.conf dla
postfix? Jesli wybierzesz "Nie", trzeba bedzie recznie zmienic
/etc/mailer.conf, aby dzialalo dostarczanie poczty.}

message license
{Aby uzywac interfejsu sieciowego %s, musisz zgodzic sie na licencje
zawarta w pliku %s.
Aby obejrzec ten plik, mozesz wpisac ^Z, przejrzec jego zawartosc,
a nastepnie wpisac "fg".}

message binpkg
{Aby skonfigurowac binarny system pakietow, wybierz lokalizacje siecowa, z
ktorej nalezy pobrac pakiety. Gdy system wstanie, mozesz wykorzystac 'pkgin' do
zainstalowania albo odinstalowania dodatkowych pakietow.}

message pkgpath
{Ponizej wyszczegolniono protokol, hosta, katalog, nazwe uzytkownika oraz haslo,
ktore beda wykorzystane do nawiazania polaczenia. Haslo nie jest wymagane gdy
nazwa uzytkownika to "ftp".

}
message rcconf_backup_failed {Stworzenie kopii zapasowej rc.conf nie powiodlo
sie. Kontynuowac?}
message rcconf_backup_succeeded {Kopia zapasowa rc.conf zostala zapisana do %s.}
message rcconf_restore_failed {Przywracania kopii zapasowej rc.conf nie powiodlo
sie.}
message rcconf_delete_failed {Usuwanie starego wpisu %s nie powiodlo sie.}
message Pkg_dir {Katalog z pakietami}
message configure_prior {skonfiguruj przed instalacja}
message configure {skonfiguruj}
message change {zmien}
message password_set {ustawiono haslo}
message YES {TAK}
message NO {NIE}
message DONE {ZAKONCZ}
message abandoned {Porzucony}
message empty {***PUSTE***}
message timezone {Strefa czasowa}
message change_rootpw {Zmien haslo root'a}
message enable_binpkg {Wlacz instalacje pakietow binarnych}
message enable_sshd {Wlacz sshd}
message enable_ntpd {Wlacz ntpd}
message run_ntpdate {uruchom ntpdate podczas startu systemu}
message enable_mdnsd {Wlacz mdnsd}
message add_a_user {Add a user}
message configmenu {Skonfiguruj dodatkowe elementy w razie potrzeby.}
message doneconfig {Konfiguracja zakonczona}
message Install_pkgin {Zainstaluj pkgin i uaktualnij podsumowanie pakietow}
message binpkg_installed 
{Skonfigurowales system tak aby wykorzystywal pkgin do instalacji pakietow
binarnych. Aby zainstalowac pakiet, wykonaj:

pkgin install <packagename>

z powloki root'a. Jesli potrzebujesz wiecej informacji przeczytaj strone
podrecznika pkgin(1).}
message Install_pkgsrc {Pobierz i rozpakuj pkgsrc}
message pkgsrc
{Instalacja pkgsrc wymaga rozpakowania archiwum pobranego z sieci.
Ponizej wyszczegolniono protokol, hosta, katalog, nazwe uzytkownika oraz haslo,
ktore beda wykorzystane do nawiazania polaczenia. Haslo nie jest wymagane gdy
nazwa uzytkownika to "ftp".

}
message Pkgsrc_dir {katalog pkgsrc}
message get_pkgsrc {Pobierz i rozpakuj pkgsrc w celu tworzenia pakietow ze
zrodel}
message retry_pkgsrc_network {Konfiguracja sieci nie powiodla sie. Sprobowac
ponownie?}
message quit_pkgsrc {Zakoncz bez zainstalowania pkgsrc}
message pkgin_failed 
{Instalacja pkgin nie powiodla sie, prawdopodobnie dlatego ze nie znaleziono 
pakietow binarnych.
Sprawdz sciezke pakietow i sprobuj ponownie.}
message failed {Nie powiodlo sie}
message addusername {8 character username to add:}
message addusertowheel {Do you wish to add this user to group wheel?}
