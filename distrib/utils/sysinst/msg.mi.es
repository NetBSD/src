/*	$NetBSD: msg.mi.es,v 1.35.4.2 2012/05/23 10:07:19 yamt Exp $	*/

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

/* MI Message catalog -- spanish, machine independent */

message usage
{uso: sysinst [-r versi�n] [-f fichero-definici�n]
}

/*
 * We can not use non ascii characters in this message - it is displayed
 * before the locale is set up!
 */
message sysinst_message_language
{Mensajes de instalacion en castellano}

message sysinst_message_locale
{es_ES.ISO8859-15}

message Yes {S�}
message No {No}
message All {Todo}
message Some {Algunos}
message None {Ninguno}
message none {ninguno}
message OK {OK}
message ok {ok}
message On {Activado}
message Off {Desactivado}
message unchanged {sin cambios}
message Delete {�Borrar?}

message install
{instalar}

message reinstall
{reinstalar conjuntos para}

message upgrade
{actualizar}

message hello
{Bienvenido a sysinst, la herramienta de instalaci�n de NetBSD-@@VERSION@@.
Esta herramienta guiada por men�s est� dise�ada para ayudarle a instalar
NetBSD en un disco duro, o actualizar un sistema NetBSD existente con
un trabajo m�nimo. 
En los siguientes men�s teclee la letra de referencia (a, b, c, ...) para
seleccionar una opci�n, o teclee CTRL+N/CTRL+P para seleccionar la opci�n
siguiente/anterior. 
Las teclas de cursor y AvP�g/ReP�g puede que tambi�n funcionen. 
Active la selecci�n actual desde el men� pulsando la tecla Intro.
}

message thanks
{�Gracias por usar NetBSD!
}

message installusure
{Ha escogido instalar NetBSD en su disco duro.  Esto cambiar� informaci�n
de su disco duro.  �Deber�a haber hecho una copia de seguridad completa
antes de este procedimiento!  Este procedimiento realizar� las siguientes
operaciones:
	a) Particionar su disco
	b) Crear nuevos sistemas de ficheros BSD
	c) Cargar e instalar los conjuntos de distribuci�n
	d) Algunas configuraciones iniciales del sistema

(Despu�s de introducir la informaci�n de las particiones pero antes de que
su disco sea cambiado, tendr� la oportunidad de salir del programa.)

�Desea continuar?
}

message upgradeusure
{Se va a actualizar NetBSD en su disco duro.  Sin embargo, esto
cambiar� informaci�n de su disco duro.  �Deber�a hacer una copia de seguridad
completa antes de este procedimiento!  �Realmente desea actualizar NetBSD?
(�ste es su �ltimo aviso antes de que el programa empiece a modificar
sus discos.)
}

message reinstallusure
{Se va a desempaquetar los conjuntos de distribuci�n de NetBSD
a un disco duro marcado como arrancable.  Este procedimiento solamente
descarga y desempaqueta los conjuntos en un disco arrancable preparticionado.
No pone nombre a los discos, ni actualiza los bloques de arranque, ni guarda
ninguna informaci�n de configuraci�n.  (Salga y escoja `instalar' o
`actualizar' si quiere esas opciones.)  �Ya deber�a haber hecho un
`instalar' o `actualizar' antes de iniciar este procedimiento!

�Realmente quiere reinstalar los conjuntos de la distribuci�n NetBSD?
(�ste es su �ltimo aviso antes de que el programa empiece a modificar
sus discos.)
}

message mount_failed
{Mounting %s failed. Continue?
}

message nodisk
{No se ha podido encontrar ning�n disco duro para ser usado por NetBSD.
Se le volver� a llevar al men� original.
}

message onedisk
{Solamente se ha encontrado un disco, %s. 
Por tanto se entiende que quiere %s NetBSD en �l.
}

message ask_disk
{�En cu�l disco quiere %s NetBSD? }

message Available_disks
{Discos disponibles}

message heads
{cabezas}

message sectors
{sectores}

message fs_isize
{tama�o promedio del fichero (bytes)}

message mountpoint
{punto de montaje (o 'ninguno')}

message cylname
{cil}

message secname
{sec}

message megname
{MB}

message layout
{NetBSD usa una etiqueta de BSD para dividir la porci�n NetBSD del disco
en varias particiones BSD.  Ahora deber�a configurar su etiqueta BSD.

Puede usar un simple editor para establecer los tama�os de las particiones
NetBSD, o mantener los tama�os de partici�n y contenidos actuales.

Despu�s se la dar� la oportunidad de cambiar cualquier campo de la
etiqueta.

La parte NetBSD de su disco es de %d megabytes.
Una instalaci�n completa requiere al menos %d megabytes sin X y
al menos %d megabytes si se incluyen los conjuntos de X.
}

message Choose_your_size_specifier
{Seleccionar megabytes producir� tama�os de partici�n cercanos a su
elecci�n, pero alineados a los limites de los cilindros.
Seleccionar sectores le permitir� especificar los tama�os de manera
m�s precisa.  En discos ZBR modernos, el tama�o real del cilindro var�a
a lo largo del disco y no hay mucha ventaja real en el alineamiento de
cilindros.  En discos m�s viejos, lo m�s eficiente es seleccionar
tama�os de partici�n que sean multiples exactos del tama�o real del 
cilindro.

Escoja su especificador de tama�o}

message ptnsizes
{Ahora puede cambiar los tama�os de las particiones del sistema.  Por
omisi�n se asigna todo el espacio al sistema de archivos ra�z, sin embargo
usted podr�a querer separar /usr (ficheros de sistema adicionales), /var
(ficheros de registro, etc) o /home (directorios de usuario).

El espacio libre sobrante ser� a�adido a la partici�n marcada con �+�.
}

message ptnheaders
{
       MB         Cilindros	Sectores  Sistema de archivos
}

message askfsmount
{�Punto de montaje?}

message askfssize
{�Tama�o de %s en %s?}

message askunits
{Cambiar las unidades de entrada (sectores/cilindros/MB)}

message NetBSD_partition_cant_change
{Partici�n NetBSD}

message Whole_disk_cant_change
{Todo el disco}

message Boot_partition_cant_change
{Partici�n de arranque}

message add_another_ptn
{A�adir una partici�n definida por el usuario}

message fssizesok
{Aceptar los tama�os de las particiones.  Espacio libre %d %s, %d particiones libres.}

message fssizesbad
{Reducir los tama�os de las particiones en %d %s (%u sectores).}

message startoutsidedisk
{El valor del comienzo que ha especificado est� mas all� del final del disco.
}

message endoutsidedisk
{Con este valor, el final de la partici�n est� mas all� del final del disco.
El tama�o de la partici�n se ha truncado a %d %s.

Presione Intro para continuar
}

message toobigdisklabel
{
This disk is too large for a disklabel partition table to be used
and hence cannot be used as a bootable disk or to hold the root
partition.
}

message fspart
{Sus particiones con etiquetas BSD est�n ahora as�.
�sta es su �ltima oportunidad para cambiarlas.

}

message fspart_header
{   Inicio %3s Fin %3s   Tama�o %3s Tipo FS    Newfs Mont. Punto mont. 
   ---------- --------- ---------- ---------- ----- ----- -----------
}

message fspart_row
{%10lu %9lu %10lu %-10s %-5s %-5s %s}

message show_all_unused_partitions
{Mostrar todas las particiones no usadas}

message partition_sizes_ok
{Tama�os de partici�n ok}

message edfspart
{Los valores actuales de la particion `%c' son, 
Seleccione el campo que desee cambiar:

                          MB cilindros  sectores
	             ------- --------- ---------
}

message fstype_fmt
{        FStype: %9s}

message start_fmt
{        inicio: %9u %8u%c %9u}

message size_fmt
{        tama�o: %9u %8u%c %9u}

message end_fmt
{           fin: %9u %8u%c %9u}

message bsize_fmt
{tama�o  bloque: %9d bytes}

message fsize_fmt
{   tama�o frag: %9d bytes}

message isize_fmt
{tam prom archi: %9d bytes (para n�mero de inodos)}
message isize_fmt_dflt
{tam prom archi:         4 fragmentos}

message newfs_fmt
{         newfs: %9s}

message mount_fmt
{        montar: %9s}

message mount_options_fmt
{   opc montaje: }

message mountpt_fmt
{ punto montaje: %9s}

message toggle
{Conmutar}

message restore
{Restaurar los valores originales}

message Select_the_type
{Seleccione el tipo}

message other_types
{otros tipos}

message label_size
{%s
Valores especiales que se pueden introducir para el valor del tama�o:
    -1:   usar hasta el final de la parte NetBSD del disco
   a-%c:   terminar esta partici�n donde empieza la partici�n X

tama�o (%s)}

message label_offset
{%s
Valores especiales que se pueden introducir para el valor del desplazamiento:
    -1:   empezar al principio de la parte NetBSD del disco
   a-%c:   empezar al final de la partici�n anterior (a, b, ..., %c)

inicio (%s)}

message invalid_sector_number
{N�mero de sector mal formado
}

message Select_file_system_block_size
{Seleccione el tama�o de bloque del sistema de archivos}

message Select_file_system_fragment_size
{Seleccione el tama�o de fragmento del sistema de archivos}

message packname
{Por favor entroduzca un nombre para el disco NetBSD}

message lastchance
{Bien, todo est� preparado para instalar NetBSD en su disco duro (%s).
Todav�a no se ha escrito nada.  �sta es su �ltima oportunidad para salir
del proceso antes de que se cambie nada.

�Desea continuar?
}

message disksetupdone
{De acuerdo, la primera parte del procedimiento ha terminado.
Sysinst ha escrito una etiqueta en el disco objetivo, y ha
formateado y comprobado las nuevas particiones que ha indicado
para el disco objetivo.
}

message disksetupdoneupdate
{De acuerdo, la primera parte del procedimiento ha terminado.
Sysinst ha escrito una etiqueta en el disco objetivo, y
comprobdo las nuevas particiones que ha indicado para el disco objetivo.
}

message openfail
{No se ha podido abrir %s, el mensaje de error ha sido: %s.
}

message mountfail
{el montaje del dispositivo /dev/%s%c en %s ha fallado.
}

message extractcomplete
{La extracci�n de los conjuntos seleccionados para NetBSD-@@VERSION@@ ha
finalizado.  El sistema es ahora capaz de arrancar desde el disco duro
seleccionado.  Para completar la instalaci�n, sysinst le dar� la
oportunidad de configurar antes algunos aspectos esenciales.
}

message instcomplete
{La instalaci�n de NetBSD-@@VERSION@@ ha finalizado.  El sistema deber�a
arrancar desde el disco duro.  Siga las instrucciones del documento
INSTALL sobre la configuraci�n final del sistema.  La pagina de manual
de afterboot(8) es otra lectura recomendada; contiene una lista de
cosas a comprobar desp�es del primer arranque completo.

Como m�nimo, debe editar /etc/rc.conf para que cumpla sus necesidades.
Vea en /etc/defaults/rc.conf los valores predefinidos.
}

message upgrcomplete
{Actualizaci�n a NetBSD-@@VERSION@@ completada.  Ahora tendr� que
seguir las instrucciones del documento INSTALL para hacer lo que
necesite para conseguir tener el sistema configurado para su
situaci�n.
Recuerde (re)leer la p�gina de manual de afterboot(8), ya que puede
contener nuevos apartados desde su ultima actualizaci�n.

Como m�nimo, debe editar rc.conf para su entorno local y cambiar
rc_configured=NO a rc_configured=YES o los reinicios se detendr�n en
single-user, y copie de nuevo los ficheros de contrase�as (teniendo en
cuenta las nuevas cuentas de sistema que se hayan podido crear para esta
versi�n) si estuviera usando ficheros de contrase�as locales.
}


message unpackcomplete
{Finalizado el desempaquetamiento de conjuntos adicionales de NetBSD-@@VERSION@@.
Ahora necesitar� seguir las instrucciones del documento INSTALL para
tener su sistema reconfigurado para su situaci�n.
La p�gina de manual de afterboot(8) tambi�n puede serle de ayuda.

Como m�nimo, debe editar rc.conf para su entorno local, y cambiar
rd_configure=NO a rc_configured=YES, o los reinicios se detendr�n
en modo de usuario �nico.
}

message distmedium
{Su disco est� ahora preparado para la instalaci�n el nucleo y los conjuntos
de la distribuci�n.  Como se apunta en las notas INSTALL, tiene diversas
opciones.  Para ftp o nfs, tiene que estar conectado a una red con acceso
a las maquinas apropiadas.

Conjuntos seleccionados: %d, procesados: %d. Siguiente conjunto: %s.

}

message distset
{La distribuci�n NetBSD est� dividida en una colecci�n de conjuntos de
distribuci�n.  Hay algunos conjuntos b�sicos que son necesarios para todas
las instalaciones, y otros conjuntos que no son necesarios para todas las
instalaciones.  Puede escoger para instalar s�lo los conjuntos esenciales
(instalaci�n m�nima); instalar todos ellos (Instalaci�n completa) o
seleccionar de entre los conjuntos de distribuci�n opcionales.
}

message ftpsource
{Lo siguiente son el sitio %s, directorio, usuario y contrase�a que se
usar�n.  Si �usuario� es �ftp�, no se necesita contrase�a..

}

message email
{direcci�n de correo electr�nico}

message dev
{dispositivo}

message nfssource
{Introduzca el servidor nfs y el directorio del servidor donde se encuentre
la distribuci�n.  Recuerde: el directorio debe contener los archivos .tgz y
debe ser montable por nfs.

}

message floppysource
{Introduzca el dispositivo de disquete a usar y el directorio destino de la
transferencia en el sistema de archivos. Los archivos del conjunto han de estar
en el directorio ra�z de los disquetes.

}

message cdromsource
{Introduzca el dispositivo de CDROM a usar y el directorio del CDROM
donde se encuentre la distribuci�n.
Recuerde, el directorio debe contener los archivos .tgz.

}

message Available_cds
{Available CDs}

message ask_cd
{Multiple CDs found, please select the one containing the install CD.}

message cd_path_not_found
{The installation sets have not been found at the default location on this
CD. Please check device and path name.}

message localfssource
{Introduzca el dispositivo local desmontado y el directorio de ese
dispositivo donde se encuentre la distribuci�n. 
Recuerde, el directorio debe contener los archivos .tgz.

}

message localdir
{Introduzca el directorio local ya montado donde se encuentre la distribuci�n.
Recuerde, el directorio debe contener los archivos .tgz.

}

message filesys
{sistema de archivos}

message nonet
{No se ha podido encontrar ninguna interfaz de red para ser usada por NetBSD.
Se le devolver� al men� anterior.
}

message netup
{Las siguientes interfaces est�n activas: %s
�Conecta alguna de ellas al servidor requerido?}

message asknetdev
{Se han encontrado las siguientes interfaces de red: %s
\n�Cu�l se deber�a usar?}

message badnet
{No ha seleccionado ninguna interfaz de las listadas.  Por favor, vuelva a
intentarlo.
Est�n disponibles las siguientes interfaces de red: %s
\n�Cu�l deber�a usar?}

message netinfo
{Para poder usar la red, necesita responder lo siguiente:

}

message net_domain
{Su dominio DNS}

message net_host
{Su nombre de m�quina}

message net_ip
{Su n�mero IPv4}

message net_srv_ip
{N�mero servidor IPv4}

message net_mask
{M�scara de red IPv4}

message net_namesrv6
{Servidor de nombres IPv6}

message net_namesrv
{Servidor de nombres IPv4}

message net_defroute
{Pasarela IPv4}

message net_media
{Tipo de medio de la red}

message netok
{Ha introducido los siguientes valores.

Dominio DNS: 		%s 
Nombre de m�quina:	%s 
Interfaz primaria:	%s 
IP de la m�quina:	%s 
M�scara de red:		%s 
Serv de nombres IPv4:	%s 
Pasarela IPv4:		%s 
Tipo de medio:		%s 
}

message netok_slip
{Ha introducido los siguientes valores. �Son correctos?

Dominio DNS: 		%s 
Nombre de la m�quina:	%s 
Interfaz primaria:	%s 
IP de la m�quina:	%s 
IP del servidor:	%s 
M�scara de red:		%s 
Serv de nombres IPv4:	%s 
Pasarela IPv4:		%s 
Tipo de medio:		%s 
}

message netokv6
{IPv6 autoconf:		%s 
Serv de nombres IPv6:	%s 
}

message netok_ok
{�Son correctos?}

message netagain
{Por favor, vuelva a introducir la informaci�n sobre su red.  Sus �ltimas
respuestas ser�n las predeterminadas.

}

message wait_network
{
Espere mientras las interfaces de red se levantan.
}

message resolv
{No se ha podido crear /etc/resolv.conf.  Instalaci�n interrumpida.
}

message realdir
{No se ha podido cambiar el directorio a %s: %s.  Instalaci�n
interrumpida.
}

message delete_xfer_file
{A eliminar despu�s de la instalaci�n}

message notarfile
{El conjunto %s no existe.}

message endtarok
{Todos los conjuntos de distribuci�n han sido desempaquetados
correctamente.}

message endtar
{Ha habido problemas al desempaquetar los conjuntos de la distribuci�n.
Su instalaci�n est� incompleta.

Ha seleccionado %d conjuntos de distribuci�n.  %d conjuntos no han sido
encontrados, y %d han sido omitidos despu�s de que ocurriera un
error.  De los %d que se han intentado, %d se han desempaquetado
sin errores y %d con errores.

La instalaci�n ha sido interrumpida.  Por favor, compruebe de nuevo su
fuente de distribuci�n y considere el reinstalar los conjuntos desde
el men� principal.}

message abort
{Sus opciones han hecho imposible instalar NetBSD.  Instalaci�n interrumpida.
}

message abortinst
{La distribuci�n no ha sido cargada correctamente.  Necesitar� proceder
a mano.  Instalaci�n interrumpida.
}

message abortupgr
{La distribuci�n no ha sido correctamente cargada.  Necesitar� proceder
a mano.  Instalaci�n interrumpida.
}

message abortunpack
{El desempaquetamiento de los conjuntos adicionales no ha sido satisfactorio. 
Necesitar� proceder a mano, o escoger una fuente diferente para los conjuntos
de esta distribuci�n y volver a intentarlo.
}

message createfstab
{�Hay un gran problema!  No se puede crear /mnt/etc/fstab.  Desistiendo.
}


message noetcfstab
{�Ayuda!  No hay /etc/fstab del disco objetivo %s.
Interrumpiendo la actualizaci�n.
}

message badetcfstab
{�Ayuda!  No se puede analizar /etc/fstab en el disco objetivo %s.
Interrumpiendo la actualizaci�n.
}

message X_oldexists
{No se puede guardar %s/bin/X como %s/bin/X.old, porque el
disco objetivo ya tiene un %s/bin/X.old.  Por favor, arregle esto
antes de continuar.

Una manera es iniciando una shell desde el men� Utilidades, y examinar
el objetivo %s/bin/X y %s/bin/X.old.  Si
%s/bin/X.old es de una actualizaci�n completada, puede rm -f
%s/bin/X.old y reiniciar.  O si %s/bin/X.old es de
una actualizacion reciente e incompleta, puede rm -f %s/bin/X
y mv %s/bin/X.old a %s/bin/X.

Interrumpiendo la actualizaci�n.}

message netnotup
{Ha habido un problema configurando la red.  O su pasarela o su servidor
de nombres no son alcanzables por un ping.  �Quiere configurar la red
de nuevo?  (�No� le permite continuar de todos modos o interrumpir el
proceso de instalaci�n.)
}

message netnotup_continueanyway
{�Le gustar�a continuar el proceso de instalaci�n de todos modos, y
suponer que la red est� funcionando?  (�No� interrumpe el proceso de
instalaci�n.)
}

message makedev
{Creando nodos de dispositivo ...
}

message badfs
{Parece que /dev/%s%c no es un sistema de archivos BSD o el fsck no ha sido
correcto.  �Continuar?  (Error n�mero %d.)
}

message rootmissing
{ el directorio ra�z objetivo no existe %s.
}

message badroot
{El nuevo sistema de archivos ra�z no ha pasado la comprobaci�n b�sica.
 �Est� seguro de que ha instalado todos los conjuntos requeridos?

}

message fd_type
{Tipo de sistema de archivos del disquete}

message fdnotfound
{No se ha encontrado el fichero en el disco.
}

message fdremount
{El disquete no ha sido montado correctamente.
}

message fdmount
{Por favor, inserte el disquete que contiene el fichero �%s.%s�.

Si el conjunto no est� en m�s discos, seleccione "Conjunto finalizado"
para instalarlo. Seleccione "Abortar lectura" para regresar al men�
de selecci�n de medios de instalaci�n.
}

message mntnetconfig
{La informaci�n de red que ha introducido, �es la adecuada para el
funcionamiento normal de esta m�quina y desea instalarla en /etc? }

message cur_distsets
{�sta es la lista de conjuntos de distribuci�n que se usar�.

}

message cur_distsets_header
{   Conjunto de distribuci�n Selecc.
   ------------------------ --------
}

message set_base
{Base}

message set_system
{Sistema (/etc)}

message set_compiler
{Herramientas del compilador}

message set_games
{Juegos}

message set_man_pages
{Paginas de manual}

message set_misc
{Varios}

message set_modules
{Kernel Modules}

message set_tests
{Programas de prueba}

message set_text_tools
{Herramientas de procesamiento de textos}

message set_X11
{Conjuntos de X11}

message set_X11_base
{X11 base y clientes}

message set_X11_etc
{Configuraci�n de X11}

message set_X11_fonts
{Tipos de letra de X11}

message set_X11_servers
{Servidores X11}

message set_X11_prog
{Programaci�n de X11}

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

message cur_distsets_row
{%-27s %3s}

message select_all
{Seleccionar todos los conjuntos anteriores}

message select_none
{No seleccionar ninguno de los conjuntos anteriores}

message install_selected_sets
{Instalar los conjuntos seleccionados}

message tarerror
{Ha habido un error al extraer el fichero %s.  Esto significa
que algunos ficheros no han sido extraidos correctamente y su sistema
no estar� completo.

�Continuar extrayendo conjuntos?}

message must_be_one_root
{Debe haber una sola partici�n marcada para ser montada en �/�.}

message partitions_overlap
{las particiones %c y %c se solapan.}

message No_Bootcode
{No hay c�digo de arranque para la partici�n root}

message cannot_ufs2_root
{Sorry, the root file system can't be FFSv2 due to lack of bootloader support
on this port.}

message edit_partitions_again
{

Puede editar la tabla de particiones a mano, o dejarlo y retornar al
men� principal.

�Editar la tabla de particiones de nuevo?}

message config_open_error
{No se ha podido abrir el fichero de configuraci�n %s\n}

message choose_timezone
{Por favor, escoja de la siguiente lista la zona horaria que le convenga.  
Presione RETURN para seleccionar una entrada. 
Presione �x� seguido de RETURN para salir de la selecci�n de la
zona horaria.

 Predefinida:	%s 
 Seleccionada:	%s 
 Hora local: 	%s %s 
}

message tz_back
{ Volver a la lista principal de zonas horarias}

message swapactive
{El disco que ha seleccionado tiene una partici�n de intercambio (swap) que
puede que est� en uso actualmente si su sistema tiene poca memoria.  Como
se dispone a reparticionar este disco, esta partici�n swap ser� desactivada
ahora.  Se advierte de que esto puede conducir a problemas de swap.
Si obtuviera algun error, reinicie el sistema e int�ntelo de nuevo.}

message swapdelfailed
{Sysinst ha fallado en la desactivaci�n de la partici�n swap del disco que
ha escogido para la instalaci�n.  Por favor, reinicie e int�ntelo de nuevo.}

message rootpw
{La contrase�a de root del nuevo sistema instalado no ha sido asignada todav�a,
y por tanto est� vac�a.  �Quiere establecer ahora una contrase�a de root para
el sistema?}

message rootsh
{Ahora puede seleccionar que shell quiere usar para el usuario root.  Por
omisi�n es /bin/sh, pero podr�a preferir otra.}

message no_root_fs
{
No hay un sistema de archivos ra�z definido.  Necesitar� al menos un punto
de montaje con �/�.

Presione <return> para continuar.
}

message slattach {
Introduzca los par�metros de slattach
}

message Pick_an_option {Seleccione una opci�n para activar/desactivar.}
message Scripting {Scripting}
message Logging {Logging}

message Status  { Estado: }
message Command {Orden: }
message Running {Ejecutando}
message Finished {Acabado   }
message Command_failed {Orden fallida}
message Command_ended_on_signal {Orden terminada en se�al}

message NetBSD_VERSION_Install_System {Sistema de instalaci�n de NetBSD-@@VERSION@@}
message Exit_Install_System {Salir del sistema de instalaci�n}
message Install_NetBSD_to_hard_disk {Instalar NetBSD en el disco duro}
message Upgrade_NetBSD_on_a_hard_disk {Actualizar NetBSD en un disco duro}
message Re_install_sets_or_install_additional_sets {Reinstalar conjuntos o instalar conjuntos adicionales}
message Reboot_the_computer {Reiniciar la computadora}
message Utility_menu {Men� de utilidades}
message exit_utility_menu {Exit}
message NetBSD_VERSION_Utilities {Utilidades de NetBSD-@@VERSION@@}
message Run_bin_sh {Ejecutar /bin/sh}
message Set_timezone {Establecer la zona horaria}
message Configure_network {Configurar la red}
message Partition_a_disk {Particionar un disco}
message Logging_functions {Funciones de registro}
message Halt_the_system {Detener el sistema}
message yes_or_no {�s� o no?}
message Hit_enter_to_continue {Presione intro para continuar}
message Choose_your_installation {Seleccione su instalaci�n}
message Set_Sizes {Establecer los tama�os de las particiones NetBSD}
message Use_Existing {Usar los tama�os de las particiones existentes}
message Megabytes {Megabytes}
message Cylinders {Cilindros}
message Sectors {Sectores}
message Select_medium {Seleccione el medio}
message ftp {FTP}
message http {HTTP}
message nfs {NFS}
message cdrom {CD-ROM / DVD}
message floppy {Disquete}
message local_fs {Sistema de archivos desmontado}
message local_dir {Directorio Local}
message Select_your_distribution {Seleccione su distribuci�n}
message Full_installation {Instalaci�n completa}
message Full_installation_nox {Instalaci�n sin X11}
message Minimal_installation {Instalaci�n m�nima}
message Custom_installation {Instalaci�n personalizada}
message hidden {** oculto **}
message Host {M�quina}
message Base_dir {Directorio base}
message Set_dir_src {Directorio de conjuntos binary} /* fix XLAT */
message Set_dir_bin {Directorio de conjuntos source} /* fix XLAT */
message Xfer_dir {Directorio a transferir a}
message User {Usuario}
message Password {Contrase�a}
message Proxy {Proxy}
message Get_Distribution {Obtener la distribuci�n}
message Continue {Continuar}
message What_do_you_want_to_do {�Qu� desea hacer?}
message Try_again {Reintentar}
message Set_finished {Conjunto finalizado}
message Skip_set {Omitir conjunto}
message Skip_group {Omitir grupo de conjuntos}
message Abandon {Abandonar instalaci�n}
message Abort_fetch {Abortar lectura}
message Device {Dispositivo}
message File_system {Sistema de archivos}
message Select_IPv6_DNS_server {  Seleccione servidor DNS de IPv6}
message other {otro }
message Perform_IPv6_autoconfiguration {�Realizar autoconfiguraci�n IPv6?}
message Perform_DHCP_autoconfiguration {�Realizar autoconfiguraci�n DHCP ?}
message Root_shell {Shell de root}

.if AOUT2ELF
message aoutfail
{No se ha podido crear el directorio al que deber�an moverse las bibliotecas
compartidas a.out antiguas.  Por favor, intente el proceso de actualizaci�n
de nuevo y aseg�rese de que ha montado todos los sistemas de ficheros.}

message emulbackup
{El directorio /emul/aout o /emul de su sistema era un enlace simb�lico que
apuntaba a un sistema de archivos desmontado.  Se le ha dado la extension
'.old'.  Cuando vuelva a arrancar su sistema actualizado, puede que necesite
preocuparse de fundir el directorio /emul/aout nuevamente creado con el viejo.
}
.endif

.if xxxx
Si no est� preparado para completar la
instalaci�n en este momento, puede seleccionar �ninguno� y ser� retornado
al men� principal.  Cuando m�s adelante est� preparado, deber� seleccionar
�actualizar� desde el men� principal para completar la instalaci�n.

message cdrombadmount
{No se ha podido montar el CDROM /dev/%s.}

message localfsbadmount
{No se ha podido montar %s en el dispositivo local %s.}
.endif

message oldsendmail
{Sendmail ya no est� disponible en esta versi�n de NetBSD; el MTA por defecto
es ahora postfix.  El fichero /etc/mailer.conf a�n est� configurado para usar
el sendmail eliminado.  �Desea actualizar el fichero /etc/mailer.conf
autom�ticamente para que apunte a postfix?  Si escoge "No" tendr� que
actualizar /etc/mailer.conf usted mismo para asegurarse de que los mensajes
de correo electr�nico se env�en correctamente.}

message license
{To use the network interface %s, you must agree to the license in
file %s.
To view this file now, you can type ^Z, look at the contents of
the file and then type "fg" to resume.}

message binpkg
{To configure the binary package system, please choose the network location
to fetch packages from.  Once your system comes up, you can use 'pkgin'
to install additional packages, or remove packages.}

message pkgpath
{The following are the protocol, host, directory, user, and password that
will be used.  If "user" is "ftp", then the password is not needed.

}
message rcconf_backup_failed {Making backup of rc.conf failed. Continue?}
message rcconf_backup_succeeded {rc.conf backup saved to %s.}
message rcconf_restore_failed {Restoring backup rc.conf failed.}
message rcconf_delete_failed {Deleting old %s entry failed.}
message Pkg_dir {Package directory}
message configure_prior {configure a prior installation of}
message configure {configure}
message change {change}
message password_set {password set}
message YES {YES}
message NO {NO}
message DONE {DONE}
message abandoned {Abandoned}
message empty {***EMPTY***}
message timezone {Timezone}
message change_rootpw {Change root password}
message enable_binpkg {Enable installation of binary packages}
message enable_sshd {Enable sshd}
message enable_ntpd {Enable ntpd}
message run_ntpdate {Run ntpdate at boot}
message enable_mdnsd {Enable mdnsd}
message configmenu {Configure the additional items as needed.}
message doneconfig {Finished configuring}
message Install_pkgin {Install pkgin and update package summary}
message binpkg_installed 
{You are now configured to use pkgin to install binary packages.  To
install a package, run:

pkgin install <packagename>

from a root shell.  Read the pkgin(1) manual page for further information.}
message Install_pkgsrc {Fetch and unpack pkgsrc}
message pkgsrc
{Installing pkgsrc requires unpacking an archive retrieved over the network.
The following are the host, directory, user, and password that
will be used.  If "user" is "ftp", then the password is not needed.

}
message Pkgsrc_dir {pkgsrc directory}
message get_pkgsrc {Fetch and unpack pkgsrc for building from source}
message retry_pkgsrc_network {Network configuration failed.  Retry?}
message quit_pkgsrc {Quit without installing pkgsrc}
message pkgin_failed 
{Installation of pkgin failed, possibly because no binary packages  exist.  Please check the package path and try again.}
message failed {Failed}
