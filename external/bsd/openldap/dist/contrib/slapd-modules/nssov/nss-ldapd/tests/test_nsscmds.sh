#!/bin/sh

# test.sh - simple test script to check output of name lookup commands
#
# Copyright (C) 2007, 2008 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

# This script expects to be run in an environment where nss-ldapd
# is deployed with an LDAP server with the proper contents (nslcd running).
# FIXME: update the above description and provide actual LDIF file
# It's probably best to run this in an environment without nscd.

# note that nscd should not be running (breaks services test)

set -e

# check if LDAP is configured correctly
cfgfile="/etc/nss-ldapd.conf"
if [ -r "$cfgfile" ]
then
  :
else
  echo "test_nsscmds.sh: $cfgfile: not found"
  exit 77
fi

uri=`sed -n 's/^uri *//p' "$cfgfile" | head -n 1`
base="dc=test,dc=tld"

# try to fetch the base DN (fail with exit 77 to indicate problem)
ldapsearch -b "$base" -s base -x -H "$uri" > /dev/null 2>&1 || {
  echo "test_nsscmds.sh: LDAP server $uri not available for $base"
  exit 77
}

# basic check to see if nslcd is running
if [ -S /var/run/nslcd/socket ] && \
   [ -f /var/run/nslcd/nslcd.pid ] && \
   kill -s 0 `cat /var/run/nslcd/nslcd.pid` > /dev/null 2>&1
then
  :
else
  echo "test_nsscmds.sh: nslcd not running"
  exit 77
fi

# TODO: check if nscd is running

# TODO: check if /etc/nsswitch.conf is correct

echo "test_nsscmds.sh: using LDAP server $uri"

# the total number of errors
FAIL=0

check() {
  # the command to execute
  cmd="$1"
  # save the expected output
  expectfile=`mktemp -t expected.XXXXXX 2> /dev/null || tempfile -s .expected 2> /dev/null`
  cat > "$expectfile"
  # run the command
  echo 'test_nsscmds.sh: checking "'"$cmd"'"'
  actualfile=`mktemp -t actual.XXXXXX 2> /dev/null || tempfile -s .actual 2> /dev/null`
  eval "$cmd" > "$actualfile" 2>&1 || true
  # check for differences
  diff -Nauwi "$expectfile" "$actualfile" || FAIL=`expr $FAIL + 1`
  # remove temporary files
  rm "$expectfile" "$actualfile"
}

###########################################################################

echo "test_nsscmds.sh: testing aliases..."

# check all aliases
check "getent aliases|sort" << EOM
bar2:           foobar@example.com
bar:            foobar@example.com
foo:            bar@example.com
EOM

# get alias by name
check "getent aliases foo" << EOM
foo:            bar@example.com
EOM

# get alias by second name
check "getent aliases bar2" << EOM
bar2:           foobar@example.com
EOM

###########################################################################

echo "test_nsscmds.sh: testing ether..."

# get an entry by hostname
check "getent ethers testhost" << EOM
0:18:8a:54:1a:8e testhost
EOM

# get an entry by alias name
check "getent ethers testhostalias" << EOM
0:18:8a:54:1a:8e testhostalias
EOM

# get an entry by ethernet address
check "getent ethers 0:18:8a:54:1a:8b" << EOM
0:18:8a:54:1a:8b testhost2
EOM

# get entry by ip address
# this does not currently work, but maybe it should
#check "getent ethers 10.0.0.1" << EOM
#0:18:8a:54:1a:8e testhost
#EOM

# get all ethers (unsupported)
check "getent ethers" << EOM
Enumeration not supported on ethers
EOM

###########################################################################

echo "test_nsscmds.sh: testing group..."

check "getent group testgroup" << EOM
testgroup:*:6100:arthur,test
EOM

# this does not work because users is in /etc/group but it would
# be nice if libc supported this
#check "getent group users" << EOM
#users:*:100:arthur,test
#EOM

check "getent group 6100" << EOM
testgroup:*:6100:arthur,test
EOM

check "groups arthur | sed 's/^.*://'" << EOM
users testgroup testgroup2
EOM

check "getent group | egrep '^(testgroup|users):'" << EOM
users:x:100:
testgroup:*:6100:arthur,test
users:*:100:arthur,test
EOM

check "getent group | wc -l" << EOM
`grep -c : /etc/group | awk '{print $1 + 5}'`
EOM

