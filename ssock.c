#include "ssock.h"
#include "lssock.h"
//#define TEST_SOCK
#ifdef TEST_SOCK
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
#endif // TEST_SOCK
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

struct ssock *
ssock_alloc(struct ssock_cb *cb) {
	assert(cb != NULL);
#ifdef TEST_SOCK
#if defined(_WIN32)
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
	}
#endif
#endif

	struct ssock *inst = (struct ssock *)malloc(sizeof(*inst));
	int fd = 0;
#ifdef TEST_SOCK
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		//printf("创建socket 失败， ");
		exit(0);
	}
	printf_s("创建套接字成功\r\n");

#endif // TEST_SOCK
	inst->fd = fd;
	inst->callback = *cb;
	
	inst->sssl = sssl_alloc(inst);
	inst->connected = 0;
	return inst;
}

void
ssock_free(struct ssock *self) {
	sssl_free(self->sssl);
#ifdef TEST_SOCK
#if defined(_WIN32)
	WSACleanup();
#endif // defined(_WIN32)
#endif
}

int            
ssock_connect(struct ssock *self, const char *addr, int port) {
#ifdef TEST_SOCK
	struct sockaddr_in add;
	add.sin_family = AF_INET;
	if (inet_pton(AF_INET, addr, &add.sin_addr) < 0) {
		return -1;
	}
	add.sin_port = htons(port);
	int res = connect(self->fd, (struct sockaddr *)&add, sizeof(add));
	if (res < 0) {
#ifdef _WIN32
		int err = WSAGetLastError();
		if (err == EWOULDBLOCK) {

		}
#endif // _WIN32
		return -1;
	}
	printf_s("链接成功\r\n");
#endif // TEST_SOCK
	return sssl_connect(self->sssl);
}

int
ssock_update(struct ssock *self) {
#ifdef TEST_SOCK
	char buf[4096] = { 0 };
	int nread = recv(self->fd, buf, 4096, 0);
	if (nread > 0) {
		ssock_poll(self, buf, nread);
	}
#endif
	return 1;
}

int
ssock_poll(struct ssock *self, const char *buf, int size) {
	return sssl_poll(self->sssl, buf, size);
}

int
ssock_send(struct ssock *self, const char *buf, int size) {
	return sssl_send(self->sssl, buf, size);
}

int
ssock_shutdown(struct ssock *self, int how) {
#ifdef TEST_SOCK
	return shutdown(self->fd, how);
#else
	// shutdown ssl
	return sssl_shutdown(self->sssl, how);
#endif // TEST_SOCK
}

int
ssock_close(struct ssock *self) {
#ifdef TEST_SOCK
	return closesocket(self->fd);
#else
	// close ssl
	return sssl_close(self->sssl);
#endif
}

int
ssock_writex(struct ssock *self, const char *buf, int size) {
#ifdef TEST_SOCK
	return send(self->fd, buf, size, 0);
#else
	return self->callback.write_callback(buf, size, self->callback.ud);
#endif // TEST_SOCK
}

int
ssock_datax(struct ssock *self, const char *buf, int size) {
	self->callback.data_callback(buf, size, self->callback.ud);
	return 1;
}

int            
ssock_shutdownx(struct ssock *self, int how) {
	return self->callback.shutdown_callback(how, self->callback.ud);
}

int            
ssock_closex(struct ssock *self) {
	return self->callback.close_callback(self->callback.ud);
}