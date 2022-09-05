all: p1 enc1 channel enc2 p2

p1 : p1.c
	gcc -o p1 p1.c -lrt -lssl -lcrypto
enc1 : enc1.c 
	gcc -o enc1 enc1.c -lrt -lssl -lcrypto

channel : channel.c 
	gcc -o channel channel.c -lrt -lssl -lcrypto	

enc2 : enc2.c 
	gcc -o enc2 enc2.c -lrt -lssl -lcrypto	

p2 : p2.c 
	gcc -o p2 p2.c -lrt -lssl -lcrypto

clean:
	rm -f *.o p1 enc1 channel enc2 p2

.PHONY: all
