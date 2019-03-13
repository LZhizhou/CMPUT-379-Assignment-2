.PHONY: clean

# defalut
SocketThread: 
	gcc -std=c99 -pthread clientFile.c -o cf.out
	gcc -std=c99 -pthread `xml2-config --cflags` serverFile.c -o sf.out `xml2-config --libs` -lssl -lcrypto

clean:
	$(RM) *.out