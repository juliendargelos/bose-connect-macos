#ifndef BOSE_CONNECT_SRC_LIBRARY_TRANSPORT_H
#define BOSE_CONNECT_SRC_LIBRARY_TRANSPORT_H

#include <stddef.h>

#define TRANSPORT_DEVICE_NAME_MAX    248
#define TRANSPORT_DEVICE_ADDRESS_MAX 18

struct TransportDevice {
  char name[TRANSPORT_DEVICE_NAME_MAX];
  char address[TRANSPORT_DEVICE_ADDRESS_MAX];
  int  connected;
};

int transport_open(const char *address, unsigned char channel);

int transport_close(int handle);

int transport_read(int handle, void *buffer, size_t buffer_n);

int transport_write(int handle, const void *buffer, size_t buffer_n);

int transport_list_paired_devices(struct TransportDevice *devices,
                                  size_t max_devices, size_t *num_devices);

#endif
