#include "sssl.h"
#include "ssock.h"

#include <assert.h>
#include <string.h>

#define SSSL_NORMAL     0
#define SSSL_CONNECT    1
#define SSSL_CONNECTING 2
#define SSSL_CONNECTED  3
#define SSSL_SHUTDOWN   4
#define SSSL_CLOSE      5
#define SSSL_ERROR      6

struct sssl {
	SSL_CTX   *ssl_ctx;
	SSL       *ssl;
	BIO       *send_bio;
	BIO       *recv_bio;
	ringbuf_t  send_rb;
	ringbuf_t  recv_rb;
	struct ssock *fd;
	int        state;
};

static int
sssl_handle_err(struct sssl *self, int code, const char *tips);

/*
** @breif ֻ�ܰ��������ݶ������ȥ
** @return ��ȡ�����ݵ�size
*/
static int
sssl_write_ssock(struct sssl *self) {
	int w = 0;
	int l = 4096;
	char buf[4096] = { 0 };
	int nread = BIO_read(self->send_bio, buf, l);
	if (nread <= 0) {
		sssl_handle_err(self, nread, "sssl write ssock, BIO_read error.");
		return nread;
	}
	while (nread > 0) {
		ssock_writex(self->fd, buf + w, nread);
		w += nread;

		nread = BIO_read(self->send_bio, buf + w, l - w);
		if (nread <= 0) {
			sssl_handle_err(self, nread, "sssl write ssock. BIO_read error.");
			return nread;
		}
	}
	return w;
}

static int
sssl_read_data(struct sssl *self) {
	int lr = 0;
	int l = 4096;
	char buf[4096] = { 0 };
	int nread = SSL_read(self->ssl, buf, l);
	if (nread <= 0) {
		sssl_handle_err(self, nread, "sssl read data, BIO_read error.");
		return nread;
	}
	while (nread > 0) {
		ssock_datax(self->fd, buf + lr, nread);
		lr += nread;

		nread = SSL_read(self->ssl, buf + lr, l - lr);
		if (nread <= 0) {
			sssl_handle_err(self, nread, "sssl read data, BIO_read error.");
			return nread;
		}
	}

	printf("sssl read data length: %d bytes\r\n", lr);
	return lr;
}

/*
** @breif ֻ�ܴ������
*/
static int
sssl_handle_err(struct sssl *self, int code, const char *tips) {
	assert(self != NULL && tips != NULL);
	int err = SSL_get_error(self->ssl, code);
	if (err == SSL_ERROR_SSL) {
		printf("SSL_ERROR_SSL : %s\r\n", tips);
	} else if (err == SSL_ERROR_WANT_READ) {
		// waitting for poll buf.
		printf("SSL_ERROR_WANT_READ : %s\r\n", tips);
	} else if (err == SSL_ERROR_WANT_WRITE) {
		printf("SSL_ERROR_WANT_WRITE : %s\r\n", tips);
		sssl_write_ssock(self);
	} else if (err == SSL_ERROR_WANT_CONNECT) {
		printf("SSL_ERROR_WANT_WRITE : %s\r\n", tips);
	} else if (err == SSL_ERROR_SYSCALL) {
		printf("SSL_ERROR_SYSCALL : %s\r\n", tips);
	} else {
		printf("DEFAULT ERROR : %d\r\n", err);
	}
	return err;
}

struct sssl *
	sssl_alloc(struct ssock *fd) {
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ERR_load_BIO_strings();

	// ssl ctx
	struct sssl *inst = (struct sssl *)malloc(sizeof(*inst));
	memset(inst, 0, sizeof(*inst));
	inst->ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	// ssl
	inst->ssl = SSL_new(inst->ssl_ctx);

	// bio
	inst->send_bio = BIO_new(BIO_s_mem());
	inst->recv_bio = BIO_new(BIO_s_mem());

	inst->send_rb = ringbuf_new(4096);
	inst->recv_rb = ringbuf_new(4096);

	SSL_set_bio(inst->ssl, inst->recv_bio, inst->send_bio);

	inst->fd = fd;
	inst->state = SSSL_NORMAL;

	return inst;
}

void
sssl_free(struct sssl *self) {
	assert(self->state == SSSL_CLOSE);
	BIO_free(self->send_bio);
	BIO_free(self->recv_bio);

	SSL_free(self->ssl);

	SSL_CTX_free(self->ssl_ctx);
	ERR_free_strings();

	free(self);
}

