;; builtin definitions for DEC VAX.
;; Copyright (C) 2007 Free Software Foundation, Inc.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING.  If not, write to
;; the Free Software Foundation, 51 Franklin Street, Fifth Floor,
;; Boston, MA 02110-1301, USA.

(define_expand "ffssi2"
  [(set (match_operand:SI 0 "nonimmediate_operand" "")
        (ffs:SI (match_operand:SI 1 "general_operand" "")))]
  ""
  "
{
  rtx label = gen_label_rtx ();
  emit_insn (gen_unspec_ffssi2 (operands[0], operands[1]));
  emit_jump_insn (gen_bne (label));
  emit_insn (gen_negsi2 (operands[0], const1_rtx));
  emit_label (label);
  emit_insn (gen_addsi3 (operands[0], operands[0], const1_rtx));
  DONE;
}")

;;
;; Set cc0 to match argument 1 since if it is 0, Z will be set just as
;; if a tst:SI was performed.  If we did a match_dup 0, that wouldn't be
;; right since 0 will be set to (0+32) [the position (relative to the base)
;; of a bit one position to the left of the specified field].
;;
(define_insn "unspec_ffssi2"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=g")
        (unspec:SI [(match_operand:SI 1 "general_operand" "nrQ")] VUNSPEC_FFS))
   (set (cc0) (match_dup 1))]
  ""
  "ffs $0,$32,%1,%0")
