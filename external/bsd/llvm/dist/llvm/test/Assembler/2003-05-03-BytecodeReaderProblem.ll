; RUN: llvm-as < %s | llvm-dis
; RUN: verify-uselistorder %s -preserve-bc-use-list-order

define void @test() {
        %tmp.123 = trunc i64 0 to i32           ; <i32> [#uses=0]
        ret void
}
