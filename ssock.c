#include "ssock.h"

//struct ssock {
//	int fd;
//	struct sssl *sssl;
//	recv_cb      cb;
//	int        connected;  // 1: ok, 0:
//};

struct ssock *
	ssock_alloc(recv_cb cb, void *ud) {
#if defined(_WIN32)
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
	}
#endif

	struct ssock *inst = (struct ssock *)malloc(sizeof(*inst));
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		//printf("����socket ʧ�ܣ� ");
		exit(0);
	}

	printf_s("�����׽��ֳɹ�\r\n");
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

#if defined(_WIN32)
	WSACleanup();
#endif // defined(_WIN32)
}

int            ssock_connect(struct ssock *self, char *addr, int port) {
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

	printf_s("���ӳɹ�\r\n");
	sssl_connect(self->sssl);
}

int
ssock_update(struct ssock *self) {
	char buf[4096] = { 0 };
	int nread = recv(self->fd, buf, 4096, 0);
	if (nread > 0) {
		ssock_poll(self, buf, nread);
	}
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
ssock_write(struct ssock *self, char *buf, int size) {
	return send(self->fd, buf, size, 0);
}

int
ssock_data(struct ssock *self, char *buf, int size) {
	if (self->cb != NULL) {
		self->cb(buf, size, self->ud);
	}
	return 1;
}

int
ssock_shutdown(struct ssock *self, int how) {
	return shutdown(self->fd, how);
}

int
ssock_close(struct ssock *self) {
	return closesocket(self->fd);
}