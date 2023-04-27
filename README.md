# CGI Web Server

## Web Server Makefile
This is a simple Makefile for building a web server (wserver) and related utilities. The Makefile uses gcc as the compiler with the -g and -Wall flags for debugging and showing warnings, respectively.

### Targets
The Makefile contains the following targets:

- all: Builds all targets (wserver, simple, term, slowcgi, testprogtable, and large).
- wserver: Builds the web server executable.
- slowcgi: Builds the slowcgi utility.
- testprogtable: Builds the testprogtable utility.
- simple: Builds the simple CGI utility.
- large: Builds the large CGI utility.
- term: Builds the term utility.
- clean: Removes all object files and executables.

### Dependencies
The Makefile handles dependencies between files as follows:

- cgi.o depends on cgi.h
- large.o depends on cgi.h
- process_request.o depends on ws_helpers.h and wrapsock.h
- simple.o depends on cgi.h
- wrapsock.o depends on wrapsock.h
- ws_helpers.o depends on wrapsock.h and ws_helpers.h
- wserver.o depends on wrapsock.h and ws_helpers.h


Start the server on a port by running 
`./wserver <port number>`
