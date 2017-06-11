#pragma once
#ifndef SSOCK_H
#define SSOCK_H

#include "sssl.h"
#include "lssock.h"

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

int            ssock_write(struct ssock *self, const char *buf, int size);
int            ssock_data(struct ssock *self, const char *buf, int size);

int            ssock_shutdown(struct ssock *self, int how);
int            ssock_close(struct ssock *self);

#endif // !SSOCK_H

