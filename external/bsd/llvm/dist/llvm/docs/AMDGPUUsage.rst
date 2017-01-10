==============================
User Guide for AMDGPU Back-end
==============================

Introduction
============

The AMDGPU back-end provides ISA code generation for AMD GPUs, starting with
the R600 family up until the current Volcanic Islands (GCN Gen 3).

Refer to `AMDGPU section in Architecture & Platform Information for Compiler Writers <CompilerWriterInfo.html#amdgpu>`_
for additional documentation.

Conventions
===========

Address Spaces
--------------

The AMDGPU back-end uses the following address space mapping:

   ============= ============================================
   Address Space Memory Space
   ============= ============================================
   0             Private
   1             Global
   2             Constant
   3             Local
   4             Generic (Flat)
   5             Region
   ============= ============================================

The terminology in the table, aside from the region memory space, is from the
OpenCL standard.


Assembler
=========

AMDGPU backend has LLVM-MC based assembler which is currently in development.
It supports Southern Islands ISA, Sea Islands and Volcanic Islands.

This document describes general syntax for instructions and operands. For more
information about instructions, their semantics and supported combinations
of operands, refer to one of Instruction Set Architecture manuals.

An instruction has the following syntax (register operands are
normally comma-separated while extra operands are space-separated):

*<opcode> <register_operand0>, ... <extra_operand0> ...*


Operands
--------

The following syntax for register operands is supported:

* SGPR registers: s0, ... or s[0], ...
* VGPR registers: v0, ... or v[0], ...
* TTMP registers: ttmp0, ... or ttmp[0], ...
* Special registers: exec (exec_lo, exec_hi), vcc (vcc_lo, vcc_hi), flat_scratch (flat_scratch_lo, flat_scratch_hi)
* Special trap registers: tba (tba_lo, tba_hi), tma (tma_lo, tma_hi)
* Register pairs, quads, etc: s[2:3], v[10:11], ttmp[5:6], s[4:7], v[12:15], ttmp[4:7], s[8:15], ...
* Register lists: [s0, s1], [ttmp0, ttmp1, ttmp2, ttmp3]
* Register index expressions: v[2*2], s[1-1:2-1]
* 'off' indicates that an operand is not enabled

The following extra operands are supported:

* offset, offset0, offset1
* idxen, offen bits
* glc, slc, tfe bits
* waitcnt: integer or combination of counter values
* VOP3 modifiers:

  - abs (\| \|), neg (\-)

* DPP modifiers:

  - row_shl, row_shr, row_ror, row_rol
  - row_mirror, row_half_mirror, row_bcast
  - wave_shl, wave_shr, wave_ror, wave_rol, quad_perm
  - row_mask, bank_mask, bound_ctrl

* SDWA modifiers:

  - dst_sel, src0_sel, src1_sel (BYTE_N, WORD_M, DWORD)
  - dst_unused (UNUSED_PAD, UNUSED_SEXT, UNUSED_PRESERVE)
  - abs, neg, sext

DS Instructions Examples
------------------------

.. code-block:: nasm

  ds_add_u32 v2, v4 offset:16
  ds_write_src2_b64 v2 offset0:4 offset1:8
  ds_cmpst_f32 v2, v4, v6
  ds_min_rtn_f64 v[8:9], v2, v[4:5]


For full list of supported instructions, refer to "LDS/GDS instructions" in ISA Manual.

FLAT Instruction Examples
--------------------------

.. code-block:: nasm

  flat_load_dword v1, v[3:4]
  flat_store_dwordx3 v[3:4], v[5:7]
  flat_atomic_swap v1, v[3:4], v5 glc
  flat_atomic_cmpswap v1, v[3:4], v[5:6] glc slc
  flat_atomic_fmax_x2 v[1:2], v[3:4], v[5:6] glc

For full list of supported instructions, refer to "FLAT instructions" in ISA Manual.

MUBUF Instruction Examples
---------------------------

.. code-block:: nasm

  buffer_load_dword v1, off, s[4:7], s1
  buffer_store_dwordx4 v[1:4], v2, ttmp[4:7], s1 offen offset:4 glc tfe
  buffer_store_format_xy v[1:2], off, s[4:7], s1
  buffer_wbinvl1
  buffer_atomic_inc v1, v2, s[8:11], s4 idxen offset:4 slc

For full list of supported instructions, refer to "MUBUF Instructions" in ISA Manual.

SMRD/SMEM Instruction Examples
-------------------------------

