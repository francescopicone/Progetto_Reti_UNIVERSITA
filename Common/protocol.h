
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
