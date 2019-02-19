.PHONY: clean

# defalut
SocketThread: 
	gcc -std=c99 -pthread clientFile.c -o cf.out
	gcc -std=c99 -pthread serverFile.c -o sf.out

clean:
	$(RM) *.out