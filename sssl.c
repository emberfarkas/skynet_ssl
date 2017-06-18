#include "sssl.h"
#include "ssock.h"

#include <assert.h>
#include <string.h>

struct sssl {
	SSL_CTX   *ssl_ctx;
	SSL       *ssl;
	BIO       *send_bio;
	BIO       *recv_bio;
	ringbuf_t  send_rb;
	ringbuf_t  recv_rb;
	struct ssock *fd;
	int        connected;  // 1: ok, 0:
};

static int
sssl_write_ssock(struct sssl *self) {
	int w = 0;
	int b = 0;
	int e = 0;
	int l = 4096;
	char buf[4096] = { 0 };
	int nread = BIO_read(self->send_bio, buf, l);
	while (nread > 0) {
		b += nread;
		int nwrite = ssock_writex(self->fd, buf + e, b - e);
		w += nwrite;
		e += nwrite;

		nread = BIO_read(self->send_bio, buf + b, l - b);
	}
	while (b != e) {
		int nwrite = ssock_writex(self->fd, buf + e, b - e);
		w += nwrite;
		e += nwrite;
	}
	return nread;
}

static int
sssl_read_data(struct sssl *self) {
	int r = 0;
	int b = 0;
	int e = 0;
	int l = 4096;
	char buf[4096] = { 0 };
	int nread = SSL_read(self->ssl, buf, l);
	while (nread > 0) {
		b += nread;
		r += nread;

		ssock_datax(self->fd, buf + e, b - e);
		e += nread;

		nread = SSL_read(self->ssl, buf + b, l - b);
	}

	printf("接受socket数据 %d bytes\r\n", r);
	return 1;
}

static int
sssl_handle_err(struct sssl *self, int code) {
	if (code != 1) {
		int err = SSL_get_error(self->ssl, code);
		if (err == SSL_ERROR_WANT_READ) {
			// 等待
			printf("want read.\r\n");
		} else if (err == SSL_ERROR_WANT_WRITE) {
			sssl_write_ssock(self);
		} else if (err == SSL_ERROR_SSL) {
			printf("error ssl");
		} else if (err == SSL_ERROR_WANT_CONNECT) {
			printf("error want connect");
		}
	}
	return 1;
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

	return inst;
}

void
sssl_free(struct sssl *self) {
	BIO_free(self->send_bio);
	BIO_free(self->recv_bio);

	SSL_shutdown(self->ssl);
	SSL_free(self->ssl);

	SSL_CTX_free(self->ssl_ctx);
	ERR_free_strings();

	free(self);
}

int
sssl_connect(struct sssl *self) {
	SSL_set_connect_state(self->ssl);
	int ret = SSL_connect(self->ssl);

	// 第一次
	sssl_write_ssock(self);

	if (ret != 1) {
		sssl_handle_err(self, ret);
	}
	return 1;
}

int
sssl_poll(struct sssl *self, const char *buf, int sz) {
	if (sz <= 0) {
		return 0;
	}

	// 确保buf是正确的
	assert(buf != NULL && sz > 0);
	int nw = BIO_write(self->recv_bio, buf, sz);
	while (nw < sz) {
		nw = BIO_write(self->recv_bio, buf + nw, sz - nw);
	}

	// 判断hanshake是否完成
	if (!SSL_is_init_finished(self->ssl)) {
		sssl_set_connected(self, 0);
		int ret = SSL_do_handshake(self->ssl);
		sssl_write_ssock(self);
		if (ret != 1) {
			sssl_handle_err(self, ret);
			return ret;
		} else {
			printf("openssl handshake success.\r\n");
			sssl_set_connected(self, 1);
			//sssl_read_bio(self);
		}
	} else {
		sssl_set_connected(self, 1);
		sssl_read_data(self);
	}
	return 1;
}

int
sssl_send(struct sssl *self, const char *buf, int sz) {
	assert(self->connected == 1);
	if (sz <= 0) {
		return 0;
	}
	assert(buf != NULL && sz > 0);
	int ret = SSL_write(self->ssl, buf, sz);
	while (ret < sz) {
		sz -= ret;
		ret = SSL_write(self->ssl, buf + ret, sz);
	}
	if (ret > 0) {
		sssl_write_ssock(self);
	} else {
		sssl_handle_err(self, ret);
	}
	return ret;
}

void
sssl_set_connected(struct sssl *self, int v) {
	if (self->connected != v) {
		self->connected = v;
		ssock_set_connected(self->fd, v);
	}
}

int          
sssl_shutdown(struct sssl *self, int how) {
	return ssock_shutdownx(self->fd, how);
}

int          
sssl_close(struct sssl *self) {
	return ssock_closex(self->fd);
}