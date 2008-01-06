/*	$NetBSD: msg.mi.es,v 1.17.4.1 2008/01/06 05:00:35 wrstuden Exp $	*/

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

/* MI Message catalog -- spanish, machine independent */

message usage
{uso: sysinst [-r versión] [-f fichero-definición]
}

message sysinst_message_language
{Mensajes de instalación en castellano}

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


message nodisk
{No se ha podido encontrar ningún disco duro para ser usado por NetBSD.
Se le volverá a llevar al menú original.
}

message onedisk
{Solamente se ha encontrado un disco, %s. 
Por tanto se entiende que quiere %s NetBSD en él.
}

message ask_disk
{¿En cuál disco quiere instalar NetBSD? }

message Available_disks
{Discos disponibles}

message heads
{cabezas}

message sectors
{sectores}

message fs_isize
{tamaño promedio del fichero (bytes)}

message mountpoint
{punto de montaje (o 'ninguno')}

message cylname
{cil}

message secname
{sec}

message megname
{MB}

message layout
{NetBSD usa una etiqueta de BSD para dividir la porción NetBSD del disco
en varias particiones BSD.  Ahora debería configurar su etiqueta BSD. 

Puede usar un simple editor para establecer los tamaños de las particiones
NetBSD, o mantener los tamaños de partición y contenidos actuales.

Después se la dará la oportunidad de cambiar cualquier campo de la
etiqueta.

La parte NetBSD de su disco es de %d megabytes.
Una instalación completa requiere al menos %d megabytes sin X y
al menos %d megabytes si se incluyen los conjuntos de X.
}

message Choose_your_size_specifier
{Seleccionar megabytes producirá tamaños de partición cercanos a su
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

El espacio libre sobrante será añadido a la partición marcada con «+».
}

message ptnheaders
{
       MB         Cilindros	Sectores  Sistema de archivos
}

message askfsmount
{¿Punto de montaje?}

message askfssize
{¿Tamaño de %s en %s?}

message askunits
{Cambiar las unidades de entrada (sectores/cilindros/MB)}

message NetBSD_partition_cant_change
{Partición NetBSD}

message Whole_disk_cant_change
{Todo el disco}

message Boot_partition_cant_change
{Partición de arranque}

message add_another_ptn
{Añadir una partición definida por el usuario}

message fssizesok
{Aceptar los tamaños de las particiones.  Espacio libre %d %s, %d particiones libres.}

message fssizesbad
{Reducir los tamaños de las particiones en %d %s (%d sectores).}

message startoutsidedisk
{El valor del comienzo que ha especificado está mas allá del final del disco.
}

message endoutsidedisk
{Con este valor, el final de la partición está mas allá del final del disco.
El tamaño de la partición se ha truncado a %d %s.

Presione Intro para continuar
}

message fspart
{Sus particiones con etiquetas BSD están ahora así.
Ésta es su última oportunidad para cambiarlas.

}

message fspart_header
{   Inicio %3s Fin %3s   Tamaño %3s Tipo FS    Newfs Mont. Punto mont. 
   ---------- --------- ---------- ---------- ----- ----- -----------
}

message fspart_row
{%10d %9d %10d %-10s %-5s %-5s %s}

message show_all_unused_partitions
{Mostrar todas las particiones no usadas}

message partition_sizes_ok
{Tamaños de partición ok}

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
{        tamaño: %9u %8u%c %9u}

message end_fmt
{           fin: %9u %8u%c %9u}

message bsize_fmt
{tamaño  bloque: %9d bytes}

message fsize_fmt
{   tamaño frag: %9d bytes}

message isize_fmt
{tam prom archi: %9d bytes (para número de inodos)}
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
Valores especiales que se pueden introducir para el valor del tamaño:
    -1:   usar hasta el final de la parte NetBSD del disco
   a-%c:   terminar esta partición donde empieza la partición X

tamaño (%s)}

message label_offset
{%s
Valores especiales que se pueden introducir para el valor del desplazamiento:
    -1:   empezar al principio de la parte NetBSD del disco
   a-%c:   empezar al final de la partición anterior (a, b, ..., %c)

inicio (%s)}

message invalid_sector_number
{Número de sector mal formado
}

message Select_file_system_block_size
{Seleccione el tamaño de bloque del sistema de archivos}

message Select_file_system_fragment_size
{Seleccione el tamaño de fragmento del sistema de archivos}

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

message mountfail
{el montaje del dispositivo /dev/%s%c en %s ha fallado.
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

message ftpsource
{Lo siguiente son el sitio %s, directorio, usuario y contraseña que se
usarán.  Si «usuario» es «ftp», no se necesita contraseña..

}

message email
{dirección de correo electrónico}

message dev
{dispositivo}

message nfssource
{Introduzca el servidor nfs y el directorio del servidor donde se encuentre
la distribución.  Recuerde: el directorio debe contener los archivos .tgz y
debe ser montable por nfs.

}

message floppysource
{Introduzca el dispositivo de disquete a usar y el directorio destino de la
transferencia en el sistema de archivos. Los archivos del conjunto han de estar
en el directorio raíz de los disquetes.

}

message cdromsource
{Introduzca el dispositivo de CDROM a usar y el directorio del CDROM
donde se encuentre la distribución.
Recuerde, el directorio debe contener los archivos .tgz.

}

message localfssource
{Introduzca el dispositivo local desmontado y el directorio de ese
dispositivo donde se encuentre la distribución. 
Recuerde, el directorio debe contener los archivos .tgz.

}

message localdir
{Introduzca el directorio local ya montado donde se encuentre la distribución.
Recuerde, el directorio debe contener los archivos .tgz.

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
{Se han encontrado las siguientes interfaces de red: %s
\n¿Cuál se debería usar?}

message badnet
{No ha seleccionado ninguna interfaz de las listadas.  Por favor, vuelva a
intentarlo.
Están disponibles las siguientes interfaces de red: %s
\n¿Cuál debería usar?}

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
Nombre de máquina:	%s 
Interfaz primaria:	%s 
IP de la máquina:	%s 
Máscara de red:		%s 
Serv de nombres IPv4:	%s 
Pasarela IPv4:		%s 
Tipo de medio:		%s 
}

message netok_slip
{Ha introducido los siguientes valores. ¿Son correctos?

Dominio DNS: 		%s 
Nombre de la máquina:	%s 
Interfaz primaria:	%s 
IP de la máquina:	%s 
IP del servidor:	%s 
Máscara de red:		%s 
Serv de nombres IPv4:	%s 
Pasarela IPv4:		%s 
Tipo de medio:		%s 
}

message netokv6
{IPv6 autoconf:		%s 
Serv de nombres IPv6:	%s 
}

message netok_ok
{¿Son correctos?}

message netagain
{Por favor, vuelva a introducir la información sobre su red.  Sus últimas
respuestas serán las predeterminadas.

}

message wait_network
{
Espere mientras las interfaces de red se levantan.
}

message resolv
{No se ha podido crear /etc/resolv.conf.  Instalación interrumpida.
}

message realdir
{No se ha podido cambiar el directorio a %s: %s.  Instalación
interrumpida.
}

message delete_xfer_file
{A eliminar después de la instalación}

message verboseextract
{
El siguiente paso es descargar y desempaquetar los conjuntos de
ficheros de la distribución.

Durante el proceso de extracción, ¿qué desea ver según se vaya
extrayendo cada uno de los ficheros?
}

message notarfile
{El conjunto %s no existe.}

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

message abort
{Sus opciones han hecho imposible instalar NetBSD.  Instalación interrumpida.
}

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
{No se puede guardar /usr/X11R6/bin/X como /usr/X11R6/bin/X.old, porque el
disco objetivo ya tiene un /usr/X11R6/bin/X.old.  Por favor, arregle esto
antes de continuar.

Una manera es iniciando una shell desde el menú Utilidades, y examinar
el objetivo /usr/X11R6/bin/X y /usr/X11R6/bin/X.old.  Si
/usr/X11R6/bin/X.old es de una actualización completada, puede rm -f
/usr/X11R6/bin/X.old y reiniciar.  O si /usr/X11R6/bin/X.old es de
una actualizacion reciente e incompleta, puede rm -f /usr/X11R6/bin/X
y mv /usr/X11R6/bin/X.old a /usr/X11R6/bin/X.

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

message badfs
{Parece que /dev/%s%c no es un sistema de archivos BSD o el fsck no ha sido
correcto.  La actualización ha sido interrumpida.  (Error número %d.)
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

message set_games
{Juegos}

message set_man_pages
{Paginas de manual}

message set_misc
{Varios}

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
no estará completo.

¿Continuar extrayendo conjuntos?}

message must_be_one_root
{Debe haber una sola partición marcada para ser montada en «/».}

message partitions_overlap
{las particiones %c y %c se solapan.}

message edit_partitions_again
{

Puede editar la tabla de particiones a mano, o dejarlo y retornar al
menú principal.

¿Editar la tabla de particiones de nuevo?}

message config_open_error
{No se ha podido abrir el fichero de configuración %s\n}

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

message choose_crypt
{Por favor, seleccione el algoritmo de cifrado de contraseñas a usar.
NetBSD puede ser configurado para usar los esquemas DES, MD5 o Blowfish.

El esquema tradicional DES es compatible con la mayoría de los demás
sistemas operativos de tipo Unix, pero sólo se reconocerán los primeros 8
carácteres de cualquier contraseña.
Los esquemas MD5 y Blowfish permiten contraseñas más largas, y algunos
aseguran que es más seguro.

Si tiene una red y pretende usar NIS, por favor considere las capacidades
de otras máquinas en su red.

Si está actualizando y le gustaria mantener la configuración sin cambios,
escoja la última opción «no cambiar».
}

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
message Set_Sizes {Establecer los tamaños de las particiones NetBSD}
message Use_Existing {Usar los tamaños de las particiones existentes}
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
message Select_your_distribution {Seleccione su distribución}
message Full_installation {Instalación completa}
message Minimal_installation {Instalación mínima}
message Custom_installation {Instalación personalizada}
message hidden {** oculto **}
message Host {Máquina}
message Base_dir {Directorio base}
message Set_dir {Directorio de conjuntos}
message Xfer_dir {Directorio a transferir a}
message User {Usuario}
message Password {Contraseña}
message Proxy {Proxy}
message Get_Distribution {Obtener la distribución}
message Continue {Continuar}
message What_do_you_want_to_do {¿Qué desea hacer?}
message Try_again {Reintentar}
message Set_finished {Conjunto finalizado}
message Skip_set {Omitir conjunto}
message Skip_group {Omitir grupo de conjuntos}
message Abandon {Abandonar instalación}
message Abort_fetch {Abortar lectura}
message Password_cipher {Cifrado de las contraseñas}
message DES {DES}
message MD5 {MD5}
message Blowfish_2_7_round {Blowfish 2^7 round}
message do_not_change {no cambiar}
message Device {Dispositivo}
message File_system {Sistema de archivos}
message Select_IPv6_DNS_server {  Seleccione servidor DNS de IPv6}
message other {otro }
message Perform_IPv6_autoconfiguration {¿Realizar autoconfiguración IPv6?}
message Perform_DHCP_autoconfiguration {¿Realizar autoconfiguración DHCP ?}
message Root_shell {Shell de root}
message Select_set_extraction_verbosity {Seleccione la prolijidad de la extracción de conjuntos}
message Progress_bar {Barra de progreso (recomendado)}
message Silent {Silencioso}
message Verbose {Listado de nombres de ficheros detallado (lento)}

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
