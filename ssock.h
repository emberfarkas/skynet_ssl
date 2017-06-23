#pragma once
#ifndef SSOCK_H
#define SSOCK_H

#include "sssl.h"
#include "lssock.h"

typedef enum ssock_state {
	ss_normal,
	ss_connect,
	ss_connecting,
	ss_connected,
	ss_shutdown,
	ss_close,
	ss_error,
} ssock_state;

typedef void (*data_cb)(const char *data, int dlen, void *ud);
typedef int (*write_cb)(const char *data, int dlen, void *ud);
typedef int (*shutdown_cb)(int how, void *ud);
typedef int (*close_cb)(void *ud);

struct ssock_cb {
	void *ud;
	data_cb     data_callback;
	write_cb    write_callback;
	shutdown_cb shutdown_callback;
	close_cb    close_callback;
};

struct ssock {
	int fd;
	int ss;  // 1: ok, 0:
	struct sssl *sssl;
	struct ssock_cb     callback;
};

#define        ssock_set_sssl(self, v) ((self)->sssl = (v))
#define        ssock_set_ss(self, v) ((self)->ss = (v))
#define        ssock_ss(self) ((self)->ss)

struct ssock * ssock_alloc(struct ssock_cb *cb);
void           ssock_free(struct ssock *self);


int            ssock_connect(struct ssock *self, const char *addr, int port);

int            ssock_update(struct ssock *self);
int            ssock_poll(struct ssock *self, const char *buf, int size);
int            ssock_send(struct ssock *self, const char *buf, int size);
int            ssock_shutdown(struct ssock *self, int how);
int            ssock_close(struct ssock *self);
int            ssock_clear(struct ssock *self);

/* ------------called been sssl ---------------------------------------------------------*/

/*
 * @breif 返回write了的数据的长度
*/
int            ssock_writex(struct ssock *self, const char *buf, int size);
/*
 * @breif 接受到的数据无论返回
 */
int            ssock_datax(struct ssock *self, const char *buf, int size);

int            ssock_shutdownx(struct ssock *self, int how);
int            ssock_closex(struct ssock *self);

#endif // !SSOCK_H