check "getent group | grep ^largegroup" << EOM
largegroup:*:1005:oebrani,hpaek,enastasi,sgurski,hsweezer,utrezize,ihashbarger,lkhubba,rlatessa,behrke,kbradbury,hmachesky,hhydrick,dciviello,wselim,ngata,gcubbison,testusr2,hgalavis,hhaffey,testusr3,yautin,wvalcin,jyeater,slaforge,vpender,lvittum,hpolk,rkoonz,ngullett,btempel,igurwell,rworkowski,phaye,lbuchtel,nfunchess,fcunard,cmanno,nfilipek,dfirpo,vdelnegro,hzagami,htomlinson,khathway,gzuhlke,wworf,tabdelal,mjuris,okveton,dbye,wbrettschneide,kklavetter,ndipanfilo,psowa,osaines,uschweyen,vwaltmann,nkraker,dgivliani,purquilla,otrevor,ghanauer,oclunes,gdreitzler,gdaub,nroepke,mciaccia,tpaa,gtinnel,tfalconeri,cjody,vmigliori,vleyton,alat,znightingale,showe,zwinterbottom,lgandee,vmedici,lseehafer,gpomerance,mbodley,bdevera,bmoldan,akraskouskas,pdossous,sdebry,gsusoev,gvollrath,nriofrio,mblanchet,lmauracher,dgosser,ameisinger,clouder,ykisak,emcquiddy,zgingrich,vchevalier,nrybij
EOM

check "getent group largegroup" << EOM
largegroup:*:1005:oebrani,hpaek,enastasi,sgurski,hsweezer,utrezize,ihashbarger,lkhubba,rlatessa,behrke,kbradbury,hmachesky,hhydrick,dciviello,wselim,ngata,gcubbison,testusr2,hgalavis,hhaffey,testusr3,yautin,wvalcin,jyeater,slaforge,vpender,lvittum,hpolk,rkoonz,ngullett,btempel,igurwell,rworkowski,phaye,lbuchtel,nfunchess,fcunard,cmanno,nfilipek,dfirpo,vdelnegro,hzagami,htomlinson,khathway,gzuhlke,wworf,tabdelal,mjuris,okveton,dbye,wbrettschneide,kklavetter,ndipanfilo,psowa,osaines,uschweyen,vwaltmann,nkraker,dgivliani,purquilla,otrevor,ghanauer,oclunes,gdreitzler,gdaub,nroepke,mciaccia,tpaa,gtinnel,tfalconeri,cjody,vmigliori,vleyton,alat,znightingale,showe,zwinterbottom,lgandee,vmedici,lseehafer,gpomerance,mbodley,bdevera,bmoldan,akraskouskas,pdossous,sdebry,gsusoev,gvollrath,nriofrio,mblanchet,lmauracher,dgosser,ameisinger,clouder,ykisak,emcquiddy,zgingrich,vchevalier,nrybij
EOM

check "getent group | grep ^hugegroup" << EOM
hugegroup:*:1006:amccroskey,erathert,rrasual,mlinak,psiroky,ichewning,dtuholski,yautin,denriquez,yolivier,tnitzel,kmuros,ppedraja,mrizer,jsweezy,nriofrio,joligee,klitehiser,emcquiddy,gallanson,dbertels,tcossa,hhagee,blovig,ebattee,khartness,nforti,kfend,sgunder,wesguerra,yduft,jzych,edrinkwater,esonia,pphuaphes,ualway,tmysinger,tnaillon,ygockel,sbettridge,clapenta,igizzi,svogler,pbrentano,emanikowski,uwalpole,kwinterling,ghumbles,lparrish,ewilles,oebrani,gdrilling,wtruman,ggillim,phyer,hholyfield,epoinelli,nagerton,wbrill,bswantak,bdadds,vstirman,hbukovsky,lgadomski,sskyers,ddeguire,ekalfas,tbagne,yeven,rdubs,wvalcin,mdoering,rfidel,hkippes,lmichaud,vburton,charriman,hkarney,mswogger,klundsten,nciucci,rpastorin,tcacal,rramirez,thelfritz,hschoepfer,sdebry,vbaldasaro,asivley,vpender,akravetz,llarmore,vmaynard,lmcgeary,rheinzmann,kthede,gcummer,opoch,akertzman,ngrowney,lsobrino,hveader,jspohn,cabare,hrenart,sbrabyn,ohatto,hbrandow,dhammontree,kwidrick,ascovel,jskafec,uslavinski,imcbay,wclokecloak,cflenner,hbastidos,lcaudell,gcarlini,opuglisi,nbugtong,hbetterman,lshilling,nfunchess,nlainhart,kconkey,ktuccio,mcontreras,dasiedu,cbotdorf,rchevrette,mgavet,hchaviano,zwinterbottom,fthein,zculp,bdominga,dlargo,hbickford,lrandall,ykimbel,lautovino,cfasone,hdoiel,ediga,hmatonak,fmilsaps,amckinney,mquigg,mvanpelt,daubert,dgiacomazzi,hhysong,svielle,zanderlik,mpizzaro,bromano,kmarzili,uweyand,smullowney,rbernhagen,ajaquess,ekeuper,lbove,greiff,uransford,ewicks,cpentreath,kepps,uhayakawa,tmccamish,rdubuisson,dtashjian,ibreitbart,ffigert,ycostaneda,kmedcaf,fgrashot,tredfearn,nedgin,mrydelek,tsowells,ilamberth,hhartranft,dsharr,oport,areid,bbeckfield,bluellen,fagro,ihegener,sackles,fparness,lvaleriano,faleo,fbielecki,jeuresti,lcavez,nerbach,tschnepel,zkutchera,limbrogno,nkubley,afredin,gwaud,bmoling,rschkade,kfaure,vtresch,ekurter,estockwin,rgoonez,erostad,nrysavy,hhaffey,apurdon,llasher,jholzmiller,ashuey,rbillingsly,osaber,asemons,edurick,tgindhart,svongal,mvedder,jvillaire,dholdaway,bbrenton,lpitek,jjumalon,kjoslyn,cparee,cklem,mcoch,kmcardle,dsteever,nlemma,rfauerbach,wnunziata,fsirianni,dciullo,udatu,cjody,mvas,hkinderknecht,cpencil,rmcstay,tboxx,brodgerson,mfeil,eberkman,gdeblasio,hspiry,ilevian,wdagrella,bharnois,sscheiern,vbigalow,nschmig,pwohlenhaus,uflander,ckodish,amcgraw,cswigert,mcampagnone,inarain,kmcguire,tharr,bdaughenbaugh,garchambeault,bmarszalek,pvirelli,snotari,nspolar,skanjirathinga,fsunderland,mmesidor,lmuehlberger,glafontaine,aferge,hcarrizal,pdurando,gdeyarmond,fmarchi,wstjean,obeaufait,nslaby,dlongbotham,tplatko,jcaroll,isplonskowski,zscammahorn,sstuemke,cnoriego,nsiemonsma,lseabold,cmafnas,dhendon,bfishbeck,gkerens,eklunder,fburrough,ebusk,tmarkus,clouder,cweiss,mpellew,ojerabek,veisenhardt,vwokwicz,tvrooman,rpitter,slerew,dwittlinger,habby,mpanahon,rguinane,zneeb,eyounglas,gcervantez,kbrugal,ycobetto,tkeala,pheathcock,cmellberg,hmiazga,bmicklos,bphou,ngullett,jwinterton,lcremer,jmartha,icoard,ahandy,eparham,gtinnel,wganther,umarbury,fhalon,bsibal,uschweyen,gearnshaw,cbleimehl,omasone,cdeckard,ctetteh,arosel,pmineo,gclapham,jamber,sbonnie,eaguire,jmarugg,ihalford,wdovey,sarndt,gbitar,ovibbert,ewismer,gmilian,rginer,gdaub,showe,hlynema,rtooker,svandewalle,fhain,jlunney,jreigh,kmandolfo,leberhardt,wkhazaleh,nasmar,egrago,ablackstock,lcocherell,pvierthaler,vrunyon,kpalka,ubenken,hmuscaro,jherkenratt,pminnis,bscadden,srubenfield,cnabzdyk,mpytko,gchounlapane,pwademan,nousdahl,pcornn,zmeeker,hpalmquist,jrees,mkofoed,mkibler,lbassin,fplayfair,hmogush,nvyhnal,ileaman,gschaumburg,thoch,wconces,hliverman,gmackinder,rbrisby,isowder,rkraszewski,hzagami,obihl,nhelfinstine,mbravata,thynson,vwaltmann,tlana,ggehrke,pwutzke,zbuscaglia,ewuitschick,hgalavis,ddigerolamo,wmendell,etunby,jkimpton,mheilbrun,laksamit,hvannette,jseen,sgurski,iroiger,lcanestrini,baigner,dminozzi,uazatyan,gjankowiak,bstrede,mstirn,hfludd,mdyce,tbattista,gfaire,gapkin,esproull,gcurnutt,tstalworth,ienglert,hbrehmer,csoomaroo,kaanerud,nlinarez,jeverton,uspittler,prowena,gsantella,oreiss,rcheshier,tpaa,kwirght,gparkersmith,jquicksall,xrahaim,vwisinger,aesbensen,eorsten,imensah,omalvaez,dnegri,wmailey,tyounglas,vtowell,pgrybel,lmauracher,lschollmeier,ithum,umosser,pbeckerdite,hsabol,dhindsman,ugerpheide,gconver,lhuggler,amanganelli,omatula,zhaulk,lkimel,mruppel,egospatrick,kseisler,ehindbaugh,mdecourcey,kbartolet,vcrofton,cdegravelle,ksiering,fvallian,kalguire,dblazejewski,vdesir,tairth,hcusta,mjeon,smccaie,hpolintan,ihimmelwright,fbeatrice,yvdberg,uednilao,vmedici,sskone,dbarriball,ndrumgole,ccyganiewicz,cdrumm,usevera,vsefcovic,mfitzherbert,fberyman,upater,vpiraino,pwashuk,kshippy,bcolorado,cbarlup,cmiramon,kdevincent,mcaram,cbourek,hkohlmeyer,lringuette,lgradilla,slaningham,ksparling,tcrissinger,senrico,dlanois,iyorks,gbolay,rpikes,hcafourek,shaith,fverfaille,btheim,iambrosino,ghann,fkeef,tsearle,tsepulueda,iherrarte,fvinal,sherzberg,iiffert,astrunk,ghelderman,moller,gmassi,oahyou,cjuntunen,mvanbergen,tkelly,eziebert,nhija,sjankauskas,pdech,mmangiamele,clewicki,meconomides,tmccaffity,carguellez,prepasky,amaslyn,kmallach,ejeppesen,hwoodert,dgivliani,nglathar,fwidhalm,kheadlon,ihernan,oshough,nevan,mpilon,mviverette,beon,alat,ktriblett,ivanschaack,vnazzal,lwedner,alienhard,slaudeman,cpalmios,gishii,kpuebla,ascheno,ocrabbs,dledenbach,ebeachem,ideveyra,sspagnuolo,fsymmonds,srees,isteinlicht,bveeneman,myokoyama,agordner,xlantey,broher,bpinedo,psharits,iweibe,nchrisman,htomlinson,cdickes,draymundo,jbielicki,ulanigan,ihanneman,ppeper,ljomes,khovanesian,ibeto,ilacourse,iseipel,iogasawara,jglotzbecker,mferandez,gpomerance,pdulac,mgayden,skoegler,kbattershell,uvanmatre,wvermeulen,ekenady,ikulbida,htsuha,lvanconant,njordon,oosterhouse,tmelland,lspielvogel,bmarlin,bouten,fgoben,bjolly,iyorgey,htilzer,dgosser,gcobane,vpeairs,dloubier,zfarler,fvascones,awhitt,cscullion,nkempon,rgriffies,wconstantino,opizzuti,scocuzza,pgreenier,ueriks,cwank,mdanos,kmisove,ndesautels,hlichota,cgalinol,rlambertus,zvagt,ohoffert,vchevalier,vwabasha,amayorga,mtintle,rbloomstrand,swoodie,gportolese,hriech,ckerska,gvollrath,bdevera,lmadruga,mbeagley,hdyner,fcha,rlatessa,lsivic,mdedon,mcashett,ubynum,lcoulon,cbrechbill,kgremminger,yfrymoyer,pahles,guresti,kmayoras,mbodley,phalkett,kolexa,fsapien,cghianni,oalthouse,mpark,mlenning,gfedewa,imicthell,farquette,nhayer,vglidden,tkhora,mneubacher,esthill,ecolden,nnamanworth,eklein,pgiegerich,smillian,nmccolm,ameisinger,rtole,jsegundo,jknight,behrke,tguinnip,wlynch,tmorr,omcdaid,dfollman,kmosko,mground,pfavolise,dfirpo,aponcedeleon,wenglander,pduitscher,emehta,lyoula,bmadamba,critchie,gloebs,jscheitlin,tsann,tmalecki,okave,dsherard,wdevenish,dmahapatra,redling,venfort,hstreitnatter,tfetherston,jsenavanh,mmerriwether,pbondroff,tabdelal,badair,bhelverson,jlebouf,tfalconeri,sgefroh,mredd,wselim,ikadar,nrybij,eathey,pschrayter,gmings,xeppley,hrapisura,tdonathan,bcoletta,mdickinson,vdolan,pbiggart,ibyles,kcomparoni,jmatty,psundeen,imarungo,cmcanulty,tmcmickle,obenallack,qhanly,saben,owhitelow,dtornow,btempel,agimm,cpluid,ktoni,rlosinger,fnottage,mfaeth,tmurata,fcunard,saycock,mmcchristian,mcasida,kmoesch,kottomaniello,bwynes,emargulis,kbarnthouse,psalesky,mlinardi,fberra,cgaudette,sestergard,afuchs,esheehan,dscheurer,sgropper,jbjorkman,dflore,vbonder,nnickel,klurie,hmateer,lseehafer,cpinela,maustine,zratti,ohove,okveton,mhollings,vrodick,nwescott,mtanzi,ktuner,yschmuff,akraskouskas,lschnorbus,dmcgillen,aziernicki,wleiva,nendicott,kcofrancesco,cmanno,deshmon,adenicola,hlauchaire,mlaverde,kpenale,dmarchizano,pviviani,vemily,agarbett,ohedlund,werrick,imillin,oconerly,wottesen,kmeester,nwiker,nranck,jroman,cspilis,mallmand,yhenriques,nphan,nbuford,nlohmiller,istallcup,hzinda,atollefsrud,spolmer,purquilla,bgavagan,nramones,lnormand,adishaw,jdodge,moser,urosentrance,oclunes,lpeagler,ubieniek,sgirsh,dzurek,hlemon,pwetherwax,wcreggett,kgarced,pthornberry,nmoren,gcukaj,lbuchtel,dcaltabiano,ibuzo,akomsthoeft,upellam,ptraweek,abortignon,ralspach,pcaposole,hcintron,cbartnick,vnery,lfarraj,pwhitmire,kpannunzio,vfeigel,lpintor,tlowers,fsplinter,rfassinger,ofelcher,csever,oolivarez,kbrevitz,ctuzzo,owhelchel,ptoenjes,mskeele,lschenkelberg,tsablea,hloftis,cbelardo,ycerasoli,gmoen,obercier,cfleurantin,hbraim,ihoa,ochasten,fsavela,zborgmeyer,sbemo,mcolehour,vtrumpp,lgandee,atonkin,rpinilla,hsweezer,hwestermark,lbanco,bwinterton,hcowles,ninnella,ehathcock,uholecek,alamour,bguthary,mdimaio,lsous,ecelestin,ademosthenes,ncermeno,vkrug,ngiesler,pdauterman,achhor,hpimpare,epeterson,lfichtner,tgelen,pdischinger,nlatchaw,psabado,ecordas,dpebbles,ckistenmacher,oscarpello,hschelb,nridinger,tvehrs,lpondexter,rgramby,ocalleo,imuehl,istarring,teliades,ctenny,kstachurski,ugreenberg,cpaccione,cgaler,mmattu,opeet,sstough,dlablue,mespinel,sbloise,ohearl,cbrom,krahman,ysnock,vlubic,rmandril,eserrett,gshrode,ksollitto,ilawbaugh,jappleyard,pbascom,rnordby
EOM

