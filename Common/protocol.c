/*----------------------------------------------------------------------------
file: protocol.c
autore: Francesco Picone
Implementa le funzioni di protocollo per l'invio e la ricezione dei pacchetti
----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "protocol.h"


//Legge esattamente count byte s iterando opportunamente le letture. Legge anche se viene interrotta da una System Call.
ssize_t full_read(int fd, void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_read;
    n_left = count;
    while (n_left > 0) {  // repeat finchè non ci sono left
        if ((n_read = read(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; // Se si verifica una System Call che interrompe ripeti il ciclo
            else exit(n_read);
        } else if (n_read == 0) break; // Se sono finiti, esci
        n_left -= n_read;
        buffer += n_read;
    }
    buffer = 0;
    return n_left;
}


//Scrive esattamente count byte s iterando opportunamente le scritture. Scrive anche se viene interrotta da una System Call.
ssize_t full_write(int fd, const void *buffer, size_t count) {
    size_t n_left;
    ssize_t n_written;
    n_left = count;
    while (n_left > 0) {          //repeat finchè non ci sono left
        if ((n_written = write(fd, buffer, n_left)) < 0) {
            if (errno == EINTR) continue; //Se si verifica una System Call che interrompe ripeti il ciclo
            else exit(n_written); //Se non è una System Call, esci con un errore
        }
        n_left -= n_written;
        buffer += n_written;
    }
    buffer = 0;
    return n_left;
}

// Funzione wrapped con controllo sull'errore della full_read
ssize_t FullRead(int fd, void *buffer, size_t count){

	int nleft;

	if ( (nleft = full_read(fd, buffer, count)) < 0) {
		perror("full_read() error");
		exit(1);
	}

	return nleft;

}

// Funzione wrapped con controllo sull'errore della full_write
ssize_t FullWrite(int fd, const void *buffer, size_t count){

	int nleft;

	if( (nleft = full_write(fd, buffer, count)) < 0) {
		perror("full_write() error");
		exit(1);
	}

	return nleft;
}

int Socket(int namespace, int style, int protocol){

	 int listen_fd;

	 if ((listen_fd = socket(namespace, style, protocol)) < 0) {
	        perror("socket() error");
	        exit(1);
	 }

	 return listen_fd;

}
int Bind(int listen_fd, struct sockaddr *addr, socklen_t lenght){

	int result;

	if ( (result = bind(listen_fd, addr, lenght)) < 0) {
	        perror("bind() error");
	        exit(1);
	}

	return result;

}
int Listen(int listen_fd, int n){

	 int result;

	 if ( (result = listen(listen_fd, n)) < 0) {
	        perror("listen() error");
	        exit(1);
	 }

	 return result;
}

int Accept(int listen_fd, struct sockaddr *addr, socklen_t *lenght_ptr){

	int connfd;

	if ((connfd = accept(listen_fd, addr, lenght_ptr)) < 0) {
		perror("accept() error");
	    exit(1);
	}

	return connfd;
}
int Connect(int fd, struct sockaddr *addr, socklen_t lenght){

	int result;

	if (connect(fd, addr, lenght) < 0) {
		        perror("connect() error");
		        exit(1);
	}

	return result;
}
