/*	$NetBSD: msg.mi.es,v 1.34 2022/05/18 16:39:03 martin Exp $	*/

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

/*
 * We can not use non ascii characters in this message - it is displayed
 * before the locale is set up!
 */
message sysinst_message_language
{Mensajes de instalacion en castellano}

message sysinst_message_locale
{es_ES.ISO8859-15}

message	out_of_memory	{Out of memory!}
message Yes {Sí}
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
message Delete {¿Borrar?}

message install
{instalar}

message reinstall
{reinstalar conjuntos para}

message upgrade
{actualizar}

message hello
{Bienvenido a sysinst, la herramienta de instalación de NetBSD-@@VERSION@@.
Esta herramienta guiada por menús está diseñada para ayudarle a instalar
NetBSD en un disco duro, o actualizar un sistema NetBSD existente con
un trabajo mínimo. 
En los siguientes menús teclee la letra de referencia (a, b, c, ...) para
seleccionar una opción, o teclee CTRL+N/CTRL+P para seleccionar la opción
siguiente/anterior. 
Las teclas de cursor y AvPág/RePág puede que también funcionen. 
Active la selección actual desde el menú pulsando la tecla Intro.
}

message thanks
{¡Gracias por usar NetBSD!
}

message installusure
{Ha escogido instalar NetBSD en su disco duro.  Esto cambiará información
de su disco duro.  ¡Debería haber hecho una copia de seguridad completa
antes de este procedimiento!  Este procedimiento realizará las siguientes
operaciones:
	a) Particionar su disco
	b) Crear nuevos sistemas de ficheros BSD
	c) Cargar e instalar los conjuntos de distribución
	d) Algunas configuraciones iniciales del sistema

(Después de introducir la información de las particiones pero antes de que
su disco sea cambiado, tendrá la oportunidad de salir del programa.)

¿Desea continuar?
}

message upgradeusure
{Se va a actualizar NetBSD en su disco duro.  Sin embargo, esto
cambiará información de su disco duro.  ¡Debería hacer una copia de seguridad
completa antes de este procedimiento!  ¿Realmente desea actualizar NetBSD?
(Éste es su último aviso antes de que el programa empiece a modificar
sus discos.)
}