check "getent group hugegroup" << EOM
hugegroup:*:1006:amccroskey,erathert,rrasual,mlinak,psiroky,ichewning,dtuholski,yautin,denriquez,yolivier,tnitzel,kmuros,ppedraja,mrizer,jsweezy,nriofrio,joligee,klitehiser,emcquiddy,gallanson,dbertels,tcossa,hhagee,blovig,ebattee,khartness,nforti,kfend,sgunder,wesguerra,yduft,jzych,edrinkwater,esonia,pphuaphes,ualway,tmysinger,tnaillon,ygockel,sbettridge,clapenta,igizzi,svogler,pbrentano,emanikowski,uwalpole,kwinterling,ghumbles,lparrish,ewilles,oebrani,gdrilling,wtruman,ggillim,phyer,hholyfield,epoinelli,nagerton,wbrill,bswantak,bdadds,vstirman,hbukovsky,lgadomski,sskyers,ddeguire,ekalfas,tbagne,yeven,rdubs,wvalcin,mdoering,rfidel,hkippes,lmichaud,vburton,charriman,hkarney,mswogger,klundsten,nciucci,rpastorin,tcacal,rramirez,thelfritz,hschoepfer,sdebry,vbaldasaro,asivley,vpender,akravetz,llarmore,vmaynard,lmcgeary,rheinzmann,kthede,gcummer,opoch,akertzman,ngrowney,lsobrino,hveader,jspohn,cabare,hrenart,sbrabyn,ohatto,hbrandow,dhammontree,kwidrick,ascovel,jskafec,uslavinski,imcbay,wclokecloak,cflenner,hbastidos,lcaudell,gcarlini,opuglisi,nbugtong,hbetterman,lshilling,nfunchess,nlainhart,kconkey,ktuccio,mcontreras,dasiedu,cbotdorf,rchevrette,mgavet,hchaviano,zwinterbottom,fthein,zculp,bdominga,dlargo,hbickford,lrandall,ykimbel,lautovino,cfasone,hdoiel,ediga,hmatonak,fmilsaps,amckinney,mquigg,mvanpelt,daubert,dgiacomazzi,hhysong,svielle,zanderlik,mpizzaro,bromano,kmarzili,uweyand,smullowney,rbernhagen,ajaquess,ekeuper,lbove,greiff,uransford,ewicks,cpentreath,kepps,uhayakawa,tmccamish,rdubuisson,dtashjian,ibreitbart,ffigert,ycostaneda,kmedcaf,fgrashot,tredfearn,nedgin,mrydelek,tsowells,ilamberth,hhartranft,dsharr,oport,areid,bbeckfield,bluellen,fagro,ihegener,sackles,fparness,lvaleriano,faleo,fbielecki,jeuresti,lcavez,nerbach,tschnepel,zkutchera,limbrogno,nkubley,afredin,gwaud,bmoling,rschkade,kfaure,vtresch,ekurter,estockwin,rgoonez,erostad,nrysavy,hhaffey,apurdon,llasher,jholzmiller,ashuey,rbillingsly,osaber,asemons,edurick,tgindhart,svongal,mvedder,jvillaire,dholdaway,bbrenton,lpitek,jjumalon,kjoslyn,cparee,cklem,mcoch,kmcardle,dsteever,nlemma,rfauerbach,wnunziata,fsirianni,dciullo,udatu,cjody,mvas,hkinderknecht,cpencil,rmcstay,tboxx,brodgerson,mfeil,eberkman,gdeblasio,hspiry,ilevian,wdagrella,bharnois,sscheiern,vbigalow,nschmig,pwohlenhaus,uflander,ckodish,amcgraw,cswigert,mcampagnone,inarain,kmcguire,tharr,bdaughenbaugh,garchambeault,bmarszalek,pvirelli,snotari,nspolar,skanjirathinga,fsunderland,mmesidor,lmuehlberger,glafontaine,aferge,hcarrizal,pdurando,gdeyarmond,fmarchi,wstjean,obeaufait,nslaby,dlongbotham,tplatko,jcaroll,isplonskowski,zscammahorn,sstuemke,cnoriego,nsiemonsma,lseabold,cmafnas,dhendon,bfishbeck,gkerens,eklunder,fburrough,ebusk,tmarkus,clouder,cweiss,mpellew,ojerabek,veisenhardt,vwokwicz,tvrooman,rpitter,slerew,dwittlinger,habby,mpanahon,rguinane,zneeb,eyounglas,gcervantez,kbrugal,ycobetto,tkeala,pheathcock,cmellberg,hmiazga,bmicklos,bphou,ngullett,jwinterton,lcremer,jmartha,icoard,ahandy,eparham,gtinnel,wganther,umarbury,fhalon,bsibal,uschweyen,gearnshaw,cbleimehl,omasone,cdeckard,ctetteh,arosel,pmineo,gclapham,jamber,sbonnie,eaguire,jmarugg,ihalford,wdovey,sarndt,gbitar,ovibbert,ewismer,gmilian,rginer,gdaub,showe,hlynema,rtooker,svandewalle,fhain,jlunney,jreigh,kmandolfo,leberhardt,wkhazaleh,nasmar,egrago,ablackstock,lcocherell,pvierthaler,vrunyon,kpalka,ubenken,hmuscaro,jherkenratt,pminnis,bscadden,srubenfield,cnabzdyk,mpytko,gchounlapane,pwademan,nousdahl,pcornn,zmeeker,hpalmquist,jrees,mkofoed,mkibler,lbassin,fplayfair,hmogush,nvyhnal,ileaman,gschaumburg,thoch,wconces,hliverman,gmackinder,rbrisby,isowder,rkraszewski,hzagami,obihl,nhelfinstine,mbravata,thynson,vwaltmann,tlana,ggehrke,pwutzke,zbuscaglia,ewuitschick,hgalavis,ddigerolamo,wmendell,etunby,jkimpton,mheilbrun,laksamit,hvannette,jseen,sgurski,iroiger,lcanestrini,baigner,dminozzi,uazatyan,gjankowiak,bstrede,mstirn,hfludd,mdyce,tbattista,gfaire,gapkin,esproull,gcurnutt,tstalworth,ienglert,hbrehmer,csoomaroo,kaanerud,nlinarez,jeverton,uspittler,prowena,gsantella,oreiss,rcheshier,tpaa,kwirght,gparkersmith,jquicksall,xrahaim,vwisinger,aesbensen,eorsten,imensah,omalvaez,dnegri,wmailey,tyounglas,vtowell,pgrybel,lmauracher,lschollmeier,ithum,umosser,pbeckerdite,hsabol,dhindsman,ugerpheide,gconver,lhuggler,amanganelli,omatula,zhaulk,lkimel,mruppel,egospatrick,kseisler,ehindbaugh,mdecourcey,kbartolet,vcrofton,cdegravelle,ksiering,fvallian,kalguire,dblazejewski,vdesir,tairth,hcusta,mjeon,smccaie,hpolintan,ihimmelwright,fbeatrice,yvdberg,uednilao,vmedici,sskone,dbarriball,ndrumgole,ccyganiewicz,cdrumm,usevera,vsefcovic,mfitzherbert,fberyman,upater,vpiraino,pwashuk,kshippy,bcolorado,cbarlup,cmiramon,kdevincent,mcaram,cbourek,hkohlmeyer,lringuette,lgradilla,slaningham,ksparling,tcrissinger,senrico,dlanois,iyorks,gbolay,rpikes,hcafourek,shaith,fverfaille,btheim,iambrosino,ghann,fkeef,tsearle,tsepulueda,iherrarte,fvinal,sherzberg,iiffert,astrunk,ghelderman,moller,gmassi,oahyou,cjuntunen,mvanbergen,tkelly,eziebert,nhija,sjankauskas,pdech,mmangiamele,clewicki,meconomides,tmccaffity,carguellez,prepasky,amaslyn,kmallach,ejeppesen,hwoodert,dgivliani,nglathar,fwidhalm,kheadlon,ihernan,oshough,nevan,mpilon,mviverette,beon,alat,ktriblett,ivanschaack,vnazzal,lwedner,alienhard,slaudeman,cpalmios,gishii,kpuebla,ascheno,ocrabbs,dledenbach,ebeachem,ideveyra,sspagnuolo,fsymmonds,srees,isteinlicht,bveeneman,myokoyama,agordner,xlantey,broher,bpinedo,psharits,iweibe,nchrisman,htomlinson,cdickes,draymundo,jbielicki,ulanigan,ihanneman,ppeper,ljomes,khovanesian,ibeto,ilacourse,iseipel,iogasawara,jglotzbecker,mferandez,gpomerance,pdulac,mgayden,skoegler,kbattershell,uvanmatre,wvermeulen,ekenady,ikulbida,htsuha,lvanconant,njordon,oosterhouse,tmelland,lspielvogel,bmarlin,bouten,fgoben,bjolly,iyorgey,htilzer,dgosser,gcobane,vpeairs,dloubier,zfarler,fvascones,awhitt,cscullion,nkempon,rgriffies,wconstantino,opizzuti,scocuzza,pgreenier,ueriks,cwank,mdanos,kmisove,ndesautels,hlichota,cgalinol,rlambertus,zvagt,ohoffert,vchevalier,vwabasha,amayorga,mtintle,rbloomstrand,swoodie,gportolese,hriech,ckerska,gvollrath,bdevera,lmadruga,mbeagley,hdyner,fcha,rlatessa,lsivic,mdedon,mcashett,ubynum,lcoulon,cbrechbill,kgremminger,yfrymoyer,pahles,guresti,kmayoras,mbodley,phalkett,kolexa,fsapien,cghianni,oalthouse,mpark,mlenning,gfedewa,imicthell,farquette,nhayer,vglidden,tkhora,mneubacher,esthill,ecolden,nnamanworth,eklein,pgiegerich,smillian,nmccolm,ameisinger,rtole,jsegundo,jknight,behrke,tguinnip,wlynch,tmorr,omcdaid,dfollman,kmosko,mground,pfavolise,dfirpo,aponcedeleon,wenglander,pduitscher,emehta,lyoula,bmadamba,critchie,gloebs,jscheitlin,tsann,tmalecki,okave,dsherard,wdevenish,dmahapatra,redling,venfort,hstreitnatter,tfetherston,jsenavanh,mmerriwether,pbondroff,tabdelal,badair,bhelverson,jlebouf,tfalconeri,sgefroh,mredd,wselim,ikadar,nrybij,eathey,pschrayter,gmings,xeppley,hrapisura,tdonathan,bcoletta,mdickinson,vdolan,pbiggart,ibyles,kcomparoni,jmatty,psundeen,imarungo,cmcanulty,tmcmickle,obenallack,qhanly,saben,owhitelow,dtornow,btempel,agimm,cpluid,ktoni,rlosinger,fnottage,mfaeth,tmurata,fcunard,saycock,mmcchristian,mcasida,kmoesch,kottomaniello,bwynes,emargulis,kbarnthouse,psalesky,mlinardi,fberra,cgaudette,sestergard,afuchs,esheehan,dscheurer,sgropper,jbjorkman,dflore,vbonder,nnickel,klurie,hmateer,lseehafer,cpinela,maustine,zratti,ohove,okveton,mhollings,vrodick,nwescott,mtanzi,ktuner,yschmuff,akraskouskas,lschnorbus,dmcgillen,aziernicki,wleiva,nendicott,kcofrancesco,cmanno,deshmon,adenicola,hlauchaire,mlaverde,kpenale,dmarchizano,pviviani,vemily,agarbett,ohedlund,werrick,imillin,oconerly,wottesen,kmeester,nwiker,nranck,jroman,cspilis,mallmand,yhenriques,nphan,nbuford,nlohmiller,istallcup,hzinda,atollefsrud,spolmer,purquilla,bgavagan,nramones,lnormand,adishaw,jdodge,moser,urosentrance,oclunes,lpeagler,ubieniek,sgirsh,dzurek,hlemon,pwetherwax,wcreggett,kgarced,pthornberry,nmoren,gcukaj,lbuchtel,dcaltabiano,ibuzo,akomsthoeft,upellam,ptraweek,abortignon,ralspach,pcaposole,hcintron,cbartnick,vnery,lfarraj,pwhitmire,kpannunzio,vfeigel,lpintor,tlowers,fsplinter,rfassinger,ofelcher,csever,oolivarez,kbrevitz,ctuzzo,owhelchel,ptoenjes,mskeele,lschenkelberg,tsablea,hloftis,cbelardo,ycerasoli,gmoen,obercier,cfleurantin,hbraim,ihoa,ochasten,fsavela,zborgmeyer,sbemo,mcolehour,vtrumpp,lgandee,atonkin,rpinilla,hsweezer,hwestermark,lbanco,bwinterton,hcowles,ninnella,ehathcock,uholecek,alamour,bguthary,mdimaio,lsous,ecelestin,ademosthenes,ncermeno,vkrug,ngiesler,pdauterman,achhor,hpimpare,epeterson,lfichtner,tgelen,pdischinger,nlatchaw,psabado,ecordas,dpebbles,ckistenmacher,oscarpello,hschelb,nridinger,tvehrs,lpondexter,rgramby,ocalleo,imuehl,istarring,teliades,ctenny,kstachurski,ugreenberg,cpaccione,cgaler,mmattu,opeet,sstough,dlablue,mespinel,sbloise,ohearl,cbrom,krahman,ysnock,vlubic,rmandril,eserrett,gshrode,ksollitto,ilawbaugh,jappleyard,pbascom,rnordby
EOM

###########################################################################

echo "test_nsscmds.sh: testing hosts..."

check "getent hosts testhost" << EOM
10.0.0.1        testhost testhostalias
EOM

check "getent hosts testhostalias" << EOM
10.0.0.1        testhost testhostalias
EOM

check "getent hosts 10.0.0.1" << EOM
10.0.0.1        testhost testhostalias
EOM

check "getent hosts | grep testhost" << EOM
10.0.0.1        testhost testhostalias
EOM

# dummy test for IPv6 envoronment
check "getent hosts ::1" << EOM
::1             ip6-localhost ip6-loopback
EOM

# TODO: add more tests for IPv6 support

###########################################################################

echo "test_nsscmds.sh: testing netgroup..."

# check netgroup lookup of test netgroup
check "getent netgroup tstnetgroup" << EOM
tstnetgroup          (aap, , ) (noot, , )
EOM

###########################################################################

echo "test_nsscmds.sh: testing networks..."

check "getent networks testnet" << EOM
testnet               10.0.0.0
EOM

check "getent networks 10.0.0.0" << EOM
testnet               10.0.0.0
EOM

check "getent networks | grep testnet" << EOM
testnet               10.0.0.0
EOM

###########################################################################

echo "test_nsscmds.sh: testing passwd..."

check "getent passwd ecolden" << EOM
ecolden:x:5972:1000:Estelle Colden:/home/ecolden:/bin/bash
EOM

