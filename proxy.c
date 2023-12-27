/*
 * This is the main program for the proxy, which receives connections for sending and receiving clients
 * both in binary and XML format. Many clients can be connected at the same time. The proxy implements
 * an event loop.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "connection.h"
#include "record.h"
#include "recordFromFormat.h"
#include "recordToFormat.h"
#include "xmlfile.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BUFSIZE 51200
#define MAXSERVER 32

/* This struct should contain the information that you want
 * keep for one connected client.
 */
struct Client {
    int index;   // the index this client is in
    int sock;    // FD to this client
    char id;     // client's ID
    char type;   // clients type, 'X'(xml) or 'B'(binary)
    int bufsize; // size of the stored data in buffer
    char *buf;   // a buffer of bytes received through the connection
};

typedef struct Client Client;

// Global variables
int server_sock, connected_sockets = 0;
fd_set fdset, ready_fdset;

void usage(char *cmd) {
    fprintf(stderr, "Usage: %s <port>\n"
                    "       This is the proxy server. It takes as imput the port where it accepts connections\n"
                    "       from \"xmlSender\", \"binSender\" and \"anyReceiver\" applications.\n"
                    "       <port> - a 16-bit integer in host byte order identifying the proxy server's port\n"
                    "\n",
            cmd);
    exit(-1);
}

/*
 * This function is called when a new connection is noticed on the server
 * socket.
 * The proxy accepts a new connection and creates the relevant data structures.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
Client *handleNewClient(int client_sock) {
    Client *c = malloc(sizeof *c);
    c->sock = client_sock;
    c->id = 0;
    c->bufsize = 0;
    c->type = 0;
    c->id = 0;
    c->buf = malloc(sizeof(c->buf) * BUFSIZE);

    return c;
}

/*
 * This function is called when a connection is broken by one of the connecting
 * clients. Data structures are clean up and resources that are no longer needed
 * are released.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */
void removeClient(Client *c, Client **client_arr) {
    client_arr[c->index] = NULL;
    free(c->buf);
    free(c);
}

/*
 * This function is called when the proxy received enough data from a sending
 * client to create a Record. The 'dest' field of the Record determines the
 * client to which the proxy should send this Record.
 *
 * If no such client is connected to the proxy, the Record is discarded without
 * error. Resources are released as appropriate.
 *
 * If such a client is connected, this functions find the correct socket for
 * sending to that client, and determines if the Record must be converted to
 * XML format or to binary format for sendig to that client.
 *
 * It does then send the converted messages.
 * Finally, this function deletes the Record before returning.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */

void forwardMessage(Record *r, Client **client_arr) {
    for (int i = 0; i < MAXSERVER; i++) {
        Client *c = client_arr[i];
        if (c == NULL)
            continue;
        //        if (!FD_ISSET(c->sock, &fdset))
        //            continue;
        if (r->dest != c->id)
            continue;

        char *buffer;
        int buflen = 0;

        if (c->type == 'B')
            buffer = recordToBinary(r, &buflen);
        else
            buffer = recordToXML(r, &buflen);

        printf("%s\n", buffer);

        if (buffer != NULL) {
            write(c->sock, buffer, buflen);
            free(buffer);
        }
    }
}

/*
 * This function is called whenever activity is noticed on an connected socket,
 * and that socket is associated with a client. This can be sending client
 * or a receiving client.
 *
 * The calling function finds the Client structure for the socket where
 * acticity has occurred and calls this function.
 *
 * If this function receives data that completes a record, it creates an internal
 * Record data structure on the heap and calls forwardMessage() with this Record.
 *
 * If this function notices that a client has disconnected, it calls removeClient()
 * to release the resources associated with it.
 *
 * *** The parameters and return values of this functions can be changed. ***
 */

Record *createDummyRecord() {
    Record *r = malloc(sizeof(Record));
    r->has_source = 1, r->has_dest = 1;
    r->has_username = 1, r->has_id = 1;
    r->has_group = 1, r->has_semester = 1;
    r->has_grade = 1, r->has_courses = 1;
    r->source = 'A', r->dest = 'X';
    r->username = strdup("griff"), r->id = 1003;
    r->group = 200, r->semester = 27;
    r->grade = Grade_PhD, r->courses = Course_IN1020 | Course_IN1060;
    return r;
}

Client *find_client_ref(int sock, Client **client_arr) {
    printf("looking for sock: %d\n", sock);
    for (int i = 0; i < MAXSERVER; i++) {
        if (client_arr[i] != NULL && client_arr[i]->sock == sock)
            return client_arr[i];
    }
    return NULL;
}

bool connected(int sock) {
    char buf;
    size_t err = recv(sock, &buf, 1, MSG_PEEK);
    if (err == 0)
        return false;

    return true;
}

