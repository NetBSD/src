CFLAGS = -g
AFLAGS = -g -I../../compile/PMAX

fptest: fptest.o fptest1.o fp.o
	$(CC) $(CFLAGS) -o fptest fptest.o fptest1.o fp.o

fp.o: ../pmax/fp.s
	$(CC) $(AFLAGS) -c ../pmax/fp.s

fp1: fp1.o
	$(CC) $(CFLAGS) -o fp1 fp1.o

fp2: fp2.o
	$(CC) $(CFLAGS) -o fp2 fp2.o -lm

fp3: fp3.o
	$(CC) $(CFLAGS) -o fp3 fp3.o -lm

fp4: fp4.o
	$(CC) $(CFLAGS) -o fp4 fp4.o -lm

fp5: fp5.o
	$(CC) $(CFLAGS) -o fp5 fp5.o -lm

fp6: fp6.o
	$(CC) $(CFLAGS) -o fp6 fp6.o -lm

fp7: fp7.o
	$(CC) $(CFLAGS) -o fp7 fp7.o

fp8: fp8.o
	$(CC) $(CFLAGS) -o fp8 fp8.o
