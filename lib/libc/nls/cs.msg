$set 1
$ EPERM
1 Operace není povolena
$ ENOENT
2 Neexistující soubor nebo adresáø
$ ESRCH
3 Proces neexistuje
$ EINTR
4 Systémové volání pøeru¹eno
$ EIO
5 Vstupní/výstupní chyba
$ ENXIO
6 Zaøízení není nakonfigurováno
$ E2BIG
7 Seznam argumentù pøíli¹ dlouhý
$ ENOEXEC
8 Chyba ve formátu spou¹tìného souboru
$ EBADF
9 ©patný souborový deskriptor
$ ECHILD
10 Neexistující proces potomka
$ EDEADLK
11 Poøizování prostøedku by vedlo k deadlocku
$ ENOMEM
12 Není mo¾no alokovat pamì»
$ EACCES
13 Permission denied
$ EFAULT
14 ©patná adresa
$ ENOTBLK
15 Vy¾adováno blokové zaøízení
$ EBUSY
16 Zaøízení zamìstnáno
$ EEXIST
17 Soubor existuje
$ EXDEV
18 Tvrdý link vede na jiný filesystém ne¾ soubor
$ ENODEV
19 Operace není zaøízením podporována
$ ENOTDIR
20 Nejedná se o adresáø
$ EISDIR
21 Jedná se o adresáø
$ EINVAL
22 ©patný argument
$ ENFILE
23 Pøíli¹ mnoho otevøených souborù v systému
$ EMFILE
24 Pøíli¹ mnoho otevøených souborù
$ ENOTTY
25 Pro dané zaøízení nevhodné ioctl
$ ETXTBSY
26 Soubor momentálnì pou¾íván
$ EFBIG
27 Soubor pøíli¹ velký
$ ENOSPC
28 Na zaøízení nezbývá místo
$ ESPIPE
29 ©patný seek
$ EROFS
30 Souborový systém pouze pro ètení
$ EMLINK
31 Pøíli¹ mnoho linkù
$ EPIPE
32 Broken pipe
$ EDOM
33 Parametr mimo èíselný obor
$ ERANGE
34 Výsledek pøíli¹ velký
$ EAGAIN, EWOULDBLOCK
35 Prosøedek je doèasnì nedostupný
$ EINPROGRESS
36 Operace teï probíhá
$ EALREADY
37 Operace u¾ probíhá
$ ENOTSOCK
38 Socketová operace na ne-socketu
$ EDESTADDRREQ
39 Po¾adována cílová adresa
$ EMSGSIZE
40 Zpráva pøíli¹ dlouhá
$ EPROTOTYPE
41 Protokol nepodporuje daný typ socketu
$ ENOPROTOOPT
42 Protokol není dostupný
$ EPROTONOSUPPORT
43 Protokol není podporován
$ ESOCKTNOSUPPORT
44 Typ socketu není podporován
$ EOPNOTSUPP
45 Operace není podporována
$ EPFNOSUPPORT
46 Protokolová rodina není podporována
$ EAFNOSUPPORT
47 Pou¾ita adresa nekompatibilní s po¾adovaným protokolem
$ EADDRINUSE
48 Adresa se u¾ pou¾ívá
$ EADDRNOTAVAIL
49 Není mo¾no pøidìlit po¾adovanou adresu
$ ENETDOWN
50 Sí» vypnutá
$ ENETUNREACH
51 Sí» nedostupná
$ ENETRESET
52 Sí» ukonèila spojení po resetu
$ ECONNABORTED
53 Software zpùsobilo zru¹ení spojení
$ ECONNRESET
54 Spojení zru¹eno partnerem
$ ENOBUFS
55 Není k dispozici buffer
$ EISCONN
56 Socket u¾ pøipojen
$ ENOTCONN
57 Socket není pøipojen
$ ESHUTDOWN
58 Není mo¾no posílat po shutdownu socketu
$ ETOOMANYREFS
59 Pøíli¹ mnoho odkazù: nelze propojit
$ ETIMEDOUT
60 Pøekroèen èasový limit pøi pokusu o spojení
$ ECONNREFUSED
61 Spojení odmítnuto
$ ELOOP
62 Pøíli¹ mnoho úrovní symbolických linkù
$ ENAMETOOLONG
63 Jméno souboru pøíli¹ dlouhé
$ EHOSTDOWN
64 Host vypnut
$ EHOSTUNREACH
65 Neexistuje cesta k hostovi
$ ENOTEMPTY
66 Adresáø není prázdný
$ EPROCLIM
67 Pøíli¹ mnoho procesù
$ EUSERS
68 Pøíli¹ mnoho u¾ivatelù
$ EDQUOT
69 Disková kvóta pøekroèena
$ ESTALE
70 NFS handler propadl
$ EREMOTE
71 Pøíli¹ mnoho úrovní vzdáleného v cestì
$ EBADRPC
72 ©patná RPC struktura
$ ERPCMISMATCH
73 ©patná verze RPC
$ EPROGUNAVAIL
74 RPC program není pøístupný
$ EPROGMISMATCH
75 ©patná verze programu
$ EPROCUNAVAIL
76 ©patná procedura pro program
$ ENOLCK
77 Zámky nejsou k dispozici
$ ENOSYS
78 Funkce není implementována
$ EFTYPE
79 Nevhodný typ nebo formát souboru
$ EAUTH
80 Chyba autentifikace
$ ENEEDAUTH
81 Je tøeba se autentifikovat
$ EIDRM
82 Identifikátor odstranìn
$ ENOMSG
83 ®ádná zpráva po¾adovaného typu
$ EOVERFLOW
84 Hodnota pøíli¹ velká pro ulo¾ení v datovém typu
$set 2
$ SIGHUP
1 Hangup
$ SIGINT
2 Pøeru¹ení
$ SIGQUIT
3 Quit
$ SIGILL
4 Neplatná instrukce
$ SIGTRAP
5 Trace/BPT trap
$ SIGABRT
6 Abort trap
$ SIGEMT
7 EMT trap
$ SIGFPE
8 Výjimka pohyblivé èárky
$ SIGKILL
9 Zabito
$ SIGBUS
10 Chyba sbìrnice
$ SIGSEGV
11 Chyba segmentace
$ SIGSYS
12 ©patné systémove volání
$ SIGPIPE
13 Broken pipe
$ SIGALRM
14 Alarm èasovaè
$ SIGTERM
15 Ukonèeno
$ SIGURG
16 Naléhavý I/O po¾adavek
$ SIGSTOP
17 Pozastaveno (signal)
$ SIGTSTP
18 Pozastaveno
$ SIGCONT
19 Pokraèuje
$ SIGCHLD
20 Potomek skonèil
$ SIGTTIN
21 Pozastaveno (tty vstup)
$ SIGTTOU
22 Pozastaveno (tty výstup)
$ SIGIO
23 I/O mo¾né
$ SIGXCPU
24 Pøekroèen limit procesorového èasu
$ SIGXFSZ
25 Pøekroèen limit velikosti souboru
$ SIGVTALRM
26 Vypr¹el virtuální èasovaè
$ SIGPROF
27 Vypr¹el profilovací èasovaè
$ SIGWINCH
28 Zmìna velikosti okna
$ SIGINFO
29 Informaèní po¾adavek
$ SIGUSR1
30 U¾ivatelský signál 1
$ SIGUSR2
31 U¾ivatelský signál 2
