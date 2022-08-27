1.
#define A(a) a
A();

2.
#define B(a, b) (a,b)
B(a, );
B(, b);
B( , );
B(a,);
B(,b);
B(,);

3.
#define C(a, b, c) (a,b,c)
C(a, b, );
C(a, , c);
C(, , c);
C(a, , );
C(, b, );
C(, , c);
C(, , )
C(a,b,);
C(a,,c);
C(,,c);
C(a,,);
C(,b,);
C(,,c);
C(,,)
