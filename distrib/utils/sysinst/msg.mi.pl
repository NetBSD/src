/*	$NetBSD: msg.mi.pl,v 1.26 2003/07/18 10:29:37 dsl Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.mi.en,v 1.86 2002/04/04 14:26:44 ad Exp 	*/

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

/* MI Message catalog -- english, machine independent */

message usage
{uzycie: sysinst [-r wersja] [-f plik-definicji]
}

message sysinst_message_language
{Installation messages in Polish}	/* XXX translate */

message Yes {Tak}
message No {Nie}
message All {All}	/* XXX translate */
message Some {Some}	/* XXX translate */
message None {Zadne}
message none {zadne}
message ok {ok}
message OK {OK}

message install
{zainstalowac}

message reinstall
{przeinstaluj pakiety dla}

message upgrade
{zaktualizowac}


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
{Na ktorym z nich chcesz zainstalowac NetBSD? }

message Available_disks	/* XXX translate */
{Available disks}

message pleasemountroot
{Glowny dysk nie jest zamountowany. Zamountuj go.

Wybrany przez ciebie dysk docelowy %s jest takze aktualnym glownym dyskiem.
Musze wiedziec czy aktualnie dzialam poza docelowym dyskiem (%sa), czy 
poza alternatywnym (powiedzmy, w %sb, twojej partycji wymiany).
Nie moge tego stwierdzic dopoki nie zamountujesz glownej partycji z ktorej uruchomiles system zapis/odczyt (np 'mount /dev/%sb /').

Przerywam i wracam do glownego menu, abys mogl uruchomic powloke.
}

message cylinders
{cylindry}

message heads
{glowice}

message sectors
{sektory}

message mountpoint
{mount point (or 'none')}

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
-- Uzyj istniejacych: Uzywa aktualnych partycji. Musisz je zamountowac.

Dysk NetBSD to %d Megabajtow. 
Standard wymaga przynajmniej %d Megabajtow.
Standard z Xami wymaga przynajmniej %d Megabajtow.
}

message sizechoice
{Zdecydowales sie podac rozmiary partycji (albo dla BSD disklabel,
lub na niektorych portach, dla plastrow MBR). Musisz najpierw wybrac
jednostke rozmiaru. Wybranie Megabajtow nada partycji rozmiar bliski
twojemu wyborowi, ale dopasowany do granic cylindrow. Wybranie sektorow
pozwoli ci na bardziej dokladne okreslenie rozmiarow. Na nowych dyskach ZBR
rozmiar cylindra zmienia sie w zaleznosci od miejsca na dysku, jest wiec
niewielka korzysc z dopasowania cylindrow. Na starszych dyskach bardziej
wydajne jest podawanie rozmiaru dysku, ktore sa wielokrotnoscia aktualnego
rozmiaru cylindra.
}

message defaultunit
{Jezeli nie wybrales 'M' (megabajty), 'G' (gigabajty), 'c' (cylindry)
lub 's' sektory rozmiary i przesuniecia podawane sa w %s.
}

