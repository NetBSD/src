/*	$NetBSD: msg.md.es,v 1.1.6.2 2014/08/20 00:05:16 tls Exp $	*/

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

/* MD Message catalog -- spanish, mac68k version */

message md_hello
{
}

message md_may_remove_boot_medium
{
}

message fullpart
{Ahora vamos a instalar NetBSD en el disco %s. Debería escoger
entre instalar NetBSD en el disco entero o en una parte del disco.
¿Que le gustaría hacer?
}

message nodiskmap
{Parece que su disco, %s, no ha sido formateado e inicializado para
usar en MacOS.  Este programa obtiene la disposición de su disco
leyendo el Mapa de Particiones del Disco que es escrito por el
formateador de disco cuando prepara el disco para usar con el
sistema Macintosh. Sus opciones son:

* Abortar la instalación. Puede formatear y definir una estructura
preliminar de particiones con cualquier formateador de disco.
Las particiones no tienen que estar definidas como del tipo A/UX;
el proceso de instalación de NetBSD le permitira redefinirlas
si fuera necesario.

* Permitir a sysinst inicializar el Mapa de Particiones del Disco
por usted. El disco no será iniciable por MacOS y puede que no sea
usable por MacOS a no ser que los drivers de MacOS estén instalados
con un formateador compatible con MacOS.

}

message okwritediskmap
{Sysinst intentará inicializar su disco con un nuevo Mapa de Particiones
de Disco usando los valores obtenidos del driver del disco.  Este mapa
por defecto incluira una definición minima de Block0, un Mapa de
Particiones permitiendo hasta 15 particiones de disco, y particiones
pre-definidas para el Mapa, drivers de disco, y una partición minima
de MacOS HFS. El resto del disco será marcado como disponible para uso.
Las particiones de drivers de disco no serán inicializadas asi que el
volumen no será iniciable por MacOS.  Si escoge proceder con la
instalación desde este punto deberá editar este nuevo Mapa de Particiones
para ajustar NetBSD a sus necesidades.  El Mapa no será escrito a disco
hasta que no haya completado el ajuste de sus particiones.

¿Deberíamos continuar?}

message part_header
{Part     Inicio     Tamaño TipoFS Uso       Punt Montaj (Nomb)
----  ---------- ---------- ------ --------- ------------------\n}

message part_row
{%4s%c %10d %10d %6s %-9s %s\n}

message ovrwrite
{Actualmente su disco tiene al menos una partición MacOS HFS.
Sobreescribiendo el disco entero eliminará la partición y puede hacer
el disco inusable bajo MacOS.  Debería considerar el crear una partición
sola MacOS HFS para asegurarse de que el disco pueda ser montado bajo
MacOS.  Este será un buen sitio para mantener una copia de la aplicación
de arranque de NetBSD/mac68k usada para iniciar NetBSD desde MacOS.

¿Está realmente seguro de que quiere sobreescribir la(s) particion(es)
MacOS HFS?
}

message editparttable
{Editar Tabla de Particiones de Disco: El Mapa en el disco ha sido
escaneada para todas las particiones a nivel de usuario, pero solo
se muestran las usables por NetBSD.
La tabla de particiones actualmente se muestra como:

}

message split_part
{La partición del disco que ha escogido sera partida en dos particiones.
Una partición será del tipo Apple_Scratch y la otra sera del tipo
Apple_Libre.  Entonces debera cambiar una de las dos particiones en una
de otro tipo.  Si la parte Apple_Libre es fisicamente adyacente a otra
particion que tambien sea del tipo Apple_Libre, las dos seran combinadas
automaticamente en una sola particion del tipo Apple_Free.

Hay %d bloques en la particion seleccionada.  Entre el numero de bloques
de disco que desearia alojar a la particion Apple_Scratch.
Entrar un valor de cero, o un valor mayor que el numero disponible
dejara la particion sin cambios.

}

message scratch_size
{Tamaño de la parte Apple_Scratch de la particion}

message diskfull
{Ha intentado separar una particion de disco existente en dos partes
pero no hay espacio en el Mapa de Particiones de Disco para mapear
la segunda parte. Esto es probablemente porque su formateador de disco
no reservo espacio adicional en el Mapa de Particiones de Disco original
para futuras modificaciones como esta.
Sus opciones en este punto son:

* Abandonar y esperar partir esta particion, con el Mapa de Particiones
de Disco actual, o

* Permitir a sysinst instalar el nuevo Mapa de Particiones en su disco.
Esto destruira todos los datos en todas las particiones del volumen.
¡USAR CON CUIDADO!

¿Desea Abandonar el particionaje de esta particion?}

message custom_mount_point
{Especifique un Punto de Montaje para la particion de disco
seleccionada.  Este deberia ser un nombre unico, empezando con una "/",
que no sera usado para ninguna otra particion.

}

message sanity_check
{Se han encontrado las siguientes anomalias cuando se ajustaba el
disklabel para su volumen. Deberia escoger ignorarlas y continuar,
pero hacer eso podria resultar en una instalacion fallida. Los
siguientes problemas han sido encontrados:

}

message dodiskmap
{Ajustando el Mapa de Particiones de Disco ...
}

message label_error
{La nueva etiqueta del disco no concuerda con la actual in-core.
Cualquier intento de proceder probablemente resultara en daño a cualquier
particion pre-existente en disco. Sin embargo su nuevo Mapa de Particiones
de Disco ha sido escrita a disco y estara disponible la proxima vez que
NetBSD sea iniciado.  Por favor reinicie inmediatamente y vuelva a iniciar
el Proceso de Instalacion.
}

.if debug
message mapdebug
{Mapa de Particiones:
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
{Ejecutando newfs en particion: %s\n
}

message geomdebug
{Llamando: %s %s\n
}

message geomdebug2
{Llamando: %s %d\n"
}
.endif

message partdebug
{Particion %s%c:
Compensacion: %d
Tamaño: %d
}

message disksetup_no_root
{* ¡Ninguna Particion de Disco definida para Root!\n}

message disksetup_multiple_roots
{* Multiples Particiones de Disco definidas para Root\n}

message disksetup_no_swap
{* ¡Ninguna Particion de Disco definida para SWAP!\n}

message disksetup_multiple_swaps
{* Multiples Particiones de Disco definidas para SWAP\n}

message disksetup_part_beginning
{* La Particion %s%c empieza mas alla del final del disco\n}

message disksetup_part_size
{* La Particion %s%c se extiende mas alla del final del disco\n}

message disksetup_noerrors
{** No se han hallado errores ni anomalias en el disco de setup.\n}

message parttable_fix_fixing
{Arreglando particion %s%c\n}

message parttable_fix_fine
{¡La particion %s%c ya es correcta!\n}

message dump_line
{%s\n}

message set_kernel_1
{Nucleo (GENERIC)}
message set_kernel_2
{Nucleo (GENERICSBC)}