int handleClient(Client *c, Client **client_arr) {

    // check if client is still connected
    if (!connected(c->sock)) {
        printf("client %d disconnected\n", c->sock);
        removeClient(c, client_arr);
        connected_sockets--;
        return 0;
    }

    int read_bytes = 0;
    // get type and than id first
    if (!c->id || !c->type) {
        char buffer[2];
        read_bytes = tcp_read(c->sock, buffer, 1);
        if (!c->type)
            c->type = buffer[0];
        else
            c->id = buffer[0];
    }

    if (read_bytes == -1)
        exit(EXIT_FAILURE);

    if (read_bytes)
        return 1;

    read_bytes = tcp_read(c->sock, c->buf + c->bufsize, BUFSIZE - 1);
    if (read_bytes == -1)
        exit(EXIT_FAILURE);

    c->bufsize += read_bytes;
    if (c->type == 'X') {
        c->buf[c->bufsize] = '\0';
        c->bufsize++;
    }

    // check if a valid record exists in buffer
    int bytesread;
    Record *r;
    if (c->type == 'X') {
        while (strstr(c->buf, "<record>") && strstr(c->buf, "</record>")) {
            r = XMLtoRecord(c->buf, c->bufsize, &bytesread);
            if (!r)
                break;

            c->bufsize = (int)strlen(c->buf);
            forwardMessage(r, client_arr);
            deleteRecord(r);
        }
    } else {
        do {
            r = BinaryToRecord(c->buf, c->bufsize, &bytesread);
            if (r) {
                memmove(c->buf, c->buf + bytesread, c->bufsize - bytesread + 1);
                c->bufsize -= bytesread;
                forwardMessage(r, client_arr);
                deleteRecord(r);
            }
        } while (r != NULL);
    }

    return 1;
}

void init_client_arr(Client *client_arr[MAXSERVER]) {
    for (int i = 0; i < MAXSERVER; i++) {
        client_arr[i] = NULL;
    }
}
int find_vacant_index(Client **client_arr) {
    for (int i = 0; i < MAXSERVER; i++) {
        if (client_arr[i] == NULL)
            return i;
    }
    fprintf(stderr, "Exceeded max amount of connected socket which is %d\n", MAXSERVER);
    exit(EXIT_FAILURE);
}

void insert_new_client(int client_sock, Client **client_arr) {
    int index = find_vacant_index(client_arr);
    client_arr[index] = handleNewClient(client_sock);
    client_arr[index]->index = index;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    int port = atoi(argv[1]);

    server_sock = tcp_create_and_listen(port);
    if (server_sock < 0)
        exit(-1);

    /* add your initialization code */
    int max_sock = server_sock;
    FD_ZERO(&fdset);
    FD_SET(server_sock, &fdset);

    // set all client's in this array to be NULL
    Client *clients_arr[MAXSERVER];
    init_client_arr(clients_arr);

    /*
     * The following part is the event loop of the proxy. It waits for new connections,
     * new data arriving on existing connection, and events that indicate that a client
     * has disconnected.
     *
     * This function uses handleNewClient() when activity is seen on the server socket
     * and handleClient() when activity is seen on the socket of an existing connection.
     *
     * The loops ends when no clients are connected any more.
     */
    printf("Proxy has started\n");
    int err;
    do {
        FD_ZERO(&fdset);
        FD_SET(server_sock, &fdset);
        // add the socket that is still connected, to FDSET
        for (int i = 0; i < MAXSERVER; i++) {
            if (clients_arr[i] != NULL)
                FD_SET(clients_arr[i]->sock, &fdset);
        }
        err = tcp_wait(&fdset, max_sock + 1);
        if (err == -1) {
            tcp_close(server_sock);
            return EXIT_FAILURE;
        }
        printf("Activated\n");
        for (int sd = 0; sd <= max_sock; sd++) {
            if (FD_ISSET(sd, &fdset)) {
                if (sd == server_sock) {
                    int client_sock = tcp_accept(server_sock);
                    FD_SET(client_sock, &fdset);

                    insert_new_client(client_sock, clients_arr);
                    connected_sockets++;
                    if (client_sock > max_sock)
                        max_sock = client_sock;
                    printf("New client has been connected\n");
                } else {
                    // handle the client connection
                    Client *c = find_client_ref(sd, clients_arr);
                    // if no socket was found something terrible has happened
                    if (c == NULL) {
                        fprintf(stderr, "Could not find the correct client from FD\n");
                        tcp_close(server_sock);
                        exit(EXIT_FAILURE);
                    }
                    // only clear this soket if its still connected
                    if (handleClient(c, clients_arr))
                        FD_CLR(c->sock, &fdset);
                }
            }
        }
    } while (connected_sockets > 0);
    printf("Proxy server is closing\n");
    tcp_close(server_sock);

    return 0;
}
