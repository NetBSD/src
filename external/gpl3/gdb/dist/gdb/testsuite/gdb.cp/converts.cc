class A {};
class B : public A {};

typedef A TA1;
typedef A TA2;
typedef TA2 TA3;

int foo0_1 (TA1)  { return 1; }
int foo0_2 (TA3)  { return 2; }
int foo0_3 (A***) { return 3; }

int foo1_1 (char *) {return 11;}
int foo1_2 (char[]) {return 12;}
int foo1_3 (int*)   {return 13;}
int foo1_4 (A*)     {return 14;}
int foo1_5 (void*)  {return 15;}
int foo1_6 (void**) {return 16;}
int foo1_7 (bool)   {return 17;}
int foo1_8 (long)   {return 18;}

int foo2_1 (char**  )  {return 21;}
int foo2_2 (char[][1]) {return 22;}
int foo2_3 (char *[])  {return 23;}
int foo2_4 (int  *[])  {return 24;}

int main()
{

  TA2 ta;      // typedef to..
  foo0_1 (ta); // ..another typedef
  foo0_2 (ta); // ..typedef of a typedef

  B*** bppp;    // Pointer-to-pointer-to-pointer-to-derived..
//foo0_3(bppp); // Pointer-to-pointer-to-pointer base.
  foo0_3((A***)bppp); // to ensure that the function is emitted.

  char *a;             // pointer to..
  B *bp;
  foo1_1 (a);          // ..pointer
  foo1_2 (a);          // ..array
  foo1_3 ((int*)a);    // ..pointer of wrong type
  foo1_3 ((int*)bp);   // ..pointer of wrong type
  foo1_4 (bp);         // ..ancestor pointer
  foo1_5 (bp);         // ..void pointer
  foo1_6 ((void**)bp); // ..void pointer pointer
  foo1_7 (bp);         // ..boolean
  foo1_8 ((long)bp);   // ..long int

  char **b;          // pointer pointer to..
  char ba[1][1];
  foo1_5 (b);        // ..void pointer
  foo2_1 (b);        // ..pointer pointer
  foo2_2 (ba);       // ..array of arrays
  foo2_3 (b);        // ..array of pointers
  foo2_4 ((int**)b); // ..array of wrong pointers
  return 0;          // end of main
}