.. code-block:: nasm

  s_load_dword s1, s[2:3], 0xfc
  s_load_dwordx8 s[8:15], s[2:3], s4
  s_load_dwordx16 s[88:103], s[2:3], s4
  s_dcache_inv_vol
  s_memtime s[4:5]

For full list of supported instructions, refer to "Scalar Memory Operations" in ISA Manual.

SOP1 Instruction Examples
--------------------------

.. code-block:: nasm

  s_mov_b32 s1, s2
  s_mov_b64 s[0:1], 0x80000000
  s_cmov_b32 s1, 200
  s_wqm_b64 s[2:3], s[4:5]
  s_bcnt0_i32_b64 s1, s[2:3]
  s_swappc_b64 s[2:3], s[4:5]
  s_cbranch_join s[4:5]

For full list of supported instructions, refer to "SOP1 Instructions" in ISA Manual.

SOP2 Instruction Examples
-------------------------

.. code-block:: nasm

  s_add_u32 s1, s2, s3
  s_and_b64 s[2:3], s[4:5], s[6:7]
  s_cselect_b32 s1, s2, s3
  s_andn2_b32 s2, s4, s6
  s_lshr_b64 s[2:3], s[4:5], s6
  s_ashr_i32 s2, s4, s6
  s_bfm_b64 s[2:3], s4, s6
  s_bfe_i64 s[2:3], s[4:5], s6
  s_cbranch_g_fork s[4:5], s[6:7]

For full list of supported instructions, refer to "SOP2 Instructions" in ISA Manual.

SOPC Instruction Examples
--------------------------

.. code-block:: nasm

  s_cmp_eq_i32 s1, s2
  s_bitcmp1_b32 s1, s2
  s_bitcmp0_b64 s[2:3], s4
  s_setvskip s3, s5

For full list of supported instructions, refer to "SOPC Instructions" in ISA Manual.

SOPP Instruction Examples
--------------------------

.. code-block:: nasm

  s_barrier
  s_nop 2
  s_endpgm
  s_waitcnt 0 ; Wait for all counters to be 0
  s_waitcnt vmcnt(0) & expcnt(0) & lgkmcnt(0) ; Equivalent to above
  s_waitcnt vmcnt(1) ; Wait for vmcnt counter to be 1.
  s_sethalt 9
  s_sleep 10
  s_sendmsg 0x1
  s_sendmsg sendmsg(MSG_INTERRUPT)
  s_trap 1

For full list of supported instructions, refer to "SOPP Instructions" in ISA Manual.

Unless otherwise mentioned, little verification is performed on the operands
of SOPP Instrucitons, so it is up to the programmer to be familiar with the
range or acceptable values.

Vector ALU Instruction Examples
-------------------------------

For vector ALU instruction opcodes (VOP1, VOP2, VOP3, VOPC, VOP_DPP, VOP_SDWA),
the assembler will automatically use optimal encoding based on its operands.
To force specific encoding, one can add a suffix to the opcode of the instruction:

* _e32 for 32-bit VOP1/VOP2/VOPC
* _e64 for 64-bit VOP3
* _dpp for VOP_DPP
* _sdwa for VOP_SDWA

VOP1/VOP2/VOP3/VOPC examples:

.. code-block:: nasm

  v_mov_b32 v1, v2
  v_mov_b32_e32 v1, v2
  v_nop
  v_cvt_f64_i32_e32 v[1:2], v2
  v_floor_f32_e32 v1, v2
  v_bfrev_b32_e32 v1, v2
  v_add_f32_e32 v1, v2, v3
  v_mul_i32_i24_e64 v1, v2, 3
  v_mul_i32_i24_e32 v1, -3, v3
  v_mul_i32_i24_e32 v1, -100, v3
  v_addc_u32 v1, s[0:1], v2, v3, s[2:3]
  v_max_f16_e32 v1, v2, v3

VOP_DPP examples:

.. code-block:: nasm

  v_mov_b32 v0, v0 quad_perm:[0,2,1,1]
  v_sin_f32 v0, v0 row_shl:1 row_mask:0xa bank_mask:0x1 bound_ctrl:0
  v_mov_b32 v0, v0 wave_shl:1
  v_mov_b32 v0, v0 row_mirror
  v_mov_b32 v0, v0 row_bcast:31
  v_mov_b32 v0, v0 quad_perm:[1,3,0,1] row_mask:0xa bank_mask:0x1 bound_ctrl:0
  v_add_f32 v0, v0, |v0| row_shl:1 row_mask:0xa bank_mask:0x1 bound_ctrl:0
  v_max_f16 v1, v2, v3 row_shl:1 row_mask:0xa bank_mask:0x1 bound_ctrl:0

VOP_SDWA examples:

.. code-block:: nasm

  v_mov_b32 v1, v2 dst_sel:BYTE_0 dst_unused:UNUSED_PRESERVE src0_sel:DWORD
  v_min_u32 v200, v200, v1 dst_sel:WORD_1 dst_unused:UNUSED_PAD src0_sel:BYTE_1 src1_sel:DWORD
  v_sin_f32 v0, v0 dst_unused:UNUSED_PAD src0_sel:WORD_1
  v_fract_f32 v0, |v0| dst_sel:DWORD dst_unused:UNUSED_PAD src0_sel:WORD_1
  v_cmpx_le_u32 vcc, v1, v2 src0_sel:BYTE_2 src1_sel:WORD_0

For full list of supported instructions, refer to "Vector ALU instructions".

HSA Code Object Directives
--------------------------

AMDGPU ABI defines auxiliary data in output code object. In assembly source,
one can specify them with assembler directives.

.hsa_code_object_version major, minor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*major* and *minor* are integers that specify the version of the HSA code
object that will be generated by the assembler.

.hsa_code_object_isa [major, minor, stepping, vendor, arch]
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*major*, *minor*, and *stepping* are all integers that describe the instruction
set architecture (ISA) version of the assembly program.

*vendor* and *arch* are quoted strings.  *vendor* should always be equal to
"AMD" and *arch* should always be equal to "AMDGPU".

By default, the assembler will derive the ISA version, *vendor*, and *arch*
from the value of the -mcpu option that is passed to the assembler.

.amdgpu_hsa_kernel (name)
^^^^^^^^^^^^^^^^^^^^^^^^^

This directives specifies that the symbol with given name is a kernel entry point
(label) and the object should contain corresponding symbol of type STT_AMDGPU_HSA_KERNEL.

.amd_kernel_code_t
^^^^^^^^^^^^^^^^^^

This directive marks the beginning of a list of key / value pairs that are used
to specify the amd_kernel_code_t object that will be emitted by the assembler.
The list must be terminated by the *.end_amd_kernel_code_t* directive.  For
any amd_kernel_code_t values that are unspecified a default value will be
used.  The default value for all keys is 0, with the following exceptions:

- *kernel_code_version_major* defaults to 1.
- *machine_kind* defaults to 1.
- *machine_version_major*, *machine_version_minor*, and
  *machine_version_stepping* are derived from the value of the -mcpu option
  that is passed to the assembler.
- *kernel_code_entry_byte_offset* defaults to 256.
- *wavefront_size* defaults to 6.
- *kernarg_segment_alignment*, *group_segment_alignment*, and
  *private_segment_alignment* default to 4.  Note that alignments are specified
  as a power of two, so a value of **n** means an alignment of 2^ **n**.

The *.amd_kernel_code_t* directive must be placed immediately after the
function label and before any instructions.

For a full list of amd_kernel_code_t keys, refer to AMDGPU ABI document,
comments in lib/Target/AMDGPU/AmdKernelCodeT.h and test/CodeGen/AMDGPU/hsa.s.

Here is an example of a minimal amd_kernel_code_t specification:

.. code-block:: none

   .hsa_code_object_version 1,0
   .hsa_code_object_isa

   .hsatext
   .globl  hello_world
   .p2align 8
   .amdgpu_hsa_kernel hello_world

   hello_world:

      .amd_kernel_code_t
         enable_sgpr_kernarg_segment_ptr = 1
         is_ptr64 = 1
         compute_pgm_rsrc1_vgprs = 0
         compute_pgm_rsrc1_sgprs = 0
         compute_pgm_rsrc2_user_sgpr = 2
         kernarg_segment_byte_size = 8
         wavefront_sgpr_count = 2
         workitem_vgpr_count = 3
     .end_amd_kernel_code_t

     s_load_dwordx2 s[0:1], s[0:1] 0x0
     v_mov_b32 v0, 3.14159
     s_waitcnt lgkmcnt(0)
     v_mov_b32 v1, s0
     v_mov_b32 v2, s1
     flat_store_dword v[1:2], v0
     s_endpgm
   .Lfunc_end0:
        .size   hello_world, .Lfunc_end0-hello_world
