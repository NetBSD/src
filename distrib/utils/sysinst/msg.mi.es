/*	$NetBSD: msg.mi.es,v 1.4.2.2 2005/09/19 21:10:50 tron Exp $	*/

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
{uso: sysinst [-r release] [-f fichero-definición]
}

message sysinst_message_language
{Mensajes de instalación en Español}

message Yes {Si}
message No {No}
message All {Todo}
message Some {Algunos}
message None {Ninguno}
message none {ninguno}
message OK {OK}
message ok {ok}
message On {Encendido}
message Off {Apagado}
message unchanged {sin cambios}
message Delete {borrar?}

message install
{instalar}

message reinstall
{reinstalar sets para}

message upgrade
{actualizar}

message hello
{Bienvenido a sysinst, la herramienta de instalación de NetBSD-@@VERSION@@.
Esta herramienta guiada por menús, está diseñada para ayudarle a instalar
NetBSD en un disco duro, o actualizar un sistema NetBSD existente, con
un trabajo minimo.
En los siguientes menús teclee la letra de referencia (a b, c, ...) para
seleccionar el item, o teclee CTRL+N/CTRL+P para seleccionar la opción
siguiente/anterior.
Las teclas de flechas y AvPag/RePag también funcionan.
Active la seleccion actual desde el menú pulsando la tecla enter.

}

message thanks
{¡Gracias por usar NetBSD!

}

message installusure
{Ha escogido instalar NetBSD en su disco duro. Esto cambiará información
de su disco duro. ¡Debería haber hecho una copia de seguridad completa
antes de este procedimiento! Este procedimiento realizará las siguientes
operaciones :
	a) Particionar su disco
	b) Crear nuevos sistemas de archivos BSD
	c) Cargar e instalar los sets de distribución
	d) Algunas configuraciones iniciales del sistema

(Despues de introducir la información de las particiones pero antes de que
su disco sea cambiado, tendrá la oportunidad de salir del programa.

¿Deberiamos continuar?
}

message upgradeusure
{Esta bien, vamos a actualizar NetBSD en su disco duro. Sin embargo, esto
cambiará información de su disco duro. ¡Debería hacer una copia de seguridad
completa antes de este procedimiento! ¿Realmente desea actualizar NetBSD?
(Este es su último aviso antes de que el programa empiece a modificar
sus discos.)
}

message reinstallusure
{Esta bien, vamos a desempaquetar los sets de la distribución NetBSD
a un disco duro marcado como iniciable.
Este procedimiento solo baja y desempaqueta los sets en un disco iniciable
pre-particionado. No pone nombre a discos, actualiza bootblocks, o guarda
cualquier información de configuración. (Salga y escoja 'instalar' o
'actualizar' si quiere esas opciones.) ¡Ya debería haber hecho un
'instalar' o 'actualizar' antes de iniciar este procedimiento!

¿Realmente quiere reinstalar los sets de la distribución NetBSD?
(Este es su último aviso antes de que el programa empiece a modificar
sus discos.)
}


message nodisk
{No puedo encontrar ningun disco duro para usar con NetBSD. Volverá
al menu original.
}

message onedisk
{Solamente he encontrado un disco, %s.
Por eso asumiré que quiere instalar NetBSD en %s.
}

message ask_disk
{¿En que disco quiere instalar NetBSD? }

message Available_disks
{Discos disponibles}

message cylinders
{cilindros}

message heads
{cabezales}

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
{NetBSD usa BSD disklabel para particionar la porción NetBSD del disco
en multiples particiones BSD. Ahora debería configurar su BSD disklabel.

Puede usar un editor simple para ajustar los tamaños de las particiones NetBSD,
o dejar las particiones existentes, tamaños y contenidos.

Entonces tendrá la oportunidad de cambiar cualquier campo de disklabel.

La parte NetBSD de su disco es de %d Megabytes.
Una instalación completa requiere al menos %d Megabytes sin X y
al menos %d Megabytes si los sets de X son incluidos.
}

