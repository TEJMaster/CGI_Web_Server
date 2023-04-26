#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>    /* Internet domain header */

#include "wrapsock.h"
#include "ws_helpers.h"

#define MAXCLIENTS 10

int handleClient(struct clientstate *cs, char *line);

// You may want to use this function for initial testing
//void write_page(int fd);

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: wserver <port>\n");
        exit(1);
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    int listenfd;
    struct clientstate client[MAXCLIENTS];


    // Set up the socket to which the clients will connect
    listenfd = setupServerSocket(port);

    initClients(client, MAXCLIENTS);



    // TODO: complete this function
    struct timeval timer;
    timer.tv_sec = 300; // set the timer to 5 minutes
    timer.tv_usec = 0; 
    int max_fd = listenfd;  
    fd_set all_fds;  
    FD_ZERO(&all_fds);
    FD_SET(listenfd, &all_fds);
    int num_clients = 0;
    while (num_clients <= MAXCLIENTS) {  
        
        fd_set listen_fds = all_fds; 

        // check the timer 
        if (Select(max_fd + 1, &listen_fds, NULL, NULL, &timer) > 0) {
        // check for new connections
            if (FD_ISSET(listenfd, &listen_fds)) {
                printf("Accepted connection\n");
                int user_index = 0;  
                // find the first new user index for the new client
                while (user_index < MAXCLIENTS && client[user_index].sock != -1) {
                    user_index ++;
                } 
                // if there are more than MAXCLIENTS number of clients connect to the server then terminate
                // This should never happen but if it happens we want to terminate the program
                if (user_index >= MAXCLIENTS) {  
                    fprintf(stderr, "server: max concurrent connections\n");  
                    return 1;
                } 
                else {
                        // accept connection for the user
                        int client_fd = Accept(listenfd, NULL, NULL);
                        client[user_index].sock = client_fd;
                        if (client_fd > max_fd) {
                            max_fd = client_fd;
                        }
                        FD_SET(client_fd, &all_fds);
                        num_clients ++;
                }     
            }  

            for (int index = 0; index < MAXCLIENTS; index++) {
                // every while loop check all posible clients in the server
                if (client[index].sock > -1 && FD_ISSET(client[index].sock, &listen_fds)) {  
                    int fd = client[index].sock;   
                    char buf[MAXLINE + 1];  
                    int num_read = read(fd, &buf, MAXLINE);  
                    buf[num_read] = '\0';  
                    int handle = handleClient(&client[index], buf);
                    if (num_read > 0){
                        if (handle == 1) {
                            int requestType = processRequest(&client[index]);
                            if (requestType != -1) {
                                // cases when processRequest returns cs->fd[0]
                                client[index].fd[0]= requestType;
                                FD_SET(client[index].fd[0], &all_fds);
                                if(client[index].fd[0] > max_fd) {
                                    max_fd = client[index].fd[0];
                                }
                            } else {
                                // cases for processRequest returns -1
                                fprintf(stderr, "close at process request is -1\n");
                                Close(client[index].sock);
                                FD_CLR(client[index].sock, &all_fds);
                                FD_CLR(client[index].fd[0], &all_fds);
                                resetClient(&client[index]);
                                printf("Client %d disconnected\n", fd);  
                            }
                        } else if (handle == -1) {
                            // when handle client returns -1
                            fprintf(stderr, "close at handle is -1\n");
                            Close(client[index].sock);
                            FD_CLR(client[index].sock, &all_fds);
                            resetClient(&client[index]);
                        }   
                    } 
                }
                if (client[index].fd[0] > -1 && FD_ISSET(client[index].fd[0], &listen_fds)) {  
                    
                    int pipe_read = read(client[index].fd[0], client[index].optr, MAXPAGE + client[index].output - client[index].optr);
                    // fprintf(stderr, "%d pipe read is %d\n", index, pipe_read);
                    if (pipe_read <= 0) {
                        // when it finnshes reading from the server to the client
                        fprintf(stderr, "program finnishes reading from pipe\n");
                        int status;
                        if (wait(&status) == -1) {
                            // error checking for wait call
                            perror("wait");
                        } else {
                            // wait until the child process from server to exit
                            if (WIFEXITED(status)) {
                                if (WEXITSTATUS(status) == 0) {
                                    // print 200 if exit status for child process is 0
                                    printOK(client[index].sock, client[index].output, client[index].optr-client[index].output);
                                }
                                else if (WEXITSTATUS(status) == 100) {
                                    // print not found if exit status for child process is 100
                                    printNotFound(client[index].sock);
                                } else {
                                    printServerError(client[index].sock);
                                }
                            } else {
                                // print server error when child process terminated unexpectedly
                                printServerError(client[index].sock);
                            } 
                            // after we printed the message we close the socket for this client
                            Close(client[index].fd[0]);
                            Close(client[index].sock);
                            FD_CLR(client[index].fd[0], &all_fds);
                            FD_CLR(client[index].sock, &all_fds);
                            resetClient(&client[index]);
                        }
                    } else {
                        // if pipe read is more than 1 there is still more things to read
                        client[index].optr += pipe_read;
                    }
                }
            }
        } else {
            // close the server when there is more than 5 minetes of no new select
            fprintf(stderr, "server closed due to timer\n");
            exit(0);
        }

    }


    return 0;
}

/* Update the client state cs with the request input in line.
 * Intializes cs->request if this is the first read call from the socket.
 * Note that line must be null-terminated string.
 *
 * Return 0 if the get request message is not complete and we need to wait for
 *     more data
 * Return -1 if there is an error and the socket should be closed
 *     - Request is not a GET request
 *     - The first line of the GET request is poorly formatted (getPath, getQuery)
 * 
 * Return 1 if the get request message is complete and ready for processing
 *     cs->request will hold the complete request
 *     cs->path will hold the executable path for the CGI program
 *     cs->query will hold the query string
 *     cs->output will be allocated to hold the output of the CGI program
 *     cs->optr will point to the beginning of cs->output
 */
int handleClient(struct clientstate *cs, char *line) {

    // TODO: Complete this function


    // update the request: if it is null we malloc a space for it
    // if it is not null we cat the line at the end of it.
    if (cs->request != NULL) {
        strcat(cs->request, line);
        
    } else {
	cs->request = malloc(MAXLINE*sizeof(char));
	strcpy(cs->request, line);
        
    }
    // if we didn't found \r\n\r\n it means there is still more line to read so we return 0
    if (strstr(cs->request, "\r\n\r\n") == NULL) {
        return 0;
    }

    // return -1 if the request is not a get request
    if (sizeof(cs->request) < 4 || cs->request[0] != 'G' || cs->request[1] != 'E'|| cs->request[2] != 'T' || cs->request[3] != ' ') {
        fprintf(stderr,"get error\n");
        return -1;
    }
    // return -1 if getPath returns null it means there is no path
    if ((cs -> path = getPath(cs->request)) == NULL) {
        fprintf(stderr,"path error\n");
        return -1;
    }

    // If the resource is favicon.ico we will ignore the request
    if(strcmp("favicon.ico", cs->path) == 0){
        // A suggestion for debugging output
        fprintf(stderr, "Client: sock = %d\n", cs->sock);
        fprintf(stderr, "        path = %s (ignoring)\n", cs->path);
		printNotFound(cs->sock);
        return -1;
    }

    // if the program reaches here it means the request is valid

    // update the query string using the getQuery function
    // this helper funciton already handled the case for empty query
    cs->query_string = getQuery(cs->request); 
    // malloc a space for output and an pointer to it
    cs->output = malloc(MAXPAGE * sizeof(char));
    cs->optr = cs->output;

    // A suggestion for printing some information about each client. 
    // You are welcome to modify or remove these print statements
    fprintf(stderr, "Client: sock = %d\n", cs->sock);
    fprintf(stderr, "        path = %s\n", cs->path);
    fprintf(stderr, "        query_string = %s\n", cs->query_string);

    return 1;
}

