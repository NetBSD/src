// Test that statically allocated thread-specific storage is included in the root set.
// RUN: LSAN_BASE="report_objects=1:use_stacks=0:use_registers=0"
// RUN: %clangxx_lsan %s -o %t
// RUN: LSAN_OPTIONS=$LSAN_BASE:"use_tls=0" %t 2>&1 | FileCheck %s
// RUN: LSAN_OPTIONS=$LSAN_BASE:"use_tls=1" %t 2>&1
// RUN: LSAN_OPTIONS="" %t 2>&1

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// From glibc: this many keys are stored in the thread descriptor directly.
const unsigned PTHREAD_KEY_2NDLEVEL_SIZE = 32;

int main() {
  pthread_key_t key;
  int res;
  res = pthread_key_create(&key, NULL);
  assert(res == 0);
  assert(key < PTHREAD_KEY_2NDLEVEL_SIZE);
  void *p = malloc(1337);
  res = pthread_setspecific(key, p);
  assert(res == 0);
  fprintf(stderr, "Test alloc: %p.\n", p);
  return 0;
}
// CHECK: Test alloc: [[ADDR:.*]].
// CHECK: Directly leaked 1337 byte object at [[ADDR]]
// CHECK: LeakSanitizer: detected memory leaks
// CHECK: SUMMARY: LeakSanitizer:
