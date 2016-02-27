// Make sure that __global__ functions are emitted along with correct
// annotations and are added to @llvm.used to prevent their elimination.
// REQUIRES: nvptx-registered-target
//
// RUN: %clang_cc1 %s -triple nvptx-unknown-unknown -fcuda-is-device -emit-llvm -o - | FileCheck %s

#include "Inputs/cuda.h"

// CHECK-LABEL: define void @device_function
extern "C"
__device__ void device_function() {}

// CHECK-LABEL: define void @global_function
extern "C"
__global__ void global_function() {
  // CHECK: call void @device_function
  device_function();
}

// Make sure host-instantiated kernels are preserved on device side.
template <typename T> __global__ void templated_kernel(T param) {}
// CHECK-LABEL: define weak_odr void @_Z16templated_kernelIiEvT_
void host_function() { templated_kernel<<<0,0>>>(0); }

// CHECK: !{{[0-9]+}} = !{void ()* @global_function, !"kernel", i32 1}
// CHECK: !{{[0-9]+}} = !{void (i32)* @_Z16templated_kernelIiEvT_, !"kernel", i32 1}
