// RUN: %clang -target x86_64-apple-darwin10 --analyze %s -o /dev/null -Xclang -analyzer-checker=debug.ConfigDumper -Xclang -analyzer-max-loop -Xclang 34 > %t 2>&1
// RUN: FileCheck --input-file=%t %s

void bar() {}
void foo() {
  // Call bar 33 times so max-times-inline-large is met and
  // min-blocks-for-inline-large is checked
  for (int i = 0; i < 34; ++i) {
    bar();
  }
}

// CHECK: [config]
// CHECK-NEXT: cfg-conditional-static-initializers = true
// CHECK-NEXT: cfg-temporary-dtors = false
// CHECK-NEXT: faux-bodies = true
// CHECK-NEXT: graph-trim-interval = 1000
// CHECK-NEXT: inline-lambdas = true
// CHECK-NEXT: ipa = dynamic-bifurcate
// CHECK-NEXT: ipa-always-inline-size = 3
// CHECK-NEXT: leak-diagnostics-reference-allocation = false
// CHECK-NEXT: max-inlinable-size = 50
// CHECK-NEXT: max-nodes = 150000
// CHECK-NEXT: max-times-inline-large = 32
// CHECK-NEXT: min-cfg-size-treat-functions-as-large = 14
// CHECK-NEXT: mode = deep
// CHECK-NEXT: region-store-small-struct-limit = 2
// CHECK-NEXT: widen-loops = false
// CHECK-NEXT: [stats]
// CHECK-NEXT: num-entries = 15

