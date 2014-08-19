// RUN: %clang_cc1 -verify -fopenmp=libiomp5 -ferror-limit 100 %s

int foo() {
L1:
  foo();
#pragma omp atomic
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  {
    foo();
    goto L1; // expected-error {{use of undeclared label 'L1'}}
  }
  goto L2; // expected-error {{use of undeclared label 'L2'}}
#pragma omp atomic
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  {
    foo();
  L2:
    foo();
  }

  return 0;
}

template <class T>
T read() {
  T a, b = 0;
// Test for atomic read
#pragma omp atomic read
  // expected-error@+1 {{the statement for 'atomic read' must be an expression statement of form 'v = x;', where v and x are both l-value expressions with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'read' clause}}
#pragma omp atomic read read
  a = b;

  return T();
}

int read() {
  int a, b = 0;
// Test for atomic read
#pragma omp atomic read
  // expected-error@+1 {{the statement for 'atomic read' must be an expression statement of form 'v = x;', where v and x are both l-value expressions with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'read' clause}}
#pragma omp atomic read read
  a = b;

  return read<int>();
}

template <class T>
T write() {
  T a, b = 0;
// Test for atomic write
#pragma omp atomic write
  // expected-error@+1 {{the statement for 'atomic write' must be an expression statement of form 'x = expr;', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'write' clause}}
#pragma omp atomic write write
  a = b;

  return T();
}

int write() {
  int a, b = 0;
// Test for atomic write
#pragma omp atomic write
  // expected-error@+1 {{the statement for 'atomic write' must be an expression statement of form 'x = expr;', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'write' clause}}
#pragma omp atomic write write
  a = b;

  return write<int>();
}

template <class T>
T update() {
  T a, b = 0;
// Test for atomic update
#pragma omp atomic update
  // expected-error@+1 {{the statement for 'atomic update' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'update' clause}}
#pragma omp atomic update update
  a += b;

#pragma omp atomic
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;

  return T();
}

int update() {
  int a, b = 0;
// Test for atomic update
#pragma omp atomic update
  // expected-error@+1 {{the statement for 'atomic update' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'update' clause}}
#pragma omp atomic update update
  a += b;

#pragma omp atomic
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;

  return update<int>();
}

template <class T>
T capture() {
  T a, b = 0;
// Test for atomic capture
#pragma omp atomic capture
  // expected-error@+1 {{the statement for 'atomic capture' must be an expression statement of form 'v = ++x;', 'v = --x;', 'v = x++;', 'v = x--;', 'v = x binop= expr;', 'v = x = x binop expr' or 'v = x = expr binop x', where x and v are both l-value expressions with scalar type}}
  ++a;
#pragma omp atomic capture
  // expected-error@+1 {{the statement for 'atomic capture' must be a compound statement of form '{v = x; x binop= expr;}', '{x binop= expr; v = x;}', '{v = x; x = x binop expr;}', '{v = x; x = expr binop x;}', '{x = x binop expr; v = x;}', '{x = expr binop x; v = x;}' or '{v = x; x = expr;}', '{v = x; x++;}', '{v = x; ++x;}', '{++x; v = x;}', '{x++; v = x;}', '{v = x; x--;}', '{v = x; --x;}', '{--x; v = x;}', '{x--; v = x;}' where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'capture' clause}}
#pragma omp atomic capture capture
  a = ++b;

  return T();
}

int capture() {
  int a, b = 0;
// Test for atomic capture
#pragma omp atomic capture
  // expected-error@+1 {{the statement for 'atomic capture' must be an expression statement of form 'v = ++x;', 'v = --x;', 'v = x++;', 'v = x--;', 'v = x binop= expr;', 'v = x = x binop expr' or 'v = x = expr binop x', where x and v are both l-value expressions with scalar type}}
  ++a;
#pragma omp atomic capture
  // expected-error@+1 {{the statement for 'atomic capture' must be a compound statement of form '{v = x; x binop= expr;}', '{x binop= expr; v = x;}', '{v = x; x = x binop expr;}', '{v = x; x = expr binop x;}', '{x = x binop expr; v = x;}', '{x = expr binop x; v = x;}' or '{v = x; x = expr;}', '{v = x; x++;}', '{v = x; ++x;}', '{++x; v = x;}', '{x++; v = x;}', '{v = x; x--;}', '{v = x; --x;}', '{--x; v = x;}', '{x--; v = x;}' where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'capture' clause}}
#pragma omp atomic capture capture
  a = ++b;

  return capture<int>();
}

template <class T>
T seq_cst() {
  T a, b = 0;
// Test for atomic seq_cst
#pragma omp atomic seq_cst
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'seq_cst' clause}}
#pragma omp atomic seq_cst seq_cst
  a += b;

#pragma omp atomic update seq_cst
  // expected-error@+1 {{the statement for 'atomic update' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;

  return T();
}

int seq_cst() {
  int a, b = 0;
// Test for atomic seq_cst
#pragma omp atomic seq_cst
  // expected-error@+1 {{the statement for 'atomic' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;
// expected-error@+1 {{directive '#pragma omp atomic' cannot contain more than one 'seq_cst' clause}}
#pragma omp atomic seq_cst seq_cst
  a += b;

#pragma omp atomic update seq_cst
  // expected-error@+1 {{the statement for 'atomic update' must be an expression statement of form '++x;', '--x;', 'x++;', 'x--;', 'x binop= expr;', 'x = x binop expr' or 'x = expr binop x', where x is an l-value expression with scalar type}}
  ;

 return seq_cst<int>();
}

template <class T>
T mixed() {
  T a, b = T();
// expected-error@+2 2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 2 {{'read' clause used here}}
#pragma omp atomic read write
  a = b;
// expected-error@+2 2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 2 {{'write' clause used here}}
#pragma omp atomic write read
  a = b;
// expected-error@+2 2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 2 {{'update' clause used here}}
#pragma omp atomic update read
  a += b;
// expected-error@+2 2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 2 {{'capture' clause used here}}
#pragma omp atomic capture read
  a = ++b;
  return T();
}

int mixed() {
  int a, b = 0;
// expected-error@+2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 {{'read' clause used here}}
#pragma omp atomic read write
  a = b;
// expected-error@+2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 {{'write' clause used here}}
#pragma omp atomic write read
  a = b;
// expected-error@+2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 {{'write' clause used here}}
#pragma omp atomic write update
  a = b;
// expected-error@+2 {{directive '#pragma omp atomic' cannot contain more than one 'read', 'write', 'update' or 'capture' clause}}
// expected-note@+1 {{'write' clause used here}}
#pragma omp atomic write capture
  a = b;
  // expected-note@+1 {{in instantiation of function template specialization 'mixed<int>' requested here}}
  return mixed<int>();
}

