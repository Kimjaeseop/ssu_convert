ssu_convert : ssu_convert.o
	gcc -o ssu_convert ssu_convert.o

ssu_convert.o : ssu_convert.c
	gcc -c ssu_convert.c

clean : 
	rm *.o ssu_convert