int
sssl_connect(struct sssl *self) {
	self->state = SSSL_CONNECT;
	SSL_set_connect_state(self->ssl);
	int ret = SSL_connect(self->ssl);
	if (ret == 1) {
		printf("SSL_connect successfully.");
	} else {
		sssl_handle_err(self, ret, "SSL_connect.");
	}
	sssl_write_ssock(self);
	return ret;
}

/*
** @breif �˺�������socket���ݣ����������ݵ�ssl����������
** @return �������ݴ����Ƿ���ȷ����
**         ��ȷ�����ط��͵����ݣ����󷵻�0
*/

int
sssl_poll(struct sssl *self, const char *buf, int sz) {
	if (sz <= 0) {
		return 0;
	}
	// ȷ��buf����ȷ��
	assert(buf != NULL && sz > 0);

	int nw = 0;
	while (nw < sz) {
		int w = BIO_write(self->recv_bio, buf + nw, sz - nw);
		nw += w;
	}
	assert(nw == sz);
	if (SSSL_CONNECT <= self->state && self->state <= SSSL_CONNECTED) {
		// �ж�hanshake�Ƿ����
		if (!SSL_is_init_finished(self->ssl)) {
			sssl_set_state(self, SSSL_CONNECTING);
			int ret = SSL_do_handshake(self->ssl);
			sssl_write_ssock(self);
			if (ret == 1) {
				printf("openssl handshake success.\r\n");
				sssl_set_state(self, SSSL_CONNECTED);
			} else {
				sssl_handle_err(self, ret, "SSL_do_hanshake.");
			}
			return ret;
		} else {
			sssl_set_state(self, SSSL_CONNECTED);
			sssl_read_data(self);
		}
		// �ڲ��ж�ssl�����ǶϿ�
	} else if (self->state == SSSL_SHUTDOWN) {
		int ret = SSL_shutdown(self->ssl);
		if (ret == 1) {
			printf("SSL_shutdown successfully.");
		} else if (ret == 0) {
			sssl_handle_err(self, ret, "shutdown is not yet finished.");
		} else {
			sssl_handle_err(self, ret, "shutdown is not successful.");
		}
	}

	return nw;
}

int
sssl_send(struct sssl *self, const char *buf, int sz) {
	assert(self->state == SSSL_CONNECTED);
	if (sz <= 0) {
		return 0;
	}
	assert(buf != NULL && sz > 0);
	int w = SSL_write(self->ssl, buf, sz);
	if (w <= 0) { // not successful
		self->state = SSSL_ERROR;
		sssl_handle_err(self, w, "SSL_write error.");
		return w;
	}
	while (w < sz) {
		int tw = SSL_write(self->ssl, buf + w, sz - w);
		if (tw <= 0) {
			self->state = SSSL_ERROR;
			sssl_handle_err(self, tw, "SSL_write error.");
			return tw;
		}
		w += tw;
	}
	// д�����ݳɹ��������ݷ��ͳ�ȥ
	assert(w == sz);
	sssl_write_ssock(self);
	return w;
}

void
sssl_set_state(struct sssl *self, int v) {
	if (self->state != v) {
		self->state = v;
		ssock_set_ss(self->fd, v);
	}
}

int
sssl_shutdown(struct sssl *self, int how) {
	if (self->state == SSSL_CLOSE) {
		ssock_closex(self->fd);
		return 0;
	}
	self->state = SSSL_SHUTDOWN;
	if (how == 1) {
		SSL_set_shutdown(self->ssl, SSL_SENT_SHUTDOWN);
	} else if (how == 2) {
		SSL_set_shutdown(self->ssl, SSL_RECEIVED_SHUTDOWN);
	} else {
		SSL_set_shutdown(self->ssl, SSL_SENT_SHUTDOWN);
		SSL_set_shutdown(self->ssl, SSL_RECEIVED_SHUTDOWN);
	}

	SSL_shutdown(self->ssl);
	return 1;
}

int
sssl_close(struct sssl *self) {
	if (self->state == SSSL_CLOSE) {
		ssock_closex(self->fd);
		return 0;
	}
	return sssl_shutdown(self, 0);
}

int
sssl_clear(struct sssl *self) {
	return SSL_clear(self->ssl);
}