check "getent passwd arthur" << EOM
arthur:x:1000:100:Arthur de Jong:/home/arthur:/bin/bash
EOM

check "getent passwd 4089" << EOM
jguzzetta:x:4089:1000:Josephine Guzzetta:/home/jguzzetta:/bin/bash
EOM

# count the number of passwd entries in the 4000-5999 range
check "getent passwd | grep -c ':x:[45][0-9][0-9][0-9]:'" << EOM
2000
EOM

###########################################################################

echo "test_nsscmds.sh: testing protocols..."

check "getent protocols protfoo" << EOM
protfoo               140 protfooalias
EOM

check "getent protocols protfooalias" << EOM
protfoo               140 protfooalias
EOM

check "getent protocols 140" << EOM
protfoo               140 protfooalias
EOM

check "getent protocols icmp" << EOM
icmp                  1 ICMP
EOM

check "getent protocols | grep protfoo" << EOM
protfoo               140 protfooalias
EOM

###########################################################################

echo "test_nsscmds.sh: testing rpc..."

check "getent rpc rpcfoo" << EOM
rpcfoo          160002  rpcfooalias
EOM

check "getent rpc rpcfooalias" << EOM
rpcfoo          160002  rpcfooalias
EOM

check "getent rpc 160002" << EOM
rpcfoo          160002  rpcfooalias
EOM

