;; GCC machine description for Tilera TILE-Gx synchronization
;; instructions.
;; Copyright (C) 2011-2018 Free Software Foundation, Inc.
;; Contributed by Walter Lee (walt@tilera.com)
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.
;;
;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

(define_code_iterator fetchop [plus ior and])
(define_code_attr fetchop_name [(plus "add") (ior "or") (and "and")])

(define_insn "mtspr_cmpexch<mode>"
  [(set (reg:I48MODE TILEGX_CMPEXCH_REG)
        (unspec_volatile:I48MODE
         [(match_operand:I48MODE 0 "reg_or_0_operand" "rO")]
         UNSPEC_SPR_MOVE))]
  ""
  "mtspr\tCMPEXCH_VALUE, %r0"
  [(set_attr "type" "X1")])


(define_expand "atomic_compare_and_swap<mode>"
  [(match_operand:DI 0 "register_operand" "")		;; bool output
   (match_operand:I48MODE 1 "register_operand" "")	;; val output
   (match_operand:I48MODE 2 "nonautoincmem_operand" "") ;; memory
   (match_operand:I48MODE 3 "reg_or_0_operand" "")	;; expected value
   (match_operand:I48MODE 4 "reg_or_0_operand" "")	;; desired value
   (match_operand:SI 5 "const_int_operand" "")		;; is_weak
   (match_operand:SI 6 "const_int_operand" "")		;; mod_s
   (match_operand:SI 7 "const_int_operand" "")]		;; mod_f
  ""
{
  enum memmodel mod_s = (enum memmodel) INTVAL (operands[6]);

  if (operands[3] != const0_rtx)
    operands[3] = force_reg (<MODE>mode, operands[3]);
  if (operands[4] != const0_rtx)
    operands[4] = force_reg (<MODE>mode, operands[4]);

  tilegx_pre_atomic_barrier (mod_s);
  emit_insn (gen_mtspr_cmpexch<mode> (operands[3]));
  emit_insn (gen_atomic_compare_and_swap_bare<mode> (operands[1], operands[2],
                                                   operands[4]));
  tilegx_post_atomic_barrier (mod_s);
  emit_insn (gen_insn_cmpeq_<mode>di (operands[0], operands[1], operands[3]));
  DONE;
})


(define_insn "atomic_compare_and_swap_bare<mode>"
  [(set (match_operand:I48MODE 0 "register_operand" "=r")
        (match_operand:I48MODE 1 "nonautoincmem_operand" "+U"))
   (set (match_dup 1)
        (unspec_volatile:I48MODE
         [(match_dup 1)
         (reg:I48MODE TILEGX_CMPEXCH_REG)
         (match_operand:I48MODE 2 "reg_or_0_operand" "rO")]
         UNSPEC_CMPXCHG))]
  ""
  "cmpexch<four_if_si>\t%0, %1, %r2"
  [(set_attr "type" "X1_remote")])


(define_expand "atomic_exchange<mode>"
  [(match_operand:I48MODE 0 "register_operand" "")      ;; result
   (match_operand:I48MODE 1 "nonautoincmem_operand" "") ;; memory
   (match_operand:I48MODE 2 "reg_or_0_operand" "")      ;; input
   (match_operand:SI 3 "const_int_operand" "")]         ;; model
  ""
{
  enum memmodel model = (enum memmodel) INTVAL (operands[3]);

  tilegx_pre_atomic_barrier (model);
  emit_insn (gen_atomic_exchange_bare<mode> (operands[0], operands[1],
                                             operands[2]));
  tilegx_post_atomic_barrier (model);
  DONE;
})


(define_insn "atomic_exchange_bare<mode>"
  [(set (match_operand:I48MODE 0 "register_operand" "=r")
	(match_operand:I48MODE 1 "nonautoincmem_operand" "+U"))
   (set (match_dup 1)
	(unspec_volatile:I48MODE
	 [(match_operand:I48MODE 2 "reg_or_0_operand" "rO")]
	 UNSPEC_XCHG))]
  ""
  "exch<four_if_si>\t%0, %1, %r2"
  [(set_attr "type" "X1_remote")])