message reinstallusure
{Se va a desempaquetar los conjuntos de distribución de NetBSD
a un disco duro marcado como arrancable.  Este procedimiento solamente
descarga y desempaqueta los conjuntos en un disco arrancable preparticionado.
No pone nombre a los discos, ni actualiza los bloques de arranque, ni guarda
ninguna información de configuración.  (Salga y escoja `instalar' o
`actualizar' si quiere esas opciones.)  ¡Ya debería haber hecho un
`instalar' o `actualizar' antes de iniciar este procedimiento!

¿Realmente quiere reinstalar los conjuntos de la distribución NetBSD?
(Éste es su último aviso antes de que el programa empiece a modificar
sus discos.)
}

message mount_failed
{No se ha podido montar %s.  ¿Desea continuar?
}

message nodisk
{No se ha podido encontrar ningún disco duro para ser usado por NetBSD.
Se le volverá a llevar al menú original.
}

message onedisk
{Solamente se ha encontrado un disco, %s. 
Por tanto se entiende que quiere %s NetBSD en él.
}

message ask_disk
{¿En cuál disco quiere %s NetBSD? }

message Available_disks
{Discos disponibles}

message Available_wedges	{Existing "wedges"}

message heads
{cabezas}

message sectors
{sectores}

message mountpoint
{punto de montaje (o 'ninguno')}

message cylname
{cil}

message secname
{sec}

message megname
{MB}

message gigname
{GB}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */
message	layout_prologue_none
{You can use a simple editor to set the sizes of the NetBSD partitions,
or apply the default partition sizes and contents.}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */

message	layout_prologue_existing
{If you do not want to use the existing partitions, you can
use a simple editor to set the sizes of the NetBSD partitions,
or remove existing ones and apply the default partition sizes.}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */
message layout_main
{
You will then be given the opportunity to change any of the partition
details.

The NetBSD (or free) part of your disk ($0) is $3.

A full installation requires at least $4 without X and
at least $5 if the X sets are included.}

message Choose_your_size_specifier
{Seleccionar mega/gigabytes producirá tamaños de partición cercanos a su
elección, pero alineados a los limites de los cilindros.
Seleccionar sectores le permitirá especificar los tamaños de manera
más precisa.  En discos ZBR modernos, el tamaño real del cilindro varía
a lo largo del disco y no hay mucha ventaja real en el alineamiento de
cilindros.  En discos más viejos, lo más eficiente es seleccionar
tamaños de partición que sean multiples exactos del tamaño real del 
cilindro.

Escoja su especificador de tamaño}

message ptnsizes
{Ahora puede cambiar los tamaños de las particiones del sistema.  Por
omisión se asigna todo el espacio al sistema de archivos raíz, sin embargo
usted podría querer separar /usr (ficheros de sistema adicionales), /var
(ficheros de registro, etc) o /home (directorios de usuario).

El espacio libre sobrante será añadido a la partición marcada con «+».}

/* Called with: 			Example
 *  $0 = list of marker explanations	'=' existining, '@' external
 */
message ptnsizes_markers		{Other markers: $0 partition.}
message ptnsizes_mark_existing		{«=» existing}
message ptnsizes_mark_external		{«@» external}

message ptnheaders_size		{Size}
message ptnheaders_filesystem	{Sistema de archivos}

message askfsmount
{¿Punto de montaje?}

message askfssize
{¿Tamaño de %s en %s?}

message askunits
{Cambiar las unidades de entrada (sectores/cilindros/MB/GB)}

message NetBSD_partition_cant_change
{Partición NetBSD}

message Whole_disk_cant_change
{Todo el disco}

message Boot_partition_cant_change
{Partición de arranque}

message add_another_ptn
{Añadir una partición definida por el usuario}

/* Called with: 			Example
 *  $0 = free space			1.4
 *  $1 = size unit			GB
 */
message fssizesok
{Go on.  Free space $0 $1.}

/* Called with: 			Example
 *  $0 = missing space			1.4
 *  $1 = size unit			GB
 */
message fssizesbad
{Abort.  Not enough space, $0 $1 missing!}

message startoutsidedisk
{El valor del comienzo que ha especificado está mas allá del final del disco.
}

message endoutsidedisk
{Con este valor, el final de la partición está mas allá del final del disco.
El tamaño de la partición se ha truncado.}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Master Boot Record (MBR)
 *  $2 = short version of $1		MBR
 *  $3 = disk size			3TB
 *  $4 = size limit			2TB
 */
message toobigdisklabel
{
This disk ($0) is too large ($3) for a $2 partition table (max $4),
hence only the start of the disk is usable.
}

message cvtscheme_hdr		{What would you like to do to the existing partitions?}
message cvtscheme_keep		{keep (use only part of disk)}
message cvtscheme_delete	{delete (all data will be lost!)}
message cvtscheme_convert	{convert to another partitioning method}
message cvtscheme_abort		{abort}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	BSD disklabel
 *  $2 = short version of $1		disklabel
 *  $3 = optional install flag		(I)nstall,
 *  $4 = additional flags description	(B)ootable
 *  $5 = total size			2TB
 *  $6 = free size			244MB
 */
message fspart
{Sus particiones están ahora así.
Ésta es su última oportunidad para cambiarlas.

Flags: $3(N)ewfs$4.   Total: $5, free: $6}

message ptnheaders_start	{Inicio}
message ptnheaders_end		{Fin}
message ptnheaders_fstype	{Tipo FS}

message partition_sizes_ok
{Tamaños de partición ok}

message edfspart
{Los valores actuales de la particion son.

Seleccione el campo que desee cambiar:}

message ptn_newfs		{newfs}
message ptn_mount		{montar}
message ptn_mount_options	{opc montaje}
message ptn_mountpt		{punto montaje}

message toggle
{Conmutar}

message restore
{Restaurar los valores originales}

message Select_the_type
{Seleccione el tipo}

message other_types
{otros tipos}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_head
{Valores especiales que se pueden introducir para el valor del tamaño:
    -1:   usar hasta el final del disco}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_part_hint
{   $0:   terminar esta partición donde empieza la partición X}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_tail
{Tamaño (max $1 $2)}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_head
{Valores especiales que se pueden introducir para el valor del desplazamiento:
    -1:   empezar al principio del disco}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_part_hint
{   $0:   empezar al final de la partición anterior}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_space_hint
{   $1:   start at the beginning of given free space}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_tail		{inicio ($2)}

message invalid_sector_number
{Número mal formado}

message packname
{Por favor entroduzca un nombre para el disco NetBSD}

message lastchance
{Bien, todo está preparado para instalar NetBSD en su disco duro (%s).
Todavía no se ha escrito nada.  Ésta es su última oportunidad para salir
del proceso antes de que se cambie nada.

¿Desea continuar?
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

/* Called with:				Example
 *  $0 = device name			/dev/wd0a
 *  $1 = mount path			/usr
 */
message mountfail
{el montaje del dispositivo $0 en $1 ha fallado.
}

message extractcomplete
{La extracción de los conjuntos seleccionados para NetBSD-@@VERSION@@ ha
finalizado.  El sistema es ahora capaz de arrancar desde el disco duro
seleccionado.  Para completar la instalación, sysinst le dará la
oportunidad de configurar antes algunos aspectos esenciales.
}

message instcomplete
{La instalación de NetBSD-@@VERSION@@ ha finalizado.  El sistema debería
arrancar desde el disco duro.  Siga las instrucciones del documento
INSTALL sobre la configuración final del sistema.  La pagina de manual
de afterboot(8) es otra lectura recomendada; contiene una lista de
cosas a comprobar despúes del primer arranque completo.

Como mínimo, debe editar /etc/rc.conf para que cumpla sus necesidades.
Vea en /etc/defaults/rc.conf los valores predefinidos.
}

message upgrcomplete
{Actualización a NetBSD-@@VERSION@@ completada.  Ahora tendrá que
seguir las instrucciones del documento INSTALL para hacer lo que
necesite para conseguir tener el sistema configurado para su
situación.
Recuerde (re)leer la página de manual de afterboot(8), ya que puede
contener nuevos apartados desde su ultima actualización.

Como mínimo, debe editar rc.conf para su entorno local y cambiar
rc_configured=NO a rc_configured=YES o los reinicios se detendrán en
single-user, y copie de nuevo los ficheros de contraseñas (teniendo en
cuenta las nuevas cuentas de sistema que se hayan podido crear para esta
versión) si estuviera usando ficheros de contraseñas locales.
}


message unpackcomplete
{Finalizado el desempaquetamiento de conjuntos adicionales de NetBSD-@@VERSION@@.
Ahora necesitará seguir las instrucciones del documento INSTALL para
tener su sistema reconfigurado para su situación.
La página de manual de afterboot(8) también puede serle de ayuda.

Como mínimo, debe editar rc.conf para su entorno local, y cambiar
rd_configure=NO a rc_configured=YES, o los reinicios se detendrán
en modo de usuario único.
}

message distmedium
{Su disco está ahora preparado para la instalación el nucleo y los conjuntos
de la distribución.  Como se apunta en las notas INSTALL, tiene diversas
opciones.  Para ftp o nfs, tiene que estar conectado a una red con acceso
a las maquinas apropiadas.

Conjuntos seleccionados: %d, procesados: %d. Siguiente conjunto: %s.

}

message distset
{La distribución NetBSD está dividida en una colección de conjuntos de
distribución.  Hay algunos conjuntos básicos que son necesarios para todas
las instalaciones, y otros conjuntos que no son necesarios para todas las
instalaciones.  Puede escoger para instalar sólo los conjuntos esenciales
(instalación mínima); instalar todos ellos (Instalación completa) o
seleccionar de entre los conjuntos de distribución opcionales.
}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 *  $1 = URL protocol used		ftp
 */
message ftpsource
{Lo siguiente son el sitio $1, directorio, usuario y contraseña que se
usarán.  Si «usuario» es «ftp», no se necesita contraseña..

}

message email
{dirección de correo electrónico}

message dev
{dispositivo}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message nfssource
{Introduzca el servidor nfs y el directorio del servidor donde se encuentre
la distribución.  Recuerde: el directorio debe contener los archivos $0 y
debe ser montable por nfs.

}

message floppysource
{Introduzca el dispositivo de disquete a usar y el directorio destino de la
transferencia en el sistema de archivos. Los archivos del conjunto han de estar
en el directorio raíz de los disquetes.

}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message cdromsource
{Introduzca el dispositivo de CDROM a usar y el directorio del CDROM
donde se encuentre la distribución.
Recuerde, el directorio debe contener los archivos $0.

}

message No_cd_found
{Could not locate a CD medium in any drive with the distribution sets! 
Enter the correct data manually, or insert a disk and retry. 
}

message abort_install
{Cancel installation}

message source_sel_retry
{Back to source selection & retry}

message Available_cds
{Available CDs}

message ask_cd
{Multiple CDs found, please select the one containing the install CD.}

message cd_path_not_found
{The installation sets have not been found at the default location on this
CD. Please check device and path name.}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message localfssource
{Introduzca el dispositivo local desmontado y el directorio de ese
dispositivo donde se encuentre la distribución. 
Recuerde, el directorio debe contener los archivos $0.

}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message localdir
{Introduzca el directorio local ya montado donde se encuentre la distribución.
Recuerde, el directorio debe contener los archivos $0.

}

message filesys
{sistema de archivos}

message nonet
{No se ha podido encontrar ninguna interfaz de red para ser usada por NetBSD.
Se le devolverá al menú anterior.
}

message netup
{Las siguientes interfaces están activas: %s
¿Conecta alguna de ellas al servidor requerido?}

message asknetdev
{¿Cuál interfaces usar?}

message netdevs
{Interfaces disponibles}

message netinfo
{Para poder usar la red, necesita responder lo siguiente:

}

message net_domain
{Su dominio DNS}

message net_host
{Su nombre de máquina}

message net_ip
{Su número IPv4}

message net_srv_ip
{Número servidor IPv4}

message net_mask
{Máscara de red IPv4}

message net_namesrv
{Servidor de nombres IPv4}

message net_defroute
{Pasarela IPv4}

message net_media
{Tipo de medio de la red}

message net_ssid
{Wi-Fi SSID?}

message net_passphrase
{Wi-Fi passphrase?}

message netok
{Ha introducido los siguientes valores.

Dominio DNS: 		%s 
Nombre de máquina:	%s 
Serv de nombres:	%s 
Interfaz primaria:	%s 
Tipo de medio:		%s 
IP de la máquina:	%s 
Máscara de red:		%s 
Pasarela IPv4:		%s 
}

message netok_slip
{Ha introducido los siguientes valores. ¿Son correctos?

Dominio DNS: 		%s 
Nombre de la máquina:	%s 
Serv de nombres:	%s 
Interfaz primaria:	%s 
Tipo de medio:		%s 
IP de la máquina:	%s 
IP del servidor:	%s 
Máscara de red:		%s 
Pasarela IPv4:		%s 
}

message netokv6
{IPv6 autoconf:		%s 
}

message netok_ok
{¿Son correctos?}

message wait_network
{
Espere mientras las interfaces de red se levantan.
}

message resolv
{No se ha podido crear /etc/resolv.conf.  Instalación interrumpida.
}

/* Called with: 			Example
 *  $0 = target prefix			/target
 *  $1 = error message			No such file or directory
 */
message realdir
{No se ha podido cambiar el directorio a $0: $1
Instalación interrumpida.}

message delete_xfer_file
{A eliminar después de la instalación}

/* Called with: 			Example
 *  $0 = set name			base
 */
message notarfile
{El conjunto $0 no existe.}

message endtarok
{Todos los conjuntos de distribución han sido desempaquetados
correctamente.}

message endtar
{Ha habido problemas al desempaquetar los conjuntos de la distribución.
Su instalación está incompleta.

Ha seleccionado %d conjuntos de distribución.  %d conjuntos no han sido
encontrados, y %d han sido omitidos después de que ocurriera un
error.  De los %d que se han intentado, %d se han desempaquetado
sin errores y %d con errores.

La instalación ha sido interrumpida.  Por favor, compruebe de nuevo su
fuente de distribución y considere el reinstalar los conjuntos desde
el menú principal.}

message abort_inst {Instalación interrumpida.}
message abort_part {Partitioning aborted.}

message abortinst
{La distribución no ha sido cargada correctamente.  Necesitará proceder
a mano.  Instalación interrumpida.
}

message abortupgr
{La distribución no ha sido correctamente cargada.  Necesitará proceder
a mano.  Instalación interrumpida.
}

message abortunpack
{El desempaquetamiento de los conjuntos adicionales no ha sido satisfactorio. 
Necesitará proceder a mano, o escoger una fuente diferente para los conjuntos
de esta distribución y volver a intentarlo.
}

message createfstab
{¡Hay un gran problema!  No se puede crear /mnt/etc/fstab.  Desistiendo.
}


message noetcfstab
{¡Ayuda!  No hay /etc/fstab del disco objetivo %s.
Interrumpiendo la actualización.
}

message badetcfstab
{¡Ayuda!  No se puede analizar /etc/fstab en el disco objetivo %s.
Interrumpiendo la actualización.
}

message X_oldexists
{No se puede guardar %s/bin/X como %s/bin/X.old, porque el
disco objetivo ya tiene un %s/bin/X.old.  Por favor, arregle esto
antes de continuar.

Una manera es iniciando una shell desde el menú Utilidades, y examinar
el objetivo %s/bin/X y %s/bin/X.old.  Si
%s/bin/X.old es de una actualización completada, puede rm -f
%s/bin/X.old y reiniciar.  O si %s/bin/X.old es de
una actualizacion reciente e incompleta, puede rm -f %s/bin/X
y mv %s/bin/X.old a %s/bin/X.

Interrumpiendo la actualización.}

message netnotup
{Ha habido un problema configurando la red.  O su pasarela o su servidor
de nombres no son alcanzables por un ping.  ¿Quiere configurar la red
de nuevo?  («No» le permite continuar de todos modos o interrumpir el
proceso de instalación.)
}

message netnotup_continueanyway
{¿Le gustaría continuar el proceso de instalación de todos modos, y
suponer que la red está funcionando?  («No» interrumpe el proceso de
instalación.)
}

message makedev
{Creando nodos de dispositivo ...
}

/* Called with:				Example
 *  $0 = device name			/dev/rwd0a
 *  $1 = file system type		ffs
 *  $2 = error return code form fsck	8
 */
message badfs
{Parece ser que $0 no es un sistema de archivos $1 o bien el
fsck ha fallado.  ¿Desea montarlo de todos modos?  (Código de error:
$2.)
}

message rootmissing
{ el directorio raíz objetivo no existe %s.
}

message badroot
{El nuevo sistema de archivos raíz no ha pasado la comprobación básica.
 ¿Está seguro de que ha instalado todos los conjuntos requeridos?

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
{Por favor, inserte el disquete que contiene el fichero «%s.%s».

Si el conjunto no está en más discos, seleccione "Conjunto finalizado"
para instalarlo. Seleccione "Abortar lectura" para regresar al menú
de selección de medios de instalación.
}

message mntnetconfig
{La información de red que ha introducido, ¿es la adecuada para el
funcionamiento normal de esta máquina y desea instalarla en /etc? }

message cur_distsets
{Ésta es la lista de conjuntos de distribución que se usará.

}

message cur_distsets_header
{   Conjunto de distribución Selecc.
   ------------------------ --------
}

message set_base
{Base}

message set_system
{Sistema (/etc)}

message set_compiler
{Herramientas del compilador}

message set_dtb
{Devicetree hardware descriptions}

message set_games
{Juegos}

message set_gpufw
{Graphics driver firmware}

message set_man_pages
{Paginas de manual}

message set_misc
{Varios}

message set_modules
{Kernel Modules}

message set_rescue
{Recovery tools}

message set_tests
{Programas de prueba}

message set_text_tools
{Herramientas de procesamiento de textos}

message set_X11
{Conjuntos de X11}

message set_X11_base
{X11 base y clientes}

message set_X11_etc
{Configuración de X11}

message set_X11_fonts
{Tipos de letra de X11}

message set_X11_servers
{Servidores X11}

message set_X11_prog
{Programación de X11}

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

message select_all
{Seleccionar todos los conjuntos anteriores}

message select_none
{No seleccionar ninguno de los conjuntos anteriores}

message install_selected_sets
{Instalar los conjuntos seleccionados}

message tarerror
{Ha habido un error al extraer el fichero %s.  Esto significa
que algunos ficheros no han sido extraidos correctamente y su sistema
no estará completo.

¿Continuar extrayendo conjuntos?}

/* Called with: 			Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message must_be_one_root
{Debe haber una sola partición marcada para ser montada en «/».}

/* Called with: 			Example
 *  $0 = first partition description	70 - 90 MB, MSDOS
 *  $1 = second partition description	80 - 1500 MB, 4.2BSD
 */
message partitions_overlap
{las particiones $0 y $1 se solapan.}

message No_Bootcode
{No hay código de arranque para la partición root}

message cannot_ufs2_root
{Sorry, the root file system can't be FFSv2 due to lack of bootloader support
on this port.}

message edit_partitions_again
{

Puede editar la tabla de particiones a mano, o dejarlo y retornar al
menú principal.

¿Editar la tabla de particiones de nuevo?}

/* Called with: 			Example
 *  $0 = missing file			/some/path
 */
message config_open_error
{No se ha podido abrir el fichero de configuración $0}

message choose_timezone
{Por favor, escoja de la siguiente lista la zona horaria que le convenga.  
Presione RETURN para seleccionar una entrada. 
Presione «x» seguido de RETURN para salir de la selección de la
zona horaria.

 Predefinida:	%s 
 Seleccionada:	%s 
 Hora local: 	%s %s 
}

message tz_back
{ Volver a la lista principal de zonas horarias}

message swapactive
{El disco que ha seleccionado tiene una partición de intercambio (swap) que
puede que esté en uso actualmente si su sistema tiene poca memoria.  Como
se dispone a reparticionar este disco, esta partición swap será desactivada
ahora.  Se advierte de que esto puede conducir a problemas de swap.
Si obtuviera algun error, reinicie el sistema e inténtelo de nuevo.}

message swapdelfailed
{Sysinst ha fallado en la desactivación de la partición swap del disco que
ha escogido para la instalación.  Por favor, reinicie e inténtelo de nuevo.}

message rootpw
{La contraseña de root del nuevo sistema instalado no ha sido asignada todavía,
y por tanto está vacía.  ¿Quiere establecer ahora una contraseña de root para
el sistema?}

message force_rootpw
{The root password of the newly installed system has not yet been
initialized. 
 
If you do not want to set a password, enter an empty line.}

message rootsh
{Ahora puede seleccionar que shell quiere usar para el usuario root.  Por
omisión es /bin/sh, pero podría preferir otra.}

message no_root_fs
{
No hay un sistema de archivos raíz definido.  Necesitará al menos un punto
de montaje con «/».

Presione <return> para continuar.
}

message slattach {
Introduzca los parámetros de slattach
}

message Pick_an_option {Seleccione una opción para activar/desactivar.}
message Scripting {Scripting}
message Logging {Logging}

message Status  { Estado: }
message Command {Orden: }
message Running {Ejecutando}
message Finished {Acabado   }
message Command_failed {Orden fallida}
message Command_ended_on_signal {Orden terminada en señal}

message NetBSD_VERSION_Install_System {Sistema de instalación de NetBSD-@@VERSION@@}
message Exit_Install_System {Salir del sistema de instalación}
message Install_NetBSD_to_hard_disk {Instalar NetBSD en el disco duro}
message Upgrade_NetBSD_on_a_hard_disk {Actualizar NetBSD en un disco duro}
message Re_install_sets_or_install_additional_sets {Reinstalar conjuntos o instalar conjuntos adicionales}
message Reboot_the_computer {Reiniciar la computadora}
message Utility_menu {Menú de utilidades}
message Config_menu {Menu de configuración}
message exit_menu_generic {atrás}
message exit_utility_menu {Exit}
message NetBSD_VERSION_Utilities {Utilidades de NetBSD-@@VERSION@@}
message Run_bin_sh {Ejecutar /bin/sh}
message Set_timezone {Establecer la zona horaria}
message Configure_network {Configurar la red}
message Partition_a_disk {Particionar un disco}
message Logging_functions {Funciones de registro}
message Halt_the_system {Detener el sistema}
message yes_or_no {¿sí o no?}
message Hit_enter_to_continue {Presione intro para continuar}
message Choose_your_installation {Seleccione su instalación}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Keep_existing_partitions
{Use existing $1 partitions}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Set_Sizes {Establecer los tamaños de las particiones NetBSD}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_Default_Parts {Use default partition sizes}

/* Called with:				Example
 *  $0 = current partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_Different_Part_Scheme
{Delete everything, use different partitions (not $1)}

message Gigabytes {Gigabytes}
message Megabytes {Megabytes}
message Bytes {Bytes}
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
message Select_your_distribution {Seleccione su distribución}
message Full_installation {Instalación completa}
message Full_installation_nox {Instalación sin X11}
message Minimal_installation {Instalación mínima}
message Custom_installation {Instalación personalizada}
message hidden {** oculto **}
message Host {Máquina}
message Base_dir {Directorio base}
message Set_dir_src {Directorio de conjuntos binary} /* fix XLAT */
message Set_dir_bin {Directorio de conjuntos source} /* fix XLAT */
message Xfer_dir {Directorio a transferir a}
message transfer_method {Download via}
message User {Usuario}
message Password {Contraseña}
message Proxy {Proxy}
message Get_Distribution {Obtener la distribución}
message Continue {Continuar}
message Prompt_Continue {¿Continuar?}
message What_do_you_want_to_do {¿Qué desea hacer?}
message Try_again {Reintentar}
message Set_finished {Conjunto finalizado}
message Skip_set {Omitir conjunto}
message Skip_group {Omitir grupo de conjuntos}
message Abandon {Abandonar instalación}
message Abort_fetch {Abortar lectura}
message Device {Dispositivo}
message File_system {Sistema de archivos}
message Select_DNS_server {  Seleccione servidor DNS}
message other {otro }
message Perform_autoconfiguration {¿Realizar autoconfiguración ?}
message Root_shell {Shell de root}
message Color_scheme {Combinación de colores}
message White_on_black {Blanco sobre negro}
message Black_on_white {Negro sobre blanco}
message White_on_blue {Blanco sobre azul}
message Green_on_black {Verde sobre negro}
message User_shell {Shell de user}

.if AOUT2ELF
message aoutfail
{No se ha podido crear el directorio al que deberían moverse las bibliotecas
compartidas a.out antiguas.  Por favor, intente el proceso de actualización
de nuevo y asegúrese de que ha montado todos los sistemas de ficheros.}

message emulbackup
{El directorio /emul/aout o /emul de su sistema era un enlace simbólico que
apuntaba a un sistema de archivos desmontado.  Se le ha dado la extension
'.old'.  Cuando vuelva a arrancar su sistema actualizado, puede que necesite
preocuparse de fundir el directorio /emul/aout nuevamente creado con el viejo.
}
.endif

.if xxxx
Si no está preparado para completar la
instalación en este momento, puede seleccionar «ninguno» y será retornado
al menú principal.  Cuando más adelante esté preparado, deberá seleccionar
«actualizar» desde el menú principal para completar la instalación.

message cdrombadmount
{No se ha podido montar el CDROM /dev/%s.}

message localfsbadmount
{No se ha podido montar %s en el dispositivo local %s.}
.endif

message oldsendmail
{Sendmail ya no está disponible en esta versión de NetBSD; el MTA por defecto
es ahora postfix.  El fichero /etc/mailer.conf aún está configurado para usar
el sendmail eliminado.  ¿Desea actualizar el fichero /etc/mailer.conf
automáticamente para que apunte a postfix?  Si escoge "No" tendrá que
actualizar /etc/mailer.conf usted mismo para asegurarse de que los mensajes
de correo electrónico se envíen correctamente.}

message license
{Para usar la interfaz de red %s, debe de aceptar la licencia en el archivo %s.
Para ver este archivo ahora, pulse ^Z, mire el contendido del archivo, y luego
teclee "fg" para continuar la instalación.}

message binpkg
{Para configurar el sistema de paquetes binários, por favor escoja el
sitio de red desde el cual descargar los paquetes.  Una vez el sistema
arranque, puede usar 'pkgin' para instalar paquetes adicionales, o
eliminar paquetes ya instalados.}
	
message pkgpath
{Las siguientes entradas representan el protocolo, la máquina, el
directorio, el usuario y la contraseña que se usarán.  Si el "usuario"
es "ftp", entonces la contraseña es opcional.

}
message rcconf_backup_failed
{Error al intentar hacer una cópia de seguridad de rc.conf.  ¿Desea continuar?}
message rcconf_backup_succeeded
{La cópia de seguridad de rc.conf se ha guardado en %s.}
message rcconf_restore_failed
{La recuperación de rc.conf desde su cópia de seguridad ha fallado.}
message rcconf_delete_failed {La eliminación del viejo %s ha fallado.}
message Pkg_dir {Directorio del paquete}
message configure_prior {configure a prior installation of}
message configure {Configurar}
message change {Cambiar}
message password_set {Contraseña configurada}
message YES {SI}
message NO {NO}
message DONE {HECHO}
message abandoned {Abandonado}
message empty {***VACÍO***}
message timezone {Zona horaria}
message change_rootpw {Cambiar la contraseña de root}
message enable_binpkg {Activar la instalación de paquetes binarios}
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
message doneconfig {Terminar configuración}
message Install_pkgin {Instalar pkgin y actualizar la lista de paquetes}
message binpkg_installed 
{El sistema se ha configurado para usar pkgin para instalar paquetes
binarios.  Para instalar un paquete, ejecute:

pkgin install <nombre_del_paquete>

desde una línea de comandos de root.  Lea la página de manual pkgin(1)
para más detalles.}
message Install_pkgsrc {Descargar y desempaquetar pkgsrc}
message pkgsrc
{La instalación de pkgsrc necesita desempaquetar un archivo descargado
desde la red.
Las siguientes entradas corresponden a la máquina, directorio, usuario
y contraseña a usar para la conexión.  Si "usuario" es "ftp", entonces
la contraseña es opcional.

}
message Pkgsrc_dir {Directorio de pkgsrc}
message get_pkgsrc
{Descargar y desempaquetar pkgsrc}
message retry_pkgsrc_network
{La configuración de la red ha fallado.  ¿Reintentar?}
message quit_pkgsrc {Salir sin instalar pkgsrc}
message quit_pkgs_install {Salir sin instalar bin pkg}
message pkgin_failed 
{La instalación de pkgin ha fallado, posiblemente porque no existen
paquetes binarios.  Por favor verifique el camino a los paquetes y
reinténtelo de nuevo.}
message failed {Error}

message askfsmountadv {Punto de montaje o 'raid' o 'CGD' o 'lvm'?}
message partman {Partición extendida}
message editpart {Editar particiones}
message selectwedge {Preconfigured "wedges" dk(4)}

message fremove {QUITAR}
message remove {Quitar}
message add {Añadir}
message auto {auto}

message removepartswarn {Esta eliminar todas las particiones en el disco!}
message saveprompt {Guarde los cambios antes de terminar?}
message cantsave {Los cambios no se pueden guardar.}
message noroot {No hay una partición raíz definida, no puede continuar \n}

message addusername {8 character username to add}
message addusertowheel {Do you wish to add this user to group wheel?}
message Delete_partition
{Borrar partición}

message No_filesystem_newfs
{The selected partition does not seem to have a valid file system. 
Do you want to newfs (format) it?}

message swap_display	{swap}

/* Called with: 			Example
 *  $0 = parent device name		sd0
 *  $1 = swap partition name		my_swap
 */
message Auto_add_swap_part
{A swap partition (named $1) 
seems to exist on $0. 
Do you want to use that?}

message parttype_disklabel {BSD disklabel}
message parttype_disklabel_short {disklabel}
/*
 * This is used on architectures with MBR above disklabel when there is
 * no MBR on a disk.
 */
message parttype_only_disklabel {disklabel (NetBSD only)}

message select_part_scheme
{The disk seems not to have been partitioned before. Please select
a partitioning scheme from the available options below. }

message select_other_partscheme
{Please select a different partitioning scheme from the available
options below. }

message select_part_limit
{Some schemes have size limits and can only be used for the start
of huge disks. The limit is displayed below.}

/* Called with: 			Example
 *  $0 = device name			ld0
 *  $1 = size				3 TB
 */
message part_limit_disksize
{This device ($0) is $1 big.}

message size_limit	{Max:}

message	addpart		{Add a partition}
message	nopart		{      (no partition defined)}
message	custom_type	{Unknown}

message dl_type_invalid	{Invalid file system type code (0 .. 255)}

message	cancel		{Cancel}

message	out_of_range	{Invalid value}
message	invalid_guid	{Invalid GUID}

message	reedit_partitions	{Re-edit}
message abort_installation	{Abort installation}

message dl_get_custom_fstype {File system type code (upto 255)}

message err_too_many_partitions	{Too many partitions}

/* Called with: 			Example
 *  $0 = mount point			/home
 */
message	mp_already_exists	{$0 already defined!}

message ptnsize_replace_existing
{This is an already existing partition. 
To change its size, the partition will need to be deleted and later
recreated.  All data in this partition will be lost.

Would you like to delete this partition and continue?}

message part_not_deletable	{Non-deletable system partition}

message ptn_type		{tipo}
message ptn_start		{inicio}
message ptn_size		{tamaño}
message ptn_end			{fin}

message ptn_bsize		{tamaño bloque}
message ptn_fsize		{tamaño frag}
message ptn_isize		{tam prom archi}

/* Called with:                         Example
 *  $0 = avg file size in byte          1200
 */
message ptn_isize_bytes		{$0 bytes (para número de inodos)}
message ptn_isize_dflt		{4 fragmentos}

message Select_file_system_block_size
{Seleccione el tamaño de bloque del sistema de archivos}

message Select_file_system_fragment_size
{Seleccione el tamaño de fragmento del sistema de archivos}

message ptn_isize_prompt
{tamaño promedio del fichero (bytes)}

message No_free_space {Sin espacio libre}
message Invalid_numeric {Número no válido!}
message Too_large {Demasiado grande!}

/* Called with:				Example
 *  $0 = start of free space		500
 *  $1 = end of free space		599
 *  $2 = size of free space		100
 *  $3 = unit in use			MB
 */
message free_space_line {Espacio en $0..$1 $3 (tamaño $2 $3)\n}

message	fs_type_ffsv2	{FFSv2}
message	fs_type_ffs	{FFS}
message fs_type_ext2old	{Linux Ext2 (old)}
message	other_fs_type	{Other type}

message	editpack	{Edit name of the disk}
message	edit_disk_pack_hdr
{The name of the disk is arbitrary. 
It is useful for distinguishing between multiple disks.
It may also be used when auto-creating dk(4) "wedges" for this disk.

Enter disk name}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message reeditpart
{¿Quiere reeditar la tabla de particiones $1}


/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = inner partitioning name	BSD disklabel
 *  $3 = short version of $1		MBR
 *  $4 = short version of $2		disklabel
 *  $5 = size needed for NetBSD		250M
 *  $6 = size needed to build NetBSD	15G
 */
message fullpart
{Se va a instalar NetBSD en el disco $0.

NetBSD requiere una sola partición en la tabla de particiones $1 del disco,
que es subsiguientemente dividida por el $2.
NetBSD también puede acceder a sistemas de ficheros de otras particiones $3.

Si selecciona 'Usar todo el disco', se sobreescribirá el contenido anterior
del disco, y se usará una sola partición $3 para cubrir todo el disco. 
Si desea instalar más de un sistema operativo, edite la tabla de particiones
$3 y cree una partición para NetBSD.

Para una instalación básica bastan unos pocos $5, pero deberá
dejar espacio extra para programas adicionales y los ficheros de usuario. 
Proporcione al menos $6 si quiere construir el propio NetBSD.}

message Select_your_choice
{¿Que le gustaría hacer?}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_only_part_of_the_disk
{Editar la tabla de particiones $1}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_the_entire_disk
{Usar todo el disco}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = total disk size		3000 GB
 *  $2 = unallocated space		1.2 GB
 */
message part_header
{   Tamaño total del disco $0: $1 - libre: $2}
message part_header_col_start	{Inicio}
message part_header_col_size	{Tamaño}
message part_header_col_flag	{Opc}

message Partition_table_ok
{Tabla de particiones OK}

message Dont_change
{No cambiar}
message Other_kind
{Otra}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message nobsdpart
{No hay ninguna partición NetBSD en la tabla de particiones $1.}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message multbsdpart
{Hay varias particiones NetBSD en la tabla de particiones $1.
Debería marcar la opción "instalar" en la que quiera usar. }

message ovrwrite
{Su disco tiene actualmente una partición que no es de NetBSD. ¿Realmente
quiere sobreescribir dicha partición con NetBSD?
}

message Partition_OK
{Partición OK}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = short version of $1		MBR
 *  $3 = other flag options		d = bootselect default, a = active
 */
message editparttable
{Se muestra a continuación la tabla de particiones $2 actual. 
Opcn: I => Instalar aquí$3.
Seleccione la partición que desee editar:

}

message install_flag	{I}
message newfs_flag	{N}

message ptn_install	{instalar}
message ptn_instflag_desc	{(I)nstalar, }

message clone_flag	{C}
message clone_flag_desc	{, (C)lone}

message parttype_gpt {Guid Partition Table (GPT)}
message parttype_gpt_short {GPT}

message	ptn_label	{Label}
message ptn_uuid	{UUID}
message	ptn_gpt_type	{GPT Type}
message	ptn_boot	{Boot}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message use_partitions_anyway
{Use this partitions anyway}

message	gpt_flags	{B}
message	gpt_flag_desc	{, (B)ootable}

/* Called with:				Example
 *  $0 = file system type		FFSv2
 */
message size_ptn_not_mounted		{(Other: $0)}

message running_system			{current system}

message clone_from_elsewhere		{Clone external partition(s)}
message select_foreign_part
{Please select an external source partition:}
message select_source_hdr
{Your currently selected source partitions are:}
message clone_with_data			{Clone with data}
message	select_source_add		{Add another partition}
message clone_target_end		{Add at end}
message clone_target_hdr
{Insert cloned partitions before:}
message clone_target_disp		{cloned partition(s)}
message clone_src_done
{Source selection OK, proceed to target selection}

message network_ok
{Your network seems to work fine. 
Should we skip the configuration 
and just use the network as-is?}