check "getent rpc | grep rpcfoo" << EOM
rpcfoo          160002  rpcfooalias
EOM

###########################################################################

echo "test_nsscmds.sh: testing services..."

check "getent services foosrv" << EOM
foosrv                15349/tcp
EOM

check "getent services foosrv/tcp" << EOM
foosrv                15349/tcp
EOM

check "getent services foosrv/udp" << EOM
EOM

check "getent services 15349/tcp" << EOM
foosrv                15349/tcp
EOM

check "getent services 15349/udp" << EOM
EOM

check "getent services barsrv" << EOM
barsrv                15350/tcp
EOM

check "getent services barsrv/tcp" << EOM
barsrv                15350/tcp
EOM

check "getent services barsrv/udp" << EOM
barsrv                15350/udp
EOM

check "getent services | egrep '(foo|bar)srv' | sort" << EOM
barsrv                15350/tcp
barsrv                15350/udp
foosrv                15349/tcp
EOM

check "getent services | wc -l" << EOM
`grep -c '^[^#].' /etc/services | awk '{print $1 + 3}'`
EOM

###########################################################################

echo "test_nsscmds.sh: testing shadow..."

# NOTE: the output of this should depend on whether we are root or not

check "getent shadow ecordas" << EOM
ecordas:*::::7:2::0
EOM

check "getent shadow arthur" << EOM
arthur:*::100:200:7:2::0
EOM

# check if the number of passwd entries matches the number of shadow entries
check "getent shadow | wc -l" << EOM
`getent passwd | wc -l`
EOM

# check if the names of users match between passwd and shadow
getent passwd | sed 's/:.*//' | sort | \
  check "getent shadow | sed 's/:.*//' | sort"

###########################################################################
# determine the result

if [ $FAIL -eq 0 ]
then
  echo "test_nsscmds.sh: all tests passed"
  exit 0
else
  echo "test_nsscmds.sh: $FAIL TESTS FAILED"
  exit 1
fi
