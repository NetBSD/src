// RUN: %clang_cc1 -triple wasm32-unknown-unknown -O3 -emit-llvm -o - %s \
// RUN:   | FileCheck %s -check-prefix=WEBASSEMBLY32
// RUN: %clang_cc1 -triple wasm64-unknown-unknown -O3 -emit-llvm -o - %s \
// RUN:   | FileCheck %s -check-prefix=WEBASSEMBLY64

__SIZE_TYPE__ f1(void) {
  return __builtin_wasm_memory_size();
// WEBASSEMBLY32: call {{i.*}} @llvm.wasm.memory.size.i32()
// WEBASSEMBLY64: call {{i.*}} @llvm.wasm.memory.size.i64()
}

void f2(long delta) {
  __builtin_wasm_grow_memory(delta);
// WEBASSEMBLY32: call void @llvm.wasm.grow.memory.i32(i32 %{{.*}})
// WEBASSEMBLY64: call void @llvm.wasm.grow.memory.i64(i64 %{{.*}})
}
