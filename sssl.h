#pragma once
#ifndef SSSL_H
#define SSSL_H

#include "ssock.h"
#include "ringbuf.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

struct sssl;
struct sssl *sssl_alloc(struct ssock *fd);
void         sssl_free(struct sssl *self);
int          sssl_connect(struct sssl *self);
int          sssl_poll(struct sssl *self, const char *buf, int sz);
int          sssl_send(struct sssl *self, const char *buf, int sz);
void         sssl_set_connected(struct sssl *self, int v);


#endif // !SSSL_H
