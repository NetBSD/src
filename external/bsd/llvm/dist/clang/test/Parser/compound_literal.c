// RUN: %clang_cc1 -fsyntax-only -verify %s
// expected-no-diagnostics
int main() {
  char *s;
  s = (char []){"whatever"}; 
}
