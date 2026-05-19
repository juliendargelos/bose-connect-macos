#ifndef BOSE_CONNECT_APP_LINUX_SRC_LIBRARY_TRANSPORT_H
#define BOSE_CONNECT_APP_LINUX_SRC_LIBRARY_TRANSPORT_H

#include <stddef.h>

int transport_open(const char *address, unsigned char channel);

int transport_close(int handle);

int transport_read(int handle, void *buffer, size_t buffer_n);

int transport_write(int handle, const void *buffer, size_t buffer_n);

#endif
