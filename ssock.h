#pragma once
#include "sssl.h"
#if WIN32
#include <Winsock2.h>
#include <Wininet.h>
#include <ws2tcpip.h>
#include <Windows.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <netdb.h>
#include <sys/select.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>


typedef void(*recv_cb)(void *data, int dlen, void *ud);

struct ssock {
	int fd;
	struct sssl *sssl;
	void        *ud;
	recv_cb      cb;
	int          connected;  // 1: ok, 0:
};

#define        ssock_set_sssl(self, v) ((self)->sssl = (v))
#define        ssock_set_connected(self, v) ((self)->connected = (v))
#define        ssock_connected(self) ((self)->connected)

struct ssock * ssock_alloc(recv_cb cb, void *ud);
void           ssock_free(struct ssock *self);


int            ssock_connect(struct ssock *self, char *addr, int port);

int            ssock_update(struct ssock *self);
int            ssock_poll(struct ssock *self, char *buf, int size);
int            ssock_send(struct ssock *self, char *buf, int size);

int            ssock_write(struct ssock *self, char *buf, int size);
int            ssock_data(struct ssock *self, char *buf, int size);

int            ssock_shutdown(struct ssock *self, int how);
int            ssock_close(struct ssock *self);