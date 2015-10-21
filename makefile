syscall: syscall.c
	gcc syscall.c -o syscall.x -g

clean: 
	rm -v *.o *.obj *.exe *.x
