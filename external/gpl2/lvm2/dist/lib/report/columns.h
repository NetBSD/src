/*	$NetBSD: columns.h,v 1.1.1.3 2009/12/02 00:26:45 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This file defines the fields (columns) for the reporting commands
 * (pvs/vgs/lvs).
 */
/*
 * The 'FIELD' macro arguments are defined as follows:
 * 1. report_type.  An enum value that selects a specific
 * struct dm_report_object_type in the _report_types array.  The value is
 * used to select the containing base object address (see *obj_get*
 * functions) for any data values of any field in the report.
 * 2. Containing struct.  The structure that either contains the field data
 * as a member or should be used to obtain the field data.  The containing
 * struct should match the base object of the report_type.
 * 3. Field type.  This must be either 'STR' or 'NUM'.
 * 4. Report heading.  This is the field heading that is displayed by the
 * reporting commands.
 * 5. Data value pointer.  This argument is always a member of the
 * containing struct.  It may point directly to the data value (for example,
 * lv_uuid - see _uuid_disp()) or may be used to derive the data value (for
 * example, seg_count - see _lvsegcount_disp()).  In the FIELD macro
 * definition, it is used in an offset calculation to derive the offset to
 * the data value from the containing struct base address.  Note that in some
 * cases, the argument is the first member of the struct, in which case the
 * data value pointer points to the start of the struct itself (for example,
 * 'lvid' field of struct 'lv').
 * 6. Minimum display width.  This is the minimum width used to display
 * the field value, typically matching the width of the column heading.
 * 7. Display function identifier.  Used to derive the full name of the
 * function that displays this field.  Derivation is done by appending '_'
 * then prepending this argument to '_disp'.  For example, if this argument
 * is 'uuid', the display function is _uuid_disp().  Adding a new field may
 * require defining a new display function (for example _myfieldname_disp()),
 * or re-use of an existing one (for example, _uint32_disp()).
 * 8. Unique format identifier / field id.  This name must be unique and is
 * used to select fields via '-o' in the reporting commands (pvs/vgs/lvs).
 * The string used to specify the field - the 'id' member of
 * struct dm_report_field_type.
 * 9. Description of field.  This is a brief (ideally <= 52 chars) description
 * of the field used in the reporting commands.
 */

/* *INDENT-OFF* */
FIELD(LVS, lv, STR, "LV UUID", lvid.id[1], 38, uuid, "lv_uuid", "Unique identifier.")
FIELD(LVS, lv, STR, "LV", lvid, 4, lvname, "lv_name", "Name.  LVs created for internal use are enclosed in brackets.")
FIELD(LVS, lv, STR, "Attr", lvid, 4, lvstatus, "lv_attr", "Various attributes - see man page.")
FIELD(LVS, lv, NUM, "Maj", major, 3, int32, "lv_major", "Persistent major number or -1 if not persistent.")
FIELD(LVS, lv, NUM, "Min", minor, 3, int32, "lv_minor", "Persistent minor number or -1 if not persistent.")
FIELD(LVS, lv, NUM, "Rahead", lvid, 6, lvreadahead, "lv_read_ahead", "Read ahead setting in current units.")
FIELD(LVS, lv, STR, "KMaj", lvid, 4, lvkmaj, "lv_kernel_major", "Currently assigned major number or -1 if LV is not active.")
FIELD(LVS, lv, STR, "KMin", lvid, 4, lvkmin, "lv_kernel_minor", "Currently assigned minor number or -1 if LV is not active.")
FIELD(LVS, lv, NUM, "KRahead", lvid, 7, lvkreadahead, "lv_kernel_read_ahead", "Currently-in-use read ahead setting in current units.")
FIELD(LVS, lv, NUM, "LSize", size, 5, size64, "lv_size", "Size of LV in current units.")
FIELD(LVS, lv, NUM, "#Seg", lvid, 4, lvsegcount, "seg_count", "Number of segments in LV.")
FIELD(LVS, lv, STR, "Origin", lvid, 6, origin, "origin", "For snapshots, the origin device of this LV.")
FIELD(LVS, lv, NUM, "OSize", lvid, 5, originsize, "origin_size", "For snapshots, the size of the origin device of this LV.")
FIELD(LVS, lv, NUM, "Snap%", lvid, 6, snpercent, "snap_percent", "For snapshots, the percentage full if LV is active.")
FIELD(LVS, lv, NUM, "Copy%", lvid, 6, copypercent, "copy_percent", "For mirrors and pvmove, current percentage in-sync.")
FIELD(LVS, lv, STR, "Move", lvid, 4, movepv, "move_pv", "For pvmove, Source PV of temporary LV created by pvmove.")
FIELD(LVS, lv, STR, "Convert", lvid, 7, convertlv, "convert_lv", "For lvconvert, Name of temporary LV created by lvconvert.")
FIELD(LVS, lv, STR, "LV Tags", tags, 7, tags, "lv_tags", "Tags, if any.")
FIELD(LVS, lv, STR, "Log", lvid, 3, loglv, "mirror_log", "For mirrors, the LV holding the synchronisation log.")
FIELD(LVS, lv, STR, "Modules", lvid, 7, modules, "modules", "Kernel device-mapper modules required for this LV.")

FIELD(LABEL, pv, STR, "Fmt", id, 3, pvfmt, "pv_fmt", "Type of metadata.")
FIELD(LABEL, pv, STR, "PV UUID", id, 38, uuid, "pv_uuid", "Unique identifier.")
FIELD(LABEL, pv, NUM, "DevSize", dev, 7, devsize, "dev_size", "Size of underlying device in current units.")
FIELD(LABEL, pv, STR, "PV", dev, 10, dev_name, "pv_name", "Name.")
FIELD(LABEL, pv, NUM, "PMdaFree", id, 9, pvmdafree, "pv_mda_free", "Free metadata area space on this device in current units.")
FIELD(LABEL, pv, NUM, "PMdaSize", id, 9, pvmdasize, "pv_mda_size", "Size of smallest metadata area on this device in current units.")

FIELD(PVS, pv, NUM, "1st PE", pe_start, 7, size64, "pe_start", "Offset to the start of data on the underlying device.")
FIELD(PVS, pv, NUM, "PSize", id, 5, pvsize, "pv_size", "Size of PV in current units.")
FIELD(PVS, pv, NUM, "PFree", id, 5, pvfree, "pv_free", "Total amount of unallocated space in current units.")
FIELD(PVS, pv, NUM, "Used", id, 4, pvused, "pv_used", "Total amount of allocated space in current units.")
FIELD(PVS, pv, STR, "Attr", status, 4, pvstatus, "pv_attr", "Various attributes - see man page.")
FIELD(PVS, pv, NUM, "PE", pe_count, 3, uint32, "pv_pe_count", "Total number of Physical Extents.")
FIELD(PVS, pv, NUM, "Alloc", pe_alloc_count, 5, uint32, "pv_pe_alloc_count", "Total number of allocated Physical Extents.")
FIELD(PVS, pv, STR, "PV Tags", tags, 7, tags, "pv_tags", "Tags, if any.")
FIELD(PVS, pv, NUM, "#PMda", id, 5, pvmdas, "pv_mda_count", "Number of metadata areas on this device.")

FIELD(VGS, vg, STR, "Fmt", cmd, 3, vgfmt, "vg_fmt", "Type of metadata.")
FIELD(VGS, vg, STR, "VG UUID", id, 38, uuid, "vg_uuid", "Unique identifier.")
FIELD(VGS, vg, STR, "VG", name, 4, string, "vg_name", "Name.")
FIELD(VGS, vg, STR, "Attr", cmd, 5, vgstatus, "vg_attr", "Various attributes - see man page.")
FIELD(VGS, vg, NUM, "VSize", cmd, 5, vgsize, "vg_size", "Total size of VG in current units.")
FIELD(VGS, vg, NUM, "VFree", cmd, 5, vgfree, "vg_free", "Total amount of free space in current units.")
FIELD(VGS, vg, STR, "SYS ID", system_id, 6, string, "vg_sysid", "System ID indicating when and where it was created.")
FIELD(VGS, vg, NUM, "Ext", extent_size, 3, size32, "vg_extent_size", "Size of Physical Extents in current units.")
FIELD(VGS, vg, NUM, "#Ext", extent_count, 4, uint32, "vg_extent_count", "Total number of Physical Extents.")
FIELD(VGS, vg, NUM, "Free", free_count, 4, uint32, "vg_free_count", "Total number of unallocated Physical Extents.")
FIELD(VGS, vg, NUM, "MaxLV", max_lv, 5, uint32, "max_lv", "Maximum number of LVs allowed in VG or 0 if unlimited.")
FIELD(VGS, vg, NUM, "MaxPV", max_pv, 5, uint32, "max_pv", "Maximum number of PVs allowed in VG or 0 if unlimited.")
FIELD(VGS, vg, NUM, "#PV", pv_count, 3, uint32, "pv_count", "Number of PVs.")
FIELD(VGS, vg, NUM, "#LV", cmd, 3, lvcount, "lv_count", "Number of LVs.")
FIELD(VGS, vg, NUM, "#SN", cmd, 3, snapcount, "snap_count", "Number of snapshots.")
FIELD(VGS, vg, NUM, "Seq", seqno, 3, uint32, "vg_seqno", "Revision number of internal metadata.  Incremented whenever it changes.")
FIELD(VGS, vg, STR, "VG Tags", tags, 7, tags, "vg_tags", "Tags, if any.")
FIELD(VGS, vg, NUM, "#VMda", cmd, 5, vgmdas, "vg_mda_count", "Number of metadata areas in use by this VG.")
FIELD(VGS, vg, NUM, "VMdaFree", cmd, 9, vgmdafree, "vg_mda_free", "Free metadata area space for this VG in current units.")
FIELD(VGS, vg, NUM, "VMdaSize", cmd, 9, vgmdasize, "vg_mda_size", "Size of smallest metadata area for this VG in current units.")

FIELD(SEGS, seg, STR, "Type", list, 4, segtype, "segtype", "Type of LV segment.")
FIELD(SEGS, seg, NUM, "#Str", area_count, 4, uint32, "stripes", "Number of stripes or mirror legs.")
FIELD(SEGS, seg, NUM, "Stripe", stripe_size, 6, size32, "stripesize", "For stripes, amount of data placed on one device before switching to the next.")
FIELD(SEGS, seg, NUM, "Stripe", stripe_size, 6, size32, "stripe_size", "For stripes, amount of data placed on one device before switching to the next.")
FIELD(SEGS, seg, NUM, "Region", region_size, 6, size32, "regionsize", "For mirrors, the unit of data copied when synchronising devices.")
FIELD(SEGS, seg, NUM, "Region", region_size, 6, size32, "region_size", "For mirrors, the unit of data copied when synchronising devices.")
FIELD(SEGS, seg, NUM, "Chunk", list, 5, chunksize, "chunksize", "For snapshots, the unit of data used when tracking changes.")
FIELD(SEGS, seg, NUM, "Chunk", list, 5, chunksize, "chunk_size", "For snapshots, the unit of data used when tracking changes.")
FIELD(SEGS, seg, NUM, "Start", list, 5, segstart, "seg_start", "Offset within the LV to the start of the segment in current units.")
FIELD(SEGS, seg, NUM, "Start", list, 5, segstartpe, "seg_start_pe", "Offset within the LV to the start of the segment in physical extents.")
FIELD(SEGS, seg, NUM, "SSize", list, 5, segsize, "seg_size", "Size of segment in current units.")
FIELD(SEGS, seg, STR, "Seg Tags", tags, 8, tags, "seg_tags", "Tags, if any.")
FIELD(SEGS, seg, STR, "PE Ranges", list, 9, peranges, "seg_pe_ranges", "Ranges of Physical Extents of underlying devices in command line format.")
FIELD(SEGS, seg, STR, "Devices", list, 7, devices, "devices", "Underlying devices used with starting extent numbers.")

FIELD(PVSEGS, pvseg, NUM, "Start", pe, 5, uint32, "pvseg_start", "Physical Extent number of start of segment.")
FIELD(PVSEGS, pvseg, NUM, "SSize", len, 5, uint32, "pvseg_size", "Number of extents in segment.")
/* *INDENT-ON* */
