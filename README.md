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


## Web Server Description
This C program is a simple web server that listens for incoming HTTP connections and serves requested content. The server can handle multiple clients simultaneously up to a defined maximum limit (MAXCLIENTS). The server supports running CGI programs and can handle simple HTTP GET requests.

### Main Function
The main function starts by validating the command-line arguments and setting up a server socket for incoming client connections. It then initializes an array of clientstate structures to maintain information about each connected client. A select-based approach is used to handle multiple clients without the need for multi-threading or forking.

The server uses a while loop to continuously monitor the server socket and connected client sockets for activity. When a new connection arrives, the server accepts it and updates the clientstate structure for the new client. If the number of clients exceeds the defined maximum, the server terminates.

The server also handles incoming HTTP requests from connected clients. It reads incoming data from the client sockets and processes the requests using the handleClient function. Once a complete GET request is received and validated, the server processes the request using the processRequest function, which can run CGI programs if needed.

Additionally, the server reads output data from the CGI programs and sends the data back to the client. It closes the client's socket and associated file descriptors once the response is complete or if an error occurs. The server also monitors a timer and will close if no new connections are received within the specified time limit (5 minutes in this implementation).

### handleClient Function
The handleClient function is responsible for handling incoming HTTP GET requests. It reads data from the client socket and updates the clientstate structure with the received request. If the request is incomplete, the function returns 0, indicating that more data is expected. If the request is invalid, the function returns -1, indicating that the socket should be closed. If the request is complete and valid, the function returns 1, indicating that the request is ready for processing. In this case, the function also updates the clientstate structure with the executable path, query string, and allocated output buffer.

## Starting the Server
To start the server, run the following command in your terminal:

bash

./wserver <port_number>

Replace <port_number> with the desired port number you want the server to listen on. This will start the server and allow it to accept incoming connections on the specified port.