(define_expand "atomic_fetch_<fetchop_name><mode>"
  [(match_operand:I48MODE 0 "register_operand" "")      ;; result
   (match_operand:I48MODE 1 "nonautoincmem_operand" "") ;; memory
   (unspec_volatile:I48MODE
    [(fetchop:I48MODE
      (match_dup 1)
      (match_operand:I48MODE 2 "reg_or_0_operand" ""))] ;; value
    UNSPEC_ATOMIC)
   (match_operand:SI 3 "const_int_operand" "")]         ;; model
  ""
{
  enum memmodel model = (enum memmodel) INTVAL (operands[3]);

  tilegx_pre_atomic_barrier (model);
  emit_insn (gen_atomic_fetch_<fetchop_name>_bare<mode> (operands[0],
                                                         operands[1],
                                                         operands[2]));
  tilegx_post_atomic_barrier (model);
  DONE;
})


(define_insn "atomic_fetch_<fetchop_name>_bare<mode>"
  [(set (match_operand:I48MODE 0 "register_operand" "=r")
	(match_operand:I48MODE 1 "nonautoincmem_operand" "+U"))
   (set (match_dup 1)
	(unspec_volatile:I48MODE
	 [(fetchop:I48MODE
	   (match_dup 1)
	   (match_operand:I48MODE 2 "reg_or_0_operand" "rO"))]
	   UNSPEC_ATOMIC))]
  ""
  "fetch<fetchop_name><four_if_si>\t%0, %1, %r2"
  [(set_attr "type" "X1_remote")])


(define_expand "atomic_fetch_sub<mode>"
  [(match_operand:I48MODE 0 "register_operand" "")      ;; result
   (match_operand:I48MODE 1 "nonautoincmem_operand" "") ;; memory
   (unspec_volatile:I48MODE
    [(minus:I48MODE
      (match_dup 1)
      (match_operand:I48MODE 2 "reg_or_0_operand" ""))] ;; value
    UNSPEC_ATOMIC)
   (match_operand:SI 3 "const_int_operand" "")]         ;; model
  ""
{
  rtx addend;
  enum memmodel model = (enum memmodel) INTVAL (operands[3]);

  if (operands[2] != const0_rtx)
    {
       addend = gen_reg_rtx (<MODE>mode);
       emit_move_insn (addend,
                       gen_rtx_MINUS (<MODE>mode, const0_rtx, operands[2]));
    }
  else
    addend = operands[2];

  tilegx_pre_atomic_barrier (model);
  emit_insn (gen_atomic_fetch_add_bare<mode> (operands[0],
                                              operands[1],
                                              addend));
  tilegx_post_atomic_barrier (model);
  DONE;
})


(define_expand "atomic_test_and_set"
  [(match_operand:QI 0 "register_operand" "")           ;; bool output
   (match_operand:QI 1 "nonautoincmem_operand" "+U")    ;; memory
   (match_operand:SI 2 "const_int_operand" "")]         ;; model
  ""
{
  rtx addr, aligned_addr, aligned_mem, offset, word, shmt, tmp;
  rtx result = operands[0];
  rtx mem = operands[1];
  enum memmodel model = (enum memmodel) INTVAL (operands[2]);

  addr = force_reg (Pmode, XEXP (mem, 0));

  aligned_addr = gen_reg_rtx (Pmode);
  emit_move_insn (aligned_addr, gen_rtx_AND (Pmode, addr, GEN_INT (-8)));

  aligned_mem = change_address (mem, DImode, aligned_addr);
  set_mem_alias_set (aligned_mem, 0);

  tmp = gen_reg_rtx (Pmode);
  if (BYTES_BIG_ENDIAN)
    {
      emit_move_insn (gen_lowpart (DImode, tmp),
                      gen_rtx_NOT (DImode, gen_lowpart (DImode, addr)));
    }
  else
    {
      tmp = addr;
    }

  offset = gen_reg_rtx (DImode);
  emit_move_insn (offset, gen_rtx_AND (DImode, gen_lowpart (DImode, tmp),
                                       GEN_INT (7)));

  tmp = gen_reg_rtx (DImode);
  emit_move_insn (tmp, GEN_INT (1));

  shmt = gen_reg_rtx (DImode);
  emit_move_insn (shmt, gen_rtx_ASHIFT (DImode, offset, GEN_INT (3)));

  word = gen_reg_rtx (DImode);
  emit_move_insn (word, gen_rtx_ASHIFT (DImode, tmp,
                                        gen_lowpart (SImode, shmt)));

  tmp = gen_reg_rtx (DImode);
  tilegx_pre_atomic_barrier (model);
  emit_insn (gen_atomic_fetch_or_baredi (tmp, aligned_mem, word));
  tilegx_post_atomic_barrier (model);

  emit_move_insn (gen_lowpart (DImode, result),
                  gen_rtx_LSHIFTRT (DImode, tmp,
                                    gen_lowpart (SImode, shmt)));
  DONE;
})
