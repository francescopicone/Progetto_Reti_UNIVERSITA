
/*----------------------------------------------------------------------------
file: protocol.h
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

ssize_t full_write(int fd, const void *buffer, size_t count);
ssize_t full_read(int fd, void *buffer, size_t count);

ssize_t FullRead(int fd, void *buffer, size_t count);
ssize_t FullWrite(int fd, const void *buffer, size_t count);
int Socket(int namespace, int style, int protocol);
int Bind(int listen_fd, struct sockaddr *addr, socklen_t lenght);
int Listen(int listen_fd, int n);
int Accept(int listen_fd, struct sockaddr *addr, socklen_t *lenght_ptr);
int Connect(int fd, struct sockaddr *addr, socklen_t lenght);
