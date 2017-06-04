#pragma once

#include "ssock.h"
#include "ringbuf.h"
#include <openssl\bio.h>
#include <openssl\ssl.h>
#include <openssl\err.h>

struct sssl;
struct sssl *sssl_alloc(struct ssock *fd);
void         sssl_free(struct sssl *self);
int          sssl_connect(struct sssl *self);
int          sssl_poll(struct sssl *self, char *buf, int sz);
int          sssl_send(struct sssl *self, char *buf, int sz);
void         sssl_set_connected(struct sssl *self, int v);