message Choose_your_size_specifier
{Seleccionando megabytes dará tamaños de particiones cercanas a su
selección, pero alineados a los limites de los cilindros.
Seleccionando sectores le permitirá especificar los tamaños de manera
mas precisa. En discos ZBR modernos, el tamaño actual del cilindro varía
durante el disco y hay una pequeña ganancia desde el alineamiento del
cilindro. En discos mas viejos, es mas eficiente seleccionar los tamaños
de las particiones que son multiples exactos de su tamaño actual del cilindro.

Escoja su especificador de tamaño}

message defaultunit
{A no ser que haya especificado con 'M' (megabytes), 'G' (gigabytes), 'c'
(cilindros) o 's' sectores al final de la entrada, los tamaños y
compensaciones estan en %s.
}

message ptnsizes
{Ahora puede cambiar los tamaños para las particiones del sistema. Por
defecto se aloja todo el espacio a la particion root, sin embargo
podria querer separar /usr (archivos de sistema adicionales), /var
(archivos de log etc) o /home (directorios hogar de los usuarios).

El espacio libre sobrante sera añadido a la partición marcada con '+'.
}

message ptnheaders
{
       MB         Cilindros	Sectores  Sistema de archivos 
}

message askfsmount
{¿Punto de montaje?}

message askfssize
{¿Tamaño para %s en %s?}

message askunits
{Cambiar unidades de entrada (sectores/cilindros/MB)}

message NetBSD_partition_cant_change
{particion NetBSD}

message Whole_disk_cant_change
{Todo el disco}

message Boot_partition_cant_change
{Partición de arranque}

message add_another_ptn
{Añadir una partición definida por el usuario}

message fssizesok
{Aceptar tamaño de particiones.  Espacio libre %d %s, %d particiones libres.}

message fssizesbad
{Reducir tamaño de particiones por %d %s (%d sectores).}

message startoutsidedisk
{El valor del comienzo que ha especificado está mas allá del extremo del disco.
}

message endoutsidedisk
{Con este valor, el extremo de la partición está mas allá del extremo del disco.
Su tamaño de la partición se ha truncado a %d %s. 

Presione enter para continuar
}

message fspart
{Ahora tenemos sus particiones BSD-disklabel: 
Esta es su última oportunidad para cambiarlas.

}

message fspart_header
{  Inicio %3s Fin %3s   Tamaño %3s Tipo FS    Newfs Mont. Punto mont. 
   ---------- --------- ---------- ---------- ----- ----- -----------
}

message fspart_row
{%10d %9d %10d %-10s %-5s %-5s %s}

message show_all_unused_partitions
{Mostrar todas las particiones no usadas}

message partition_sizes_ok
{Tamaños de partición ok}

