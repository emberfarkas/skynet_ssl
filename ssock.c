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



//struct ssock {
//	int fd;
//	struct sssl *sssl;
//	recv_cb      cb;
//	int        connected;  // 1: ok, 0:
//};

struct ssock *
	ssock_alloc(recv_cb cb, void *ud) {

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
	inst->cb = cb;
	inst->ud = ud;

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

int            ssock_connect(struct ssock *self, char *addr, int port) {
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
ssock_poll(struct ssock *self, char *buf, int size) {
	return sssl_poll(self->sssl, buf, size);
}

int
ssock_send(struct ssock *self, char *buf, int size) {
	return sssl_send(self->sssl, buf, size);
}

int
ssock_write(struct ssock *self, const char *buf, int size) {
	return ssockaux_write(self->ud, buf, size);
	//return send(self->fd, buf, size, 0);
}

int
ssock_data(struct ssock *self, const char *buf, int size) {
	if (self->cb != NULL) {
		self->cb(buf, size, self->ud);
	}
	return 1;
}

int
ssock_shutdown(struct ssock *self, int how) {
#ifdef TEST_SOCK
	return shutdown(self->fd, how);
#else
	return ssockaux_shutdown(self->ud, how);
#endif // TEST_SOCK

	
}

int
ssock_close(struct ssock *self) {
#ifdef TEST_SOCK
	return closesocket(self->fd);
#else
	return ssockaux_close(self->ud);
#endif
}