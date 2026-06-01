all:
	make chatclient
	make chatserver
	
chatclient: chatclient.o tools/socket.o tools/selector.o
	clang++ -o chatclient chatclient.o tools/socket.o tools/selector.o
	
chatclient.o : chatclient.cc 
	clang++ -c -I tools chatclient.cc
	
chatserver: chatserver.o tools/socket.o tools/selector.o
	clang++ -o chatserver chatserver.o tools/socket.o tools/selector.o
	
chatserver.o : chatserver.cc 
	clang++ -c -I tools chatserver.cc
	
clean:
	rm -f chatclient.o chatserver.o chatclient chatserver
	
