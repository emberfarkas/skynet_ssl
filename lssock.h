#pragma once
#ifndef LSSOCK_H
#define LSSOCK_H

struct ssockaux;
void ssockaux_data(struct ssockaux *self, void *data, int dlen);
int ssockaux_write(struct ssockaux *self, void *data, int dlen);
int ssockaux_shutdown(struct ssockaux *self, int how);
int ssockaux_close(struct ssockaux *self);

#endif // !LSSOCK_H
