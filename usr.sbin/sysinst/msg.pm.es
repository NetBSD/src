/*	$NetBSD: msg.pm.es,v 1.4 2020/11/05 11:10:11 martin Exp $	*/

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

/* extended partition manager message catalog -- Spanish, machine independent */

message fillzeros {Llenar con ceros}
message fillrandom {Llene los datos al azar}
message raid0 {0 - Sin paridad, creación de bandas sólo es simple.}
message raid1 {1 - Creación de reflejos. La paridad es el espejo.}
message raid4 {4 - Striping con paridad almacenada en el último componente. component.}
message raid5 {5 - Striping con paridad en los componentes de todos. components.}

message wannaunblock {El dispositivo está bloqueado. ¿Quiere forzar a desbloquear y continuar? unblock it and continue?}
message wannatry {¿Quieres probar?}
message create_cgd {Crear cifrado de volumen (CGD)}
message create_vnd {Crear imagen de disco virtual (VND)}
message create_vg {Crear grupo de volúmenes (LVM VG)}
message create_lv {Crear volúmenes lógicos}
message create_raid {Crear RAID por software}
message updpmlist {Actualizar lista de dispositivos}
message savepm {Guardar los cambios}
message pmblocked {BLOQUEADO}
message pmunchanged {SIN USO}
message pmused {UTILIZA}
message pmmounted {(montado)}
message pmunused {(sin usar)}
message pmgptdisk {Disco con GPT}

message finishpm {Finalizar el particionado}
message limitcount {Límite para el número de dispositivos se llegó!}
message invaliddev {No válido dispositivo!}
message avdisks {Discos disponibles:}
message nofreedev {No se puede asignar nodo de dispositivo!}
message partman_header
{Partition Manager. Todos los discos, particiones y etc muestra allí.
Si desea utilizar RAID, LVM o CGD, siga estos pasos:
1) Crear particiones con el tipo necesario;
2) Crear RAID / LVM VG / CGD el uso de estas particiones; 3) Guárdalo;
4) Crear particiones de volúmenes RAID / CGD o lógico de LVM.}

/* used to form strings like: "vnd0 on /var/tmp/disk1.img" */
message        pm_menu_on {on}
/* Called with: 			Example
 *  $0 = device name			raid0
 *  $1 = raid level		       1
 */
message raid_menufmt {$0 (nivel $1)}
message raid_err_menufmt {RAID VACIO!}
message raid_disks_fmt {Discos}
message raid_spares_fmt {Piezas de recambio}
message raid_level_fmt {RAID de nivel}
message raid_numrow_fmt {numRow}
message raid_numcol_fmt {numCol}
message raid_numspare_fmt {numSpare}
message raid_sectpersu_fmt {sectPerSU}
message raid_superpar_fmt {SUsPerParityUnit}
message raid_superrec_fmt {SUsPerReconUnit}
message raid_nomultidim {Matrices multidimensionales no son compatibles!}
message raid_numrow_ask {numRow?}
message raid_numcol_ask {numCol?}
message raid_numspare_ask {numSpare?}
message raid_sectpersu_ask {sectPerSU?}
message raid_superpar_ask {SUsPerParityUnit?}
message raid_superrec_ask {SUsPerReconUnit?}
message raid_disks {Los discos en RAID}
message vnd_err_menufmt {CAMINO NO DEFINIDO!}
message vnd_assign {ASSIGN}
message vnd_path_fmt {Ruta del archivo}
message vnd_assign_fmt {Create new image}
message vnd_size_fmt {Tamaño}
message vnd_ro_fmt {Sólo lectura}
message vnd_geom_fmt {Establecer la geometría de la mano}
message vnd_bps_fmt {Bytes por sector}
message vnd_spt_fmt {Sectores por pista}
message vnd_tpc_fmt {Pistas por cilindro}
message vnd_cyl_fmt {Cilindros}
message vnd_path_ask {Ruta de acceso?}
message vnd_size_ask {Tamaño (MB)?}
message vnd_bps_ask {Bytes por sector?}
message vnd_spt_ask {Sectores por pista?}
message vnd_tpc_ask {Pistas por cilindro?}
message vnd_cyl_ask {Cilindros}
message cgd_err_menufmt {DISCO NO SE DEFINE!}
message cgd_dev_fmt {Dispositivo de base}
message cgd_enc_fmt {Encriptación}
message cgd_key_fmt {Tamaño de la clave}
message cgd_iv_fmt {Algoritmo IV}
message cgd_keygen_fmt {La generación de claves}
message cgd_verif_fmt {Método de verificación}
message lvm_disks {Discos en VG}
message lvm_err_menufmt {VACIAR VG!}
message lvm_disks_fmt {PV's}
message lvm_name_fmt {Nombre}
message lvm_maxlv_fmt {MaxLogicalVolumes}
message lvm_maxpv_fmt {MaxPhysicalVolumes}
message lvm_extsiz_fmt {PhysicalExtentSize}
message lvm_name_ask {Nombre?}
message lvm_maxlv_ask {MaxLogicalVolumes?}
message lvm_maxpv_ask {MaxPhysicalVolumes?}
message lvm_extsiz_ask {PhysicalExtentSize (MB)?}
message lvmlv_menufmt {El volumen lógico}
message lvmlv_name_fmt {Nombre}
message lvmlv_size_fmt {Tamaño}
message lvmlv_ro_fmt {Sólo lectura}
message lvmlv_cont_fmt {Contigua}
message lvmlv_extnum_fmt {LogicalExtentsNumber}
message lvmlv_minor_fmt {Menor número}
message lvmlv_mirrors_fmt {Espejos}
message lvmlv_regsiz_fmt {MirrorLogRegionSize}
message lvmlv_pers_fmt {Número de persistente menor}
message lvmlv_readahsect_fmt {ReadAheadSectors}
message lvmlv_stripes_fmt {Rayas}
message lvmlv_stripesiz_fmt {Stripesize}
message lvmlv_zero_fmt {Puesta a cero de la primera KB}
message lvmlv_name_ask {Nombre?}
message lvmlv_size_ask {Tamaño (MB)?}
message lvmlv_extnum_ask {LogicalExtentsNumber?}
message lvmlv_minor_ask {Número menor de edad?}
message lvmlv_mirrors_ask {Espejos?}
message lvmlv_regsiz_ask {MirrorLogRegionSize?}
message lvmlv_readahsect_ask {ReadAheadSectors?}
message lvmlv_stripes_ask {Rayas?}

message notsupported {Operación no admitida!}
message edit_parts {Editar particiones}
message switch_parts {Switch partitioning scheme}
message fmtasraid {Formato como RAID}
message fmtaslvm {Formato como LVM PV}
message encrypt {Cifrar (CGD)}
message erase {Borrado seguro}
message undo {Deshacer los cambios}
message unconfig {Desconfigurar}
message edit {Editar}
message doumount {Fuerza umount}