message edfspart
{Los valores actuales para la particion `%c' son, 
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
{Habilitar}

message restore
{Restaurar valores originales}

message Select_the_type
{Seleccione el tipo}

message other_types
{otros tipos}

message label_size
{%s
Valores especiales que pueden ser entrados para el valor del tamaño:
     -1:   usar hasta la parte final del disco NetBSD
   a-%c:   acabe esta particion donde la particion X empieza

tamaño (%s)}

message label_offset
{%s
Valores especiales que pueden ser entrados para el valor de compensado:
     -1:   empezar al principio de la parte NetBSD del disco
   a-%c:   empezar al final de la particion previa (a, b, ..., %c)

inicio (%s)}

message invalid_sector_number
{Número de sector malformado
}

message Select_file_system_block_size
{Seleccione tamaño de bloque del sistema de archivos}

message Select_file_system_fragment_size
{Seleccione tamaño de fragmento del sistema de archivos}

message packname
{Por favor entre un nombre para su disco NetBSD}

message lastchance
{Esta bien, ahora estamos preparados para instalar NetBSD en su disco duro (%s).
Nada se ha escrito todavia. Esta es su última oportunidad para salir del proceso
antes de que nada sea cambiado.

¿Deberíamos continuar?
}

message disksetupdone
{De acuerdo, la primera parte del procedimiento ha terminado.
Sysinst ha escrito un disklabel en el disco objetivo, y ha
newfs'ado y fsck'ado las nuevas particiones que ha especificado
para el disco objetivo. 

El paso siguiente es bajar y desempaquetar los archivos de
distribución.

Presione <return> para proceder.
}

message disksetupdoneupdate
{De acuerdo, la primera parte del procedimiento ha terminado.
sysinst ha escrito el disklabel al disco, y fsck'ado las
nuevas particiones que ha especificado para el disco objetivo. 

El paso siguiente es bajar y desempaquetar los archivos de
distribucion.

Presione <return> para proceder.
}

message openfail
{No se ha podido abrir %s, el mensaje de error ha sido: %s.
}

message statfail
{No se pueden obtener las propiedades de %s, el mensaje de error ha sido: %s.
}

message unlink_fail
{No he podido eliminar %s, el mensaje de error ha sido: %s.
}

message rename_fail
{No he podido renombrar %s a %s, el mensaje de error ha sido: %s.
}

message deleting_files
{Como parte del proceso de actualización, lo siguiente tiene que ser eliminado:
}

message deleting_dirs
{Como parte del proceso de actualización, los siguientes directorios
tienen que ser eliminados (renombraré los que no esten vacios):
}

message renamed_dir
{El directorio %s ha sido renombrado a %s porque no estaba vacio.
}

message cleanup_warn
{Limpieza de la instalación existente fallida. Esto puede causar fallos
en la extracción del set.
}

message nomount
{El tipo de partición de %c no es 4.2BSD o msdos, por lo tanto no tiene
un punto de montaje.}

message mountfail
{montaje del dispositivo /dev/%s%c en %s fallida.
}

message extractcomplete
{Extracción de los sets seleccionados para NetBSD-@@VERSION@@ completa.
El sistema ahora es capaz de arrancar desde el disco duro seleccionado.
Para completar la instalación, sysinst le dará la oportunidad de
configurar algunos aspectos esenciales.
}

message instcomplete
{Instalación de NetBSD-@@VERSION@@ completada. El sistema debería
arrancar desde el disco duro. Siga las instrucciones del documento
INSTALL sobre la configuración final de su sistema. La pagina man
de afterboot(8) es otra lectura recomendada; contiene una lista de
cosas a comprobar despues del primer inicio completo.

Como minimo, debe editar /etc/rc.conf para cumplir sus necesidades.
Vea /etc/defaults/rc.conf para los valores por defecto.
}

message upgrcomplete
{Actualización a NetBSD-@@VERSION@@ completada. Ahora tendrá que
seguir las instrucciones del documento INSTALL en cuanto a lo que
usted necesita hacer para conseguir tener su sistema configurado
de nuevo para su situación.
Recuerde (re)leer la pagina del man afterboot(8) ya que puede contener
nuevos apartados desde su ultima actualización.

Como minimo, debe editar rc.conf para su entorno local y cambiar
rc_configured=NO a rc_configured=YES o los reinicios se pararán en
single-user, y copie de nuevo los archivos de password (considerando
nuevas cuentas que puedan haber sido creadas para esta release) si
estuviera usando archivos de password locales.
}


message unpackcomplete
{Desempaquetamiento de sets adicionales de NetBSD-@@VERSION@@ completado.
Ahora necesitara seguir las instrucciones en el documento INSTALL para
tener su sistema reconfigurado para su situación.
La pagina de man afterboot(8) también puede serle de ayuda.

Como minimo, debe editar /etc/rc.conf para cumplir sus necesidades.
Vea /etc/defaults/rc.conf para los valores por defecto.
}

message distmedium
{Su disco ahora está preparado para instalar el nucleo y los sets de
distribución. Como aparece anotado en las notas INSTALL, tiene diversas
opciones. Para ftp o nfs, tiene que estar conectado a una red con acceso
a las maquinas apropiadas. Si no esta preparado para completar la
instalación en este momento, deberá seleccionar "ninguno" y será retornado
al menú principal. Cuando esté preparado mas tarde, deberá seleccionar
"actualizar" desde el menú principal para completar la instalación.
}

message distset
{La distribución NetBSD está dividida en una colección de sets. Hay
algunos sets básicos que son necesarios para todas las instalaciones y
hay otros sets que no son necesarios para todas las instalaciones.
Deberá escoger para instalar todas (Instalación completa) o
seleccionar desde los sets de distribución opcionales.
}

message ftpsource
{Lo siguiente es el sitio ftp, directorio, usuario y password actual
listo para usar. Si el "usuario" es "ftp", entonces el password no será
necesario.

host:		%s 
dir base:	%s 
dir de sets:	%s 
usuario:	%s 
password:	%s 
proxy:		%s 
}

message email
{dirección de e-mail}

message dev
{dispositivo}

message nfssource
{Entre el host del nfs i el directorio del servidor donde esté localizada
la distribución.
Recuerde, el directorio debe contener los archivos .tgz y debe ser
montable por nfs.

host:		%s
dir base:	%s
dir de sets:	%s
}

message nfsbadmount
{El directorio %s:%s no pudo ser montado por nfs.}

message cdromsource
{Entre el dispositivo de CDROM para ser usado y el directorio del CDROM
donde está localizada la distribución.
Recuerde, el directorio debe contener los archivos .tgz.

dispositivo:	%s
dir de sets:	%s
}

message localfssource
{Entre el dispositivo local desmontado y el directorio en ese dispositivo
donde está localizada la distribución.
Recuerde, el directorio debe contener los archivos .tgz.

dispositivo:	%s
sist de archiv:	%s
dir base:	%s
dir de sets:	%s
}

message localdir
{Entre el directorio local ya montado donde esta localizada la distribución.
Recuerde, el directorio debe contener los archivos .tgz.

dir base:	%s 
dir de sets:	%s
}

message filesys
{sistema de archivos}

message cdrombadmount
{El CDROM /dev/%s no ha podido ser montado.}

message localfsbadmount
{%s no ha podido ser montado en el dispositivo local %s.}

message badlocalsetdir
{%s no es un directorio}

message badsetdir
{%s no contiene los sets de instalación obligatorios etc.tgz 
y base.tgz.  ¿Está seguro de que ha introducido el directorio
correcto?}

message nonet
{No puedo encontrar ninguna interfaz de red para usar con NetBSD.
Volverá al menú anterior.
}

message netup
{Las siguientes interfaces están activas: %s
¿Conecta alguna de ellas al servidor requerido?}

message asknetdev
{He encontrado las siguientes interfaces de red: %s
\n¿Cual debería usar?}

message badnet
{No ha seleccionado una interfaz de las listadas. Por favor vuelva a
intentarlo.
Las siguientes interfaces de red estan disponibles: %s
\n¿Cual debería usar?}

message netinfo
{Para ser capaz de usar la red, necesitamos respuestas a lo siguiente:

}

message net_domain
{Su dominio DNS}

message net_host
{Su nombre del host}

message net_ip
{Su numero IPv4}

message net_ip_2nd
{Numero servidor IPv4}

message net_mask
{Mascara IPv4}

message net_namesrv6
{Servidor de nombres IPv6}

message net_namesrv
{Servidor de nombres IPv4}

message net_defroute
{Pasarela IPv4}

message net_media
{Tipo de medio de red}

message netok
{Ha entrado los siguientes valores.

Dominio DNS :		%s 
Nombre del Host:	%s
Interfaz primaria:	%s
Host IP:		%s
Mascara:		%s
Serv de nombres IPv4:	%s
Pasarela IPv4:		%s
Tipo de medio:		%s
}

message netok_slip
{Los siguientes valores son los que has metido. Son correctos?}

message netokv6
{IPv6 autoconf:		%s
Serv de nombres IPv6:	%s
}

message netok_ok
{¿Son correctos?}

message netagain
{Por favor reentre la información sobre su red. Sus ultimas respuestas
serán las predeterminadas.

}

message wait_network
{
Espere mientras las interfaces de red se levantan.
}

message resolv
{No se ha podido crear /etc/resolv.conf.  Instalación cancelada.
}

message realdir
{No se ha podido cambiar el directorio a %s: %s.  Instalación
cancelada.
}

message ftperror
{ftp no ha podido bajar un archivo.
¿Desea intentar de nuevo?}

message distdir
{¿Que directorio debería usar para %s? }

message delete_dist_files
{¿Quiere borrar los sets de NetBSD de %s?
(Puede dejarlos para instalar/actualizar un segundo sistema.)}

message verboseextract
{Durante el proceso de instalación, ¿que desea ver cuando
cada archivo sea extraido?
}

message notarfile
{El set %s no existe.}

message notarfile_ok
{¿Continuar extrayendo sets?}

message endtarok
{Todos los sets de distribución han sido desempaquetados
correctamente.}

message endtar
{Ha habido problemas desempaquetando los sets de distribución.
Su instalación está incompleta.

Ha seleccionado %d sets de distribución. %d sets no se han
encontrado y %d han sido saltados despues de que ocurriera un
error. De los %d que se han intentado, %d se han desempaquetado
sin errores y %d con errores.

La instalación está cancelada. Por favor compruebe de nuevo su
fuente de distribución y considere el reinstalar los sets desde
el menú principal.}

message abort
{Sus opciones han hecho imposible instalar NetBSD. Instalacion abortada.
}

message abortinst
{La distribución no ha sido correctamente cargada. Necesitará proceder
a mano. Instalación abortada.
}

message abortupgr
{La distribución no ha sido correctamente cargada. Necesitará proceder
a mano. Instalación abortada.
}

message abortunpack
{Desempaquetamiento de sets adicionales no satisfactoria. Necesitará
proceder a mano, o escoger una fuente diferente para los sets de
esta release y volver a intentarlo.
}

message createfstab
{¡Hay un gran problema!  No se puede crear /mnt/etc/fstab. ¡Saliendo!
}


message noetcfstab
{¡Ayuda! No hay /etc/fstab en el disco objetivo %s. Abortando actualización.
}

message badetcfstab
{¡Ayuda! No se puede analizar /etc/fstab en el disco objetivo %s.
Abortando actualización.
}

message X_oldexists
{No puedo dejar /usr/X11R6/bin/X como /usr/X11R6/bin/X.old, porque el
disco objetivo ya tiene un /usr/X11R6/bin/X.old. Por favor arregle esto
antes de continuar.

Una manera es iniciando una shell desde el menu de Utilidades, examinar
el objetivo /usr/X11R6/bin/X y /usr/X11R6/bin/X.old. Si
/usr/X11R6/bin/X.old es de una actualización completada, puede rm -f
/usr/X11R6/bin/X.old y reiniciar. O si /usr/X11R6/bin/X.old es de
una actualizacion reciente e incompleta, puede  rm -f /usr/X11R6/bin/X
y mv /usr/X11R6/bin/X.old a /usr/X11R6/bin/X.

Abortando actualización.}

message netnotup
{Ha habido un problema configurando la red. O su pasarela o su servidor
de nombres no son alcanzables por un ping. ¿Quiere configurar la red
de nuevo? (No le deja continuar de todos modos ni abortar el proceso
de instalación.)
}

message netnotup_continueanyway
{¿Le gustaría continuar el proceso de instalación de todos modos, y
asumir que la red está funcionando? (No aborta el proceso de
instalacióon.)
}

message makedev
{Creando nodos de dispositivo ...
}

message badfs
{Parece que /dev/%s%c no es un sistema de archivos BSD o el fsck
no ha sido correcto. La actualización ha sido abortada.  (Error
número %d.)
}

message badmount
{Su sistema de archivos /dev/%s%c no ha sido montado correctamente.
Actualización abortada.}

message rootmissing
{ el directorio raiz objetivo no existe %s.
}

message badroot
{El nuevo sistema de archivos raiz no ha pasado la comprobación básica.
 ¿Está seguro de que ha instalado todos los sets requeridos? 

}

message fddev
{¿Qué dispositivo de disquette quiere usar? }

message fdmount
{Por favor inserte el disquette que contiene el archivo "%s". }

message fdnotfound
{No se ha encontrado el archivo "%s" en el disco.  Por favor inserte
el disquette que lo contenga.

Si este era el ultimo disco de sets, presione "Set acabado" para
continuar con el siguiente set, si lo hay.}

message fdremount
{El disquette no ha sido montado correctamente. Deberia:

Intentar de nuevo e insertar el disquette que contenga "%s".

No cargar ningun otro archivo de este set y continuar con el siguiente,
si lo hay.

No cargar ningun otro archivo desde el disquette y abortar el proceso.
}

message mntnetconfig
{¿Es la información que ha entrado la adecuada para esta maquina
en operación regular o lo quiere instalar en /etc? }

message cur_distsets
{Lo siguiente es la lista de sets de distribución que será usada.

}

message cur_distsets_header
{   Set de distribución     Selecc.
   ------------------------ --------
}

message set_base
{Base}

message set_system
{Sistema (/etc)}

message set_compiler
{Herramientas de Compilador}

message set_games
{Juegos}

message set_man_pages
{Paginas del Manual en Linea}

message set_misc
{Miscelaneos}

message set_text_tools
{Herramientas de Procesamiento de Texto}

message set_X11
{Sets de X11}

message set_X11_base
{X11 base y clientes}

message set_X11_etc
{Configuración de X11}

message set_X11_fonts
{Fuentes de X11}

message set_X11_servers
{Servidores X11}

message set_X_contrib
{Clientes de X contrib}

message set_X11_prog
{Programación de X11}

message set_X11_misc
{X11 Misc.}

message cur_distsets_row
{%-27s %3s\n}

message select_all
{Seleccione todos los sets anteriores}

message select_none
{Des-seleccione todos los sets anteriores}

message install_selected_sets
{Instalar sets seleccionados}

message tarerror
{Ha habido un error extrayendo el archivo %s. Esto significa
que algunos archivos no han sido extraidos correctamente y su sistema
no estará completo.

¿Continuar extrayendo sets?}

message must_be_one_root
{Debe haber una sola partición marcada para ser montada en '/'.}

message partitions_overlap
{las particiones %c y %c se solapan.}

message edit_partitions_again
{

Puede editar la tabla de particiones a mano, o dejarlo y retornar al
menú principal.

¿Editar la tabla de particiones de nuevo?}

message not_regular_file
{El archivo de configuración %s no es un archivo regular.\n}

message out_of_memory
{Sin memoria (malloc fallido).\n}

message config_open_error
{No se ha podido abrir el archivo de configuración %s\n}

message config_read_error
{No se ha podido leer el archivo de configuración %s\n}

message cmdfail
{Comando
	%s
fallido. No puedo continuar.}

message upgradeparttype
{La unica partición adecuada que se ha encontrado para la instalación de
NetBSD es del tipo viejo de partición de NetBSD/386BSD/FreeBSD. ¿Quiere
cambiar el tipo de esta partición al nuevo tipo de partición de
solo-NetBSD?}

message choose_timezone
{Por favor escoja la zona horaria que se ajuste mejor de la siguiente
lista.
Presione RETURN para seleccionar una entrada.
Presione 'x' seguido de RETURN para salir de la selección de la
zona horaria.

 Defecto:	%s 
 Seleccionado:	%s 
 Hora local: 	%s %s 
}

message tz_back
{ Volver a la lista principal de zona horaria}

message choose_crypt
{Por favor seleccione el cifrador de password a usar. NetBSD puede ser
configurado para usar los esquemas DES, MD5 o Blowfish.

El esquema tradicional DES es compatible con la mayoria de sistemas operativos
tipo-Unix, pero solo los primeros 8 carácteres de cualquier password serán
reconocidos.
Los esquemas MD5 y Blowfish permiten passwords mas largos, y hay quien
discutiría si es mas seguro.

Si tiene una red o pretende usar NIS, por favor considere las capacidades
de otras maquinas en su red.

Si está actualizando y le gustaria mantener la configuración sin cambios,
escoja la ultima opción "no cambiar".
}

message swapactive
{El disco que ha seleccionado tiene una partición swap que puede que esté
en uso actualmente si su sistema tiene poca memoria. Como está a punto
de reparticionar este disco, esta partición swap será desactivada ahora.
Por favor tenga en cuenta que esto puede conducir a problemas de swap.
Si obtuviera algun error, por favor reinicie el sistema e intente de nuevo.}

message swapdelfailed
{sysinst ha fallado en la desactivación de la partición swap en el disco
que ha escogido para la instalación. Por favor reinicie e intente de nuevo.}

message rootpw
{El password de root del nuevo sistema instalado no ha sido fijado todavia,
y por eso está vacio. ¿Quiere fijar un password de root para el sistema ahora?}

message rootsh
{Ahora puede seleccionar que shell quiere usar para el usuario root. Por
defecto es /bin/csh, pero podria preferir otra.}

message postuseexisting
{
No olvide comprobar los puntos de montaje para cada sistema de archivos
que vaya a ser montado. Presione <return> para continuar.
}

message no_root_fs
{
No hay un sistema de archivos raiz definido. Necesitara al menos un punto
de montaje con "/".

Presione <return> para continuar.
}

message slattach {
Especifique los parametros de slattach
}

message Pick_an_option {Seleccione una opción para activar/desactivar.}
message Scripting {Scripting}
message Logging {Logging}

message Status  { Estado: }
message Command {Comando: }
message Running {Ejecutando}
message Finished {Acabado}
message Command_failed {Comando fallido}
message Command_ended_on_signal {Comando terminado en señal}

message NetBSD_VERSION_Install_System {Sistema de Instalación de NetBSD-@@VERSION@@}
message Exit_Install_System {Salir del Sistema de Instalación}
message Install_NetBSD_to_hard_disk {Instalar NetBSD al disco duro}
message Upgrade_NetBSD_on_a_hard_disk {Actualizar NetBSD en un disco duro}
message Re_install_sets_or_install_additional_sets {Re-instalar sets o instalar sets adicionales}
message Reboot_the_computer {Reiniciar la computadora}
message Utility_menu {Menú de utilidades}
message NetBSD_VERSION_Utilities {Utilidades de NetBSD-@@VERSION@@}
message Run_bin_sh {Ejecutar /bin/sh}
message Set_timezone {Ajustar zona horaria}
message Configure_network {Configurar red}
message Partition_a_disk {Particionar un disco}
message Logging_functions {Funciones de loggeo}
message Halt_the_system {Parar el sistema}
message yes_or_no {¿si o no?}
message Hit_enter_to_continue {Presione enter para continuar}
message Choose_your_installation {Seleccione su instalación}
message Set_Sizes {Ajustar tamaños de particiones NetBSD}
message Use_Existing {Usar tamaños de particiones existentes}
message Megabytes {Megabytes}
message Cylinders {Cilindros}
message Sectors {Sectores}
message Select_medium {Seleccione medio}
message ftp {FTP}
message http {HTTP}
message nfs {NFS}
message cdrom {CD-ROM / DVD}
message floppy {Disquette}
message local_fs {Sistema de Archivos Desmontado}
message local_dir {Directorio Local}
message Select_your_distribution {Seleccione su distribución}
message Full_installation {Instalación completa}
message Custom_installation {Instalación personalizada}
message Change {Cambiar}
message hidden {** oculto **}
message Host {Host}
message Base_dir {Directorio base}
message Set_dir {Directorio de sets}
message Directory {Directorio}
message User {Usuario}
message Password {Password}
message Proxy {Proxy}
message Get_Distribution {Bajar Distribución}
message Continue {Continuar}
message What_do_you_want_to_do {¿Qué desea hacer?}
message Try_again {Reintentar}
message Give_up {Give up}
message Ignore_continue_anyway {Ignorar, continuar de todos modos}
message Set_finished {Finalizar}
message Abort_install {Abortar instalación}
message Password_cipher {Cifrador de password}
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
message Root_shell {Root shell}
message Select_set_extraction_verbosity {Seleccione detalle de extracción de sets}
message Progress_bar {Barra de progreso (recomendado)}
message Silent {Silencioso}
message Verbose {Listado de nombres de archivo detallado (lento)}

.if AOUT2ELF
message aoutfail
{El directorio donde las librerias compartidas antiguas a.out deberian ser
movidas no ha podido ser creado. Por favor intente el proceso de actualizacion
de nuevo y asegurese de que ha montado todos los sistemas de archivos.}

message emulbackup
{El directorio /emul/aout o /emul de su sistema tiene un enlace simbolico
apuntando a un sistema de archivos desmontado. Se le ha dado la extension '.old'.
Once you bring your upgraded system back up, you may need to take care
of merging the newly created /emul/aout directory with the old one.
}
.endif
