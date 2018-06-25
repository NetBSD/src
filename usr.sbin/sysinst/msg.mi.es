/*	$NetBSD: msg.mi.es,v 1.6.14.1 2018/06/25 07:26:12 pgoyette Exp $	*/

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
{uso: sysinst [-D] [-f fichero_definici�n] [-r versi�n] [-C bg:fg]
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
{No se ha podido montar %s.  �Desea continuar?
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
{�Cu�l interfaces usar?}

message netdevs
{Interfaces disponibles}

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
Serv de nombres:	%s 
Interfaz primaria:	%s 
Tipo de medio:		%s 
IP de la m�quina:	%s 
M�scara de red:		%s 
Pasarela IPv4:		%s 
}

message netok_slip
{Ha introducido los siguientes valores. �Son correctos?

Dominio DNS: 		%s 
Nombre de la m�quina:	%s 
Serv de nombres:	%s 
Interfaz primaria:	%s 
Tipo de medio:		%s 
IP de la m�quina:	%s 
IP del servidor:	%s 
M�scara de red:		%s 
Pasarela IPv4:		%s 
}

message netokv6
{IPv6 autoconf:		%s 
}

message netok_ok
{�Son correctos?}

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
{Parece ser que /dev/%s%c no es un sistema de archivos BSD o bien el
fsck ha fallado.  �Desea montarlo de todos modos?  (C�digo de error:
%d.)
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
{Source and debug sets}

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
{Debug symbols}

message set_xdebug
{X11 debug symbols}

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
message Config_menu {Menu de configuraci�n}
message exit_menu_generic {atr�s}
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
.if HAVE_INSTALL_IMAGE
message cdrom {CD-ROM / DVD / install image media}	/* XXX translation */
.else
message cdrom {CD-ROM / DVD}
.endif
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
message Select_DNS_server {  Seleccione servidor DNS}
message other {otro }
message Perform_autoconfiguration {�Realizar autoconfiguraci�n ?}
message Root_shell {Shell de root}
message Color_scheme {Combinaci�n de colores}
message White_on_black {Blanco sobre negro}
message Black_on_white {Negro sobre blanco}
message White_on_blue {Blanco sobre azul}
message Green_on_black {Verde sobre negro}
message User_shell {Shell de user}

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
{Para usar la interfaz de red %s, debe de aceptar la licencia en el archivo %s.
Para ver este archivo ahora, pulse ^Z, mire el contendido del archivo, y luego
teclee "fg" para continuar la instalaci�n.}

message binpkg
{Para configurar el sistema de paquetes bin�rios, por favor escoja el
sitio de red desde el cual descargar los paquetes.  Una vez el sistema
arranque, puede usar 'pkgin' para instalar paquetes adicionales, o
eliminar paquetes ya instalados.}
	
message pkgpath
{Las siguientes entradas representan el protocolo, la m�quina, el
directorio, el usuario y la contrase�a que se usar�n.  Si el "usuario"
es "ftp", entonces la contrase�a es opcional.

}
message rcconf_backup_failed
{Error al intentar hacer una c�pia de seguridad de rc.conf.  �Desea continuar?}
message rcconf_backup_succeeded
{La c�pia de seguridad de rc.conf se ha guardado en %s.}
message rcconf_restore_failed
{La recuperaci�n de rc.conf desde su c�pia de seguridad ha fallado.}
message rcconf_delete_failed {La eliminaci�n del viejo %s ha fallado.}
message Pkg_dir {Directorio del paquete}
message configure_prior {configure a prior installation of}
message configure {Configurar}
message change {Cambiar}
message password_set {Contrase�a configurada}
message YES {SI}
message NO {NO}
message DONE {HECHO}
message abandoned {Abandonado}
message empty {***VAC�O***}
message timezone {Zona horaria}
message change_rootpw {Cambiar la contrase�a de root}
message enable_binpkg {Activar la instalaci�n de paquetes binarios}
message enable_sshd {Activar sshd}
message enable_ntpd {Activar ntpd}
message run_ntpdate {Ejecutar ntpdate durante el arranque}
message enable_mdnsd {Activar mdnsd}
message enable_xdm {Enable xdm}
message enable_cgd {Enable cgd}
message enable_lvm {Enable lvm}
message enable_raid {Enable raidframe}
message add_a_user {Add a user}
message configmenu {Configurar elementos adicionales bajo demanda.}
message doneconfig {Terminar configuraci�n}
message Install_pkgin {Instalar pkgin y actualizar la lista de paquetes}
message binpkg_installed 
{El sistema se ha configurado para usar pkgin para instalar paquetes
binarios.  Para instalar un paquete, ejecute:

pkgin install <nombre_del_paquete>

desde una l�nea de comandos de root.  Lea la p�gina de manual pkgin(1)
para m�s detalles.}
message Install_pkgsrc {Descargar y desempaquetar pkgsrc}
message pkgsrc
{La instalaci�n de pkgsrc necesita desempaquetar un archivo descargado
desde la red.
Las siguientes entradas corresponden a la m�quina, directorio, usuario
y contrase�a a usar para la conexi�n.  Si "usuario" es "ftp", entonces
la contrase�a es opcional.

}
message Pkgsrc_dir {Directorio de pkgsrc}
message get_pkgsrc
{Descargar y desempaquetar pkgsrc para compilar desde c�digo fuente}
message retry_pkgsrc_network
{La configuraci�n de la red ha fallado.  �Reintentar?}
message quit_pkgsrc {Salir sin instalar pkgsrc}
message quit_pkgs_install {Salir sin instalar bin pkg}
message pkgin_failed 
{La instalaci�n de pkgin ha fallado, posiblemente porque no existen
paquetes binarios.  Por favor verifique el camino a los paquetes y
reint�ntelo de nuevo.}
message failed {Error}

message notsupported {Operaci�n no admitida!}
message askfsmountadv {Punto de montaje o 'raid' o 'CGD' o 'lvm'?}
message partman {Partici�n extendida}
message editbsdpart {Editar particiones BSD}
message editmbr {Editar y guardar MBR}
message switchgpt {Cambiar a GPT}
message switchmbr {Cambiar a MBR}
message renamedisk {Establece el nombre del disco}
message fmtasraid {Formato como RAID}
message fmtaslvm {Formato como LVM PV}
message encrypt {Cifrar}
message setbootable {La bandera de arranque}
message erase {Borrado seguro}
message undo {Deshacer los cambios}
message unconfig {Desconfigurar}
message edit {Editar}
message doumount {Fuerza umount}
message fillzeros {Llenar con ceros}
message fillrandom {Llene los datos al azar}
message fillcrypto {Rellene los datos de cifrado}
message raid0 {0 - Sin paridad, creaci�n de bandas s�lo es simple.}
message raid1 {1 - Creaci�n de reflejos. La paridad es el espejo.}
message raid4 {4 - Striping con paridad almacenada en el �ltimo componente. component.}
message raid5 {5 - Striping con paridad en los componentes de todos. components.}

message fremove {QUITAR}
message remove {Quitar}
message add {A�adir}
message auto {auto}

message removepartswarn {Esta eliminar todas las particiones en el disco. �Desea continuar? want to continue?}
message saveprompt {Guarde los cambios antes de terminar?}
message cantsave {Los cambios no se pueden guardar.}
message noroot {No hay una partici�n ra�z definida, no puede continuar \n}
message wannaunblock {El dispositivo est� bloqueado. �Quiere forzar a desbloquear y continuar? unblock it and continue?}
message wannatry {�Quieres probar?}
message create_cgd {Crear cifrado de volumen (CGD)}
message create_cnd {Crear imagen de disco virtual (VND)}
message create_vg {Crear grupo de vol�menes (LVM VG)}
message create_lv {      Crear vol�menes l�gicos}
message create_raid {Crear RAID por software}
message updpmlist {Actualizar lista de dispositivos}
message savepm {Guardar los cambios}
message pmblocked {BLOQUEADO}
message pmunchanged {SIN USO}
message pmsetboot {ARRANCAR}
message pmused {UTILIZA}
message pmmounted {(montado)}
message pmunused {(sin usar)}
message pmgptdisk {Disco con GPT}

message finishpm {Finalizar el particionado}
message limitcount {L�mite para el n�mero de dispositivos se lleg�!}
message invaliddev {No v�lido dispositivo!}
message avdisks {Discos disponibles:}
message nofreedev {No se puede asignar nodo de dispositivo!}
message partman_header
{Partition Manager. Todos los discos, particiones y etc muestra all�.
Al principio, las particiones MBR hacen, a continuaci�n, hacer etiquetas BSD.
Si desea utilizar RAID, LVM o CGD, siga estos pasos:
1) Crear particiones BSD con el tipo necesario;
2) Crear RAID / LVM VG / CGD el uso de estas particiones; 3) Gu�rdalo;
4) Crear particiones de vol�menes RAID / CGD o l�gico de LVM.}

message raid_menufmt {   raid%d (nivel %1d) en %-34s %11uM}
message raid_err_menufmt {   RAID VACIO!}
message raid_disks_fmt {Discos: %32s}
message raid_spares_fmt {Piezas de recambio: %20s}
message raid_level_fmt {RAID de nivel:    %22d}
message raid_numrow_fmt {numRow:           %22d}
message raid_numcol_fmt {numCol:           %22d}
message raid_numspare_fmt {numSpare:         %22d}
message raid_sectpersu_fmt {sectPerSU:        %22d}
message raid_superpar_fmt {SUsPerParityUnit: %22d}
message raid_superrec_fmt {SUsPerReconUnit:  %22d}
message raid_nomultidim {Matrices multidimensionales no son compatibles!}
message raid_numrow_ask {numRow?}
message raid_numcol_ask {numCol?}
message raid_numspare_ask {numSpare?}
message raid_sectpersu_ask {sectPerSU?}
message raid_superpar_ask {SUsPerParityUnit?}
message raid_superrec_ask {SUsPerReconUnit?}
message raid_disks {Los discos en RAID:}
message vnd_err_menufmt {   CAMINO NO DEFINIDO!}
message vnd_assgn_menufmt {   vnd%1d en %-51s ASSIGN}
message vnd_menufmt {   vnd%1d en %-45s %11uM}
message vnd_path_fmt {Ruta del archivo: %22s}
message vnd_assgn_fmt {Create new image: %14s}
message vnd_size_fmt {Tama�o:          %22sM}
message vnd_ro_fmt {S�lo lectura:     %22s}
message vnd_geom_fmt {Establecer la geometr�a de la mano: %4s}
message vnd_bps_fmt {Bytes por sector:     %18s}
message vnd_spt_fmt {Sectores por pista:   %18s}
message vnd_tpc_fmt {Pistas por cilindro:  %18s}
message vnd_cyl_fmt {Cilindros:        %22s}
message vnd_path_ask {Ruta de acceso?}
message vnd_size_ask {Tama�o (MB)?}
message vnd_bps_ask {Bytes por sector?}
message vnd_spt_ask {Sectores por pista?}
message vnd_tpc_ask {Pistas por cilindro?}
message vnd_cyl_ask {Cilindros}
message cgd_err_menufmt {   DISCO NO SE DEFINE!}
message cgd_menufmt {   cgd%1d %-48s %11uM}
message cgd_dev_fmt {Dispositivo de base: %19s}
message cgd_enc_fmt {Encriptaci�n:        %19s}
message cgd_key_fmt {Tama�o de la clave:  %19d}
message cgd_iv_fmt {Algoritmo IV:        %19s}
message cgd_keygen_fmt {La generaci�n de claves: %15s}
message cgd_verif_fmt {M�todo de verificaci�n:  %15s}
message lvm_disks {Discos en VG:}
message lvm_menufmt {   %-44s %20sM}
message lvm_err_menufmt {   VACIAR VG!}
message lvm_disks_fmt {PV's: %34s}
message lvm_name_fmt {Nombre: %32s}
message lvm_maxlv_fmt {MaxLogicalVolumes:  %20s}
message lvm_maxpv_fmt {MaxPhysicalVolumes: %20s}
message lvm_extsiz_fmt {PhysicalExtentSize: %20s}
message lvm_name_ask {Nombre?}
message lvm_maxlv_ask {MaxLogicalVolumes?}
message lvm_maxpv_ask {MaxPhysicalVolumes?}
message lvm_extsiz_ask {PhysicalExtentSize (MB)?}
message lvmlv_menufmt {      El volumen l�gico %-33s% 11uM}
message lvmlv_name_fmt {Nombre: %32s}
message lvmlv_size_fmt {Tama�o: %31dM}
message lvmlv_ro_fmt {S�lo lectura:  %25s}
message lvmlv_cont_fmt {Contigua:      %25s}
message lvmlv_extnum_fmt {LogicalExtentsNumber: %18s}
message lvmlv_minor_fmt {Menor n�mero:  %25s}
message lvmlv_mirrors_fmt {Espejos:       %25d}
message lvmlv_regsiz_fmt {MirrorLogRegionSize:  %18s}
message lvmlv_pers_fmt {N�mero de persistente menor: %11s}
message lvmlv_readahsect_fmt {ReadAheadSectors:     %18s}
message lvmlv_stripes_fmt {Rayas:         %25s}
message lvmlv_stripesiz_fmt {Stripesize:    %25s}
message lvmlv_zero_fmt {Puesta a cero de la primera KB: %8s}
message lvmlv_name_ask {Nombre?}
message lvmlv_size_ask {Tama�o (MB)?}
message lvmlv_extnum_ask {LogicalExtentsNumber?}
message lvmlv_minor_ask {N�mero menor de edad?}
message lvmlv_mirrors_ask {Espejos?}
message lvmlv_regsiz_ask {MirrorLogRegionSize?}
message lvmlv_readahsect_ask {ReadAheadSectors?}
message lvmlv_stripes_ask {Rayas?}

message addusername {8 character username to add}
message addusertowheel {Do you wish to add this user to group wheel?}
message Delete_partition
{Borrar partici�n}


message No_filesystem_newfs
{The selected partition does not seem to have a valid file system. 
Do you want to newfs (format) it?}

message Auto_add_swap_part
{A swap partition (named %s) seems to exist on %s. 
Do you want to use that?}