message ptnsizes	/* XXX translate */
{You can now change the sizes for the system partitions.  The default is
to allocate all the space to the root filesystem, however you may wish
to have separate /usr (additional system files), /var (log files etc)
or /home (users' home directories).

Free space will be added to the partition marked with a '+'.
}

message ptnheaders	/* XXX translate */
{
       MB Cylinders   Sectors    Filesystem
}

message askfsmount	/* XXX translate */
{Mount pount?}

message askfssize	/* XXX translate */
{Size for %s in %s?}

message askunits	/* XXX translate */
{Change input units (sectors/cylinders/MB)}

message NetBSD_partition_cant_change
{partycja NetBSD - nie mozna zmienic}

message Whole_disk_cant_change
{Caly dysk - nie mozna zmienic}

message Boot_partition_cant_change
{partycja uruchomic - nie mozna zmienic}

message add_another_ptn	/* XXX translate */
{Add a user defined partition}

message fssizesok	/* XXX translate */
{Accept partition sizes.  Free space %d %s, %d free partitions.}

message fssizesbad	/* XXX translate */
{Reduce partition sizes by %d %s (%d sectors).}

message startoutsidedisk
{Wartosc poczatkowa ktora podales jest poza koncem dysku.
}

message endoutsidedisk
{Przy tej wartosci, koniec partycji znajduje sie poza koncem dysku. Rozmiar
twojej partycji zostal zmniejszony do %d %s.
}

message fspart
{Mamy teraz twoje partycje BSD-disklabel jako (Rozmiar i Przesuniecie w %s):


}

message fspart_header
{     Rozmiar  Przesun.    Koniec Typ SP Ochrona Mount Mountpoint 
   --------- --------- --------- ------ ------- ----- ----------
}

message fspart_row
{%9d %9d %9d %-6s %-7s %-5s %s}

message show_all_unused_partitions	/* XXX translate */
{Show all unused partitions}

message partition_sizes_ok		/* XXX translate */
{Partition sizes ok}

message edfspart
{Powinienes najpierw ustawic rodzaj systemu plikow (SP). Pozniej inne wartosci.

Aktualne wartosci dla partycji %c:

}

message fstype_fmt		/* XXX translate */
{  FStype: %9s}

message start_fmt		/* XXX translate */
{   start: %9d %9d %9d}

message size_fmt		/* XXX translate */
{    size: %9d %9d %9d}

message end_fmt		/* XXX translate */
{     end: %9d %9d %9d}

message bsize_fmt		/* XXX translate */
{   bsize: %9d bytes}

message fsize_fmt		/* XXX translate */
{   fsize: %9d bytes}

message newfs_fmt		/* XXX translate */
{   newfs: %9s}

message mount_fmt		/* XXX translate */
{   mount: %9s}

message mountpt_fmt		/* XXX translate */
{mount pt: %9s}

message restore		/* XXX translate */
{Restore original values}

message Select_the_type
{Wybierz rodzaj}

message label_size
{%s
Specjalne wartosci, ktore moga byc podane jako wartosci rozmiaru:
    -1:   az do konca czesci dysku NetBSD
   a-%c:   zakoncz ta partycje tam gdzie partycja X sie zaczyna

rozmiar (%s)}

message label_offset	/* XXX translate */
{%s
Specjalne wartosci, ktore moga byc podane jako wartosci przesuniecia:
    -1:   zacznij na poczatku czesci dysku NetBSD
   a-%c:   zacznij na koncu partycji X 

start (%s)}

message invalid_sector_number	/* XXX translate */
{Badly formed sector number
}

message Select_file_system_block_size	/* XXX translate */
{Select file system block size}

message Select_number_of_fragments_per_block	/* XXX translate */
{Select number of fragments per filesystem block}

message f8_per_block {8 per block}	/* XXX translate */
message f4_per_block {4 per block}	/* XXX translate */
message f2_per_block {2 per block}	/* XXX translate */
message f1_per_block {1 per block}	/* XXX translate */

message packname
{Podaj nazwe dla swojego dysku NetBSD}

message lastchance
{Ok, jestesmy teraz gotowi zainstalowac NetBSD na twoim dysku (%s). Nic 
nie zostalo jeszcze zapisane. Masz teraz ostatnia szanse na przerwanie tego
procesu poki nic nie zostalo jeszcze zmienione.

Czy kontynuowac ?
}

message disksetupdone
{Okej, pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk doceloway, oraz utworzyl system plikow i sprawdzil
nowe partycje, ktore podales jako docelowe.

Kolejny krok to sciagniecie i rozpakowanie pakietow dystrybucji.
Nacisnij <enter> aby kontynuowac.
}

message disksetupdoneupdate
{Okej, pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk docelowy, oraz sprawdzil nowe partycje, ktore
podales jako docelowe.

Kolejny krok to sciagniecie i rozpakowanie pakietow dystrybucji.
Nacisnij <enter> aby kontynuowac.
}

message openfail
{Nie moglem otworzyc %s, blad: %s.
}

message statfail
{Nie moglem pobrac wlasciwosci %s, blad: %s.
}

message unlink_fail
{Nie moglem skasowac %s, blad: %s.
}

message rename_fail
{Nie moglem zmienic nazwy %s na %s, blad: %s.
}

message deleting_files
{Jako czesc procedury aktualizacji, ponizsze pliki musza zostac usuniete:
}

message deleting_dirs
{Jako czesc procedury aktualizacji, ponizsze katalogi musza zostac usuniete:
(Zmienie nazwy tych, ktore nie sa puste):
}

message renamed_dir
{Katalog %s zostal przezwany na %s poniewaz nie byl pusty.
}

message cleanup_warn
{Wyczyszczenie istniejacej instalacji nie powiodlo sie. Moze to spowodowac
niepowodzenie przy rozpakowywaniu pakietow.
}

message nomount
{Typ partycji %c to nie 4.2BSD lub msdos i dlatego nie ma ona swojego mountpoint.}

message mountfail
{zamountowanie urzadzenia %s na %s nie powiodlo sie.
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
odpowiadajacy twoim potrzebom. Twoj stary katalog /etc zostal zapisany
jako /etc.old.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika, oraz skopiowac spowrotem
pliki z haslami (biorac pod uwage nowe konta systemowe ktore mogly
zostac utworzone dla tej wersji), jesli uzywales lokalnych plikow hasel.
}


message unpackcomplete
{Rozpakowywanie dodatkowych pakietow NetBSD-@@VERSION@@ zostalo zakonczone. 
Rozpakowywanie nadpisalo docelowy /etc. Jakikolwiek /etc.old zapisany
przez wczesniejsza aktualizacje zostal nietkniety. Musisz teraz wykonac
polecenia zawarte w pliku INSTALL aby przekonfigurowac system do swoich
potrzeb.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika.
}

message distmedium
{Twoj dysk jest teraz gotowy na zainstalowanie jadra oraz pakietow
dystrybucyjnych. Jak napisano w pliku INSTALL masz terz kilka opcji. Dla
ftp lub nfs, musisz byc podlaczony do sieci z dostepem do odpowidnich
maszyn. Jesli nie jestes gotowy aby zakonczyc instalacje teraz, mozesz
wybrac "none" i zostaniesz przeniesiony do glownego menu. Kiedy bedziesz
juz pozniej gotowy, mozesz wybrac "aktualizuj" z glownego menu, aby
zakonczyc instalacje. 
}

message distset
{Dystrybucja NetBSD jest rozbita w kolekcje pakietow dystrybucyjnych.
Czesc z nich to pakiety podstawowe wymagane przez wszystkie instalacje,
a czesc nie jest przez wszystkie wymagana. Mozesz zainstalowac je
wszystkie (Pelna instalacja) lub wybrac z opcjonalnych pakietow.
}

message ftpsource
{Ponizej masz site ftp, katalog, uzytkownika, oraz haslo gotowe do uzycia.
Jesli "uzytkownik" to "ftp", wtedy haslo nie jest wymagane.

host:		%s 
katalog:	%s 
uzytkownik:	%s
haslo:		%s 
proxy:		%s 
}

message host
{host}

message dir
{katalog}

message user
{uzytkownik}

message passwd
{haslo}

message proxy
{proxy}

message email
{adres e-mail}

message dev
{urzadzenie}

message nfssource
{Wprowadz hosta nfs oraz katalog gdzie znajduje sie dystrybucja.
Pmietaj, ze katalog musi zawierac pliki .tgz oraz, ze musi byc
dostepny via nfs.

host:		%s 
katalog:	%s 
}

message nfsbadmount
{Katalog %s:%s jest niedostepny dla nfs.}

message cdromsource
{Podaj urzadzenie CDROM oraz katalog na CDROMie, w ktorym znajduje sie
dystrybucja. Pamietaj, ze katalog musi zawierac pliki .tgz.

urzadzenie:	%s
katalog:	%s
}

message localfssource
{Podaj niezamountowane lokalne urzadzenie oraz katalog na nim, gdzie
znajduje sie dystrybucja. Pamietaj, ze katalog musi zawierac pliki .tgz.

urzadzenie:	%s
system plikow:	%s
katalog:	%s
}

message localdir
{Podaj aktualnie zamountowany lokalny katalog, gdzie znajduje sie
dystrybucja. Pamietaj, ze katalog musi zawierac pliki .tgz.

katalog:	%s
}

message filesys
{system plikow}

message cdrombadmount
{CDROM nie moze zostac zamountowany na %s.}

message localfsbadmount
{%s nie mogl byc zamountowany na lokalnym urzadzeniu %s.}

message badlocalsetdir
{%s nie jest katalogiem}

message badsetdir
{%s nie zawiera wymaganych pakietow instalacyjnych etc.tgz, 
base.tgz.  Jestes pewien, ze podales dobry katalog ?}

message nonet
{Nie znalazlem zadnych interfejsow sieciowych do uzycia z NetBSD.
Zostaniesz przeniesiony do glownego menu.
}

message netup	/* XXX translate */
{The following network interfaces are active: %s
Does one of them connect to the required server?}

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
{Ponizej sa wartosci, ktore wprowadziles.  Czy sa poprawne?

Domena DNS:		%s 
Nazwa hosta:		%s
Podstawowy interfejs:	%s 
Adres IP:		%s 
Maska podsieci:		%s
Serwer nazw IPv4:	%s 
Bramka IPv4:		%s 
Medium sieciowe:	%s
}

message netokv6
{Autkonfiguracja IPv6:	%s 
Serwer nazw IPv6:	%s 
}

message netagain
{Wprowadz jeszcze raz informacje o twojej sieci. Twoje ostatnie odpowiedzi
beda domyslnymi wartosciami.

}

message resolv
{Nie moglem utworzyc /etc/resolv.conf.  Instalacja przerwana.
}

message realdir
{Nie moglem przejsc do katalogu %s: %s.  Instalacja przerwana.
}

message ftperror_cont
{Ftp wykrylo blad. Nacisnij <enter> aby kontynuowac.}

message ftperror
{Ftp nie moze sciagnac pliku. Czy chcesz sprobowac jeszcze raz?}

message distdir
{Jakiego katalogu powinienem uzyc dla %s? }

message verboseextract
{Czy w trakcie rozpakowywania plikow, chcesz widziec nazwe aktualnie
wypakowywanego pliku ?
}

message notarfile
{Pakiet %s nie istnieje.

Kontynuowac rozpakowywanie pakietow?}

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
{Pomocy! Nie moge przeczytac /etc/fdstab na dysku %s. Przerywamy aktualizacje.
}

message etc_oldexists
{Nie moge zapisac /etc jako /etc.old, poniewaz docelowy dysk juz posiada
/etc.old. Napraw to przed kontynuacja.

Jedyna mozliwosc to uruchomienie powloki z menu Narzedziowego,
sprawdzenie docelowego /etc i /etc.old. Jesli /etc.old pochodzi z
zakonczonej aktualizacji mozesz to usunac (rm -rf /etc.old), a potem
zresetowac komputer. Albo jesli /etc.old jest z aktualnej niekompletnej
aktualizacji mozesz usunac /etc (rm -rf /etc) a potem przeniesc /etc.old
na /etc (mv /etc.old /etc).

Przerywamy aktualizacje.}

message X_oldexists
{Nie moge zapisac /usr/X11R6/bin/X jako /usr/X11R6/bin/X.old, poniewaz
na docelowym dysku jest juz /usr/X11R6/bin/X.old. Napraw to przed kontynuacja.

Jedyny sposob to uruchomic powloke z menu Narzedziowego i sprawdzic
docelowe /usr/X11R6/bin/X oraz /usr/X11R6/bin/X.old. Jesli
/usr/X11R6/bin/X.old pochodzi z zakonczonej aktualizacji, mozesz usunac
/usr/X11R6/bin/X.old i zrobic restart. Albo jesli /usr/X11R6/bin/X.old
pochodzi z aktualnej niekompletnej aktualizacji, mozesz usunac
/usr/X11R6/bin/X i przeniesc /usr/X11R6/bin/X.old na /usr/X11R6/bin/X.

Przerywamy aktualizacje.}

message netnotup
{Pojawil sie problem z konfiguracja twojej sieci. Albo twoja bramka
albo serwer nazw nie byl osiagalny przez ping. Czy chcesz skonfigurowac
siec jeszcze raz? (Nie pozwala ci kontynuowac lub przerwac instalacje.)
}

message netnotup_continueanyway
{Czy chcesz kontynuowac proces instalacji i zalozyc, ze twoja siec dziala?
(Nie przerywa proces instalacji.)
}

message makedev
{Tworzenie plikow urzadzen ...
}

message badfs
{Wyglada na to, ze %s%s nie jest systemem plikow BSD albo nie powiodlo sie
jego sprawdzenie. Aktualizacja zostala przerwana. (Blad numer %d.)
}

message badmount
{System plikow %s%s nie zostal pomyslnie zamountowany. Aktualizacja przerwana.}

message rootmissing
{ docelowy / jest zagubiony %s.
}

message badroot
{Kompletny nowy system plikow nie przeszedl podstawowych testow.
 Jestes pewien, ze zainstalowales wszystkie wymagane pakiety? 
}

message fddev
{Ktorego urzadzenia dyskietek chcesz uzyc ? }

message fdmount
{Wloz dyskietke zawierajaca plik "%s". }

message fdnotfound
{Nie moglem znalezc pliku "%s" na dysku. Wloz dyskietke
zawierajaca ten plik.}

message fdremount
{Dyskietka nie zostala pomyslnie zamountowana. Mozesz:

Sprobowac jeszcze raz i wlozyc dyskietke z plikiem "%s".

Nie wgrywac wiecej plikow z dyskietek i przerwac proces.
}

message mntnetconfig
{Czy informacje o sieci, ktore podales sa prawidlowe dla tej maszyny
w reguralnej pracy i czy chcesz aby je zapisac w /etc? }

message cur_distsets
{Ponizej jest lista pakietow dystrybucyjnych, ktore zostana uzyte.

}

message cur_distsets_header
{Pakiet dystryb.       Uzyc?
---------------------- ----
}

message set_base	/* XXX translate */
{Base}

message set_system	/* XXX translate */
{System (/etc)}

message set_compiler	/* XXX translate */
{Compiler Tools}

message set_games	/* XXX translate */
{Games}

message set_man_pages	/* XXX translate */
{Online Manual Pages}

message set_misc	/* XXX translate */
{Miscellaneous}

message set_text_tools	/* XXX translate */
{Text Processing Tools}

message set_X11		/* XXX translate */
{X11 sets}

message set_X11_base	/* XXX translate */
{X11 base and clients}

message set_X11_fonts	/* XXX translate */
{X11 fonts}

message set_X11_servers	/* XXX translate */
{X11 servers}

message set_X_contrib	/* XXX translate */
{X contrib clients}

message set_X11_prog	/* XXX translate */
{X11 programming}

message set_X11_misc	/* XXX translate */
{X11 Misc.}

message cur_distsets_row
{%-27s %3s\n}

message select_all	/* XXX translate */
{Select all the above sets}

message select_none	/* XXX translate */
{Deselect all the above sets}

message install_selected_sets	/* XXX translate */
{Install selected sets}

message tarerror
{Pojawil sie blad w trakcie rozpakowywanie pliku %s. To znaczy, ze
pewne pliki nie zostaly prawidlowo rozpakowane i twoj system
nie bedzie kompletny.

Kontynuowac rozpakowywanie pakietow?}

message must_be_one_root	/* XXX translate */
{There must be a single partition marked to be mounted on '/'.}

message partitions_overlap
{partycje %c i %c pokrycia.}

message edit_partitions_again
{

Mozesz albo wyedytowac tablice partycji recznie, albo poddac sie
i powrocic do glownego menu.

Edytowac tablice partycji ponownie ?}

message not_regular_file
{Plik konfiguracyjny %s nie jest plikiem regularnym.\n}

message out_of_memory
{Za malo pamieci (alokacja pamieci nie powiodla sie).\n}

message config_open_error
{Nie moglem otworzyc pliku konfiguracyjnego %s\n}

message config_read_error
{Nie moglem odczytac pliku konfiguracyjnego %s\n}

message cmdfail
{Polecenie
	%s
nie powiodlo sie. Nie moge kontynuowac.}

message upgradeparttype
{Jedyna odpowienid partycja, ktora zostala znaleziona dla instalacji NetBSD
jest starego typu NetBSD/386BSD/FreeBSD. Czy chcesz zmienic typ tej partycji
na nowa partycje tylko dla NetBSD?}

message choose_timezone
{Wybierz strefe czasowa, ktora najlepiej ci odpowiada z ponizszej listy.
Nacisnij ENTER aby wybrac. 
Nacisnij 'x' a potem ENTER aby wyjsc.

 Domyslna:	%s 
 Wybrana:	%s 
 Lokalny czas:	%s %s 
}

message tz_back	/* XXX translate */
{ Back to main timezone list}

message choose_crypt
{Wybierz sposob szyfrowania hasel, ktorego chcesz uzywac. NetBSD moze korzystac
z DES, MD5 lub Blowfish.

Tradycyjna metoda DES jest kompatybilna z wiekszoscia unixowych systemow
operacyjnych, ale wtedy tylko 8 pierwszych znakow w hasle jest rozpoznawanych.
Metody MD5 oraz Blowfish umozliwiaja dluzsze hasla, niektorzy uwazaja to za
bardziej bezpieczne.

Jesli posiadasz siec oraz zamierasz korzystac z NIS, pamietaj o mozliwosciach
maszyn w twojej sieci i wynikajacych stad ograniczeniach.

Jezeli uaktualniasz swoj system i nie chcesz, aby zostaly dokonane zmiany w
konfiguracji, wybierz ostatnia opcje "nie zmieniaj".
}

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

message rootsh	/* XXX translate */
{You can now select which shell to use for the root user. The default is
/bin/csh, but you may prefer another one.}

message postuseexisting
{
Nie zapomnij zamountowac wszystkich systemow plikow, ktorych chcesz
uzywac w systemie. Nacisnij <enter> aby kontynuowac.
}

message no_root_fs
{
Nie zdefiniowano glownego systemu plikow. Musisz zdefiniowac przynajmniej
jeden mountpoint z "/".

Nacisnij <enter> aby kontynuowac.
}

message NetBSD_VERSION_Install_System {System Instalacyjny NetBSD-@@VERSION@@}
message Exit_Install_System {Wyjdz z Systemu Instalacyjnego}
message Install_NetBSD_to_hard_disk {Zainstaluj NetBSD na twardym dysku}
message Upgrade_NetBSD_on_a_hard_disk {Zaktualizuj NetBSD na twardym dysku}
message Re_install_sets_or_install_additional_sets {Przeinstaluj albo zainstaluj dodatkowe pakiety}
message Reboot_the_computer {Zrestartuj komputer}
message Utility_menu {Menu Narzedziowe}
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
message Standard {Standardowa}
message Use_Existing {Istniejaca}
message Choose_your_size_specifier {Wybierz specyfikator rozmiaru}
message Megabytes {Megabajty}
message Cylinders {Cylindry}
message Sectors {Sektory}
message Select_medium {Wybierz medium}
message ftp {ftp}
message nfs {nfs}
message cdrom {cdrom}
message floppy {dyskietka}
message unmounted_fs {niezamontowany SP}
message local_dir {lokalny katalog}
message Select_your_distribution {Wybierz swoja dystrybucje}
message Full_installation {Pelna instalacja}
message Custom_installation {Inna instalacja}
message Change {Zmien}
message hidden {** hidden **}	/* XXX translate */
message Host {Host}
message Directory {Katalog}
message User {Uzytkownik}
message Password {Haslo}
message Proxy {Proxy}
message Get_Distribution {Sciagnij Dystrybucje}
message Continue {Kontynuuj}
message What_do_you_want_to_do {Co chcesz zrobic?}
message Try_again {Sprobowac jeszcze raz}
message Give_up {Poddac sie}
message Ignore_continue_anyway {Zignorowac, kontynuowac}
message Set_finished {Set finished}  /* XXX translate */
message Abort_install {Przerwac instalacje}
message Password_cipher {Kodowanie hasel}
message DES {DES}
message MD5 {MD5}
message Blowfish_2_7_round {Blowfish 2^7 round}
message do_not_change {nie zmieniaj}
message Device {Urzadzenie}
message File_system {SystemPlikow}
message Change_directory_path {Zmien sciezke katalogu}
message Select_IPv6_DNS_server {  Wybierz serwer nazw IPv6}
message other {inny  }
message Perform_IPv6_autoconfiguration {Wykonac autokonfiguracje IPv6?}
message Perform_DHCP_autoconfiguration {Wykonac autkonfiguracje DHCP?}
message Root_shell {Root shell}	/* XXX translate */
message Select_set_extraction_verbosity {Select set extraction verbosity}	/* XXX translate */
message Progress_bar_recommended {Progress bar (recommended)}	/* XXX translate */
message Silent {Silent}	/* XXX translate */
message Verbose_file_name_listing_slow {Verbose file name listing (slow)}	/* XXX translate */

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
