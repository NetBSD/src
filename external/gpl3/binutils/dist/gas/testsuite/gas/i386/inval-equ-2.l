.*: Assembler messages:
.*:8: Error: .*
.*:8: Error: .*
.*:8: Error: .*
GAS LISTING .*


[ 	]*1[ 	]+\.globl  bar1
[ 	]*2[ 	]+\.set    bar1,\(%eax\+1\)
[ 	]*3[ 	]+\?\?\?\? A12A0000 		mov bar1,%eax
[ 	]*3[ 	]+00
[ 	]*4[ 	]+\.set    bar2,\(%eax\+1\)
[ 	]*5[ 	]+\?\?\?\? A12A0000 		mov bar2,%eax
[ 	]*5[ 	]+00
[ 	]*6[ 	]+\.globl  bar2
[ 	]*7[ 	]+\.set    bar3,\(%eax\+1\)
[ 	]*8[ 	]+\?\?\?\? A12A0000 		mov bar3,%eax
\*\*\*\*  Error:can't make global register symbol `bar3'
[ 	]*8[ 	]+00
