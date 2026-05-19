#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "transport.h"

@interface BoseRFCOMMDelegate : NSObject <IOBluetoothRFCOMMChannelDelegate>

@property(nonatomic, strong) NSMutableData *buffer;
@property(nonatomic, strong) NSLock *lock;
@property(nonatomic, assign) BOOL closed;

- (NSUInteger)consumeIntoBuffer:(void *)buffer maxLength:(NSUInteger)max_length;
- (BOOL)isClosed;

@end

@implementation BoseRFCOMMDelegate

- (instancetype)init {
  self = [super init];
  if (self == nil) {
    return nil;
  }

  _buffer = [NSMutableData data];
  _lock   = [[NSLock alloc] init];
  _closed = NO;
  return self;
}

- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel *)rfcommChannel
                     data:(void *)data_pointer
                   length:(size_t)data_length {
  (void)rfcommChannel;

  [self.lock lock];
  [self.buffer appendBytes:data_pointer length:data_length];
  [self.lock unlock];
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel *)rfcommChannel {
  (void)rfcommChannel;

  [self.lock lock];
  self.closed = YES;
  [self.lock unlock];
}

- (NSUInteger)consumeIntoBuffer:(void *)buffer maxLength:(NSUInteger)max_length {
  [self.lock lock];

  const NSUInteger available = self.buffer.length;
  const NSUInteger consume_n = MIN(available, max_length);
  if (consume_n == 0) {
    [self.lock unlock];
    return 0;
  }

  memcpy(buffer, self.buffer.bytes, consume_n);
  if (consume_n == available) {
    [self.buffer setLength:0];
  } else {
    const NSRange remaining = NSMakeRange(consume_n, available - consume_n);
    NSData       *tail      = [self.buffer subdataWithRange:remaining];
    [self.buffer setData:tail];
  }

  [self.lock unlock];
  return consume_n;
}

- (BOOL)isClosed {
  [self.lock lock];
  const BOOL is_closed = self.closed;
  [self.lock unlock];
  return is_closed;
}

@end

@interface BoseTransportConnection : NSObject

@property(nonatomic, strong) IOBluetoothRFCOMMChannel *channel;
@property(nonatomic, strong) BoseRFCOMMDelegate *delegate;

@end

@implementation BoseTransportConnection
@end

static NSLock *get_connections_lock(void) {
  static NSLock *lock = nil;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    lock = [[NSLock alloc] init];
  });
  return lock;
}

static NSMutableDictionary *get_connections(void) {
  static NSMutableDictionary *connections = nil;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    connections = [NSMutableDictionary dictionary];
  });
  return connections;
}

static int get_next_handle(void) {
  static int next_handle = 1;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    next_handle = 1;
  });

  const int handle = next_handle;
  next_handle += 1;
  if (next_handle <= 0) {
    next_handle = 1;
  }

  return handle;
}

static int store_connection(BoseTransportConnection *connection) {
  NSLock              *lock        = get_connections_lock();
  NSMutableDictionary *connections = get_connections();

  [lock lock];
  const int handle = get_next_handle();
  [connections setObject:connection forKey:@(handle)];
  [lock unlock];

  return handle;
}

static BoseTransportConnection *get_connection(int handle) {
  NSLock              *lock        = get_connections_lock();
  NSMutableDictionary *connections = get_connections();

  [lock lock];
  BoseTransportConnection *connection =
      [connections objectForKey:@(handle)];
  [lock unlock];
  return connection;
}

static BoseTransportConnection *remove_connection(int handle) {
  NSLock              *lock        = get_connections_lock();
  NSMutableDictionary *connections = get_connections();

  [lock lock];
  BoseTransportConnection *connection =
      [connections objectForKey:@(handle)];
  if (connection != nil) {
    [connections removeObjectForKey:@(handle)];
  }
  [lock unlock];

  return connection;
}

int transport_open(const char *address, unsigned char channel) {
  @autoreleasepool {
    if (address == NULL || address[0] == '\0') {
      errno = EINVAL;
      return -1;
    }

    NSString *address_string = [NSString stringWithUTF8String:address];
    if (address_string == nil) {
      errno = EINVAL;
      return -1;
    }

    IOBluetoothDevice *device =
        [IOBluetoothDevice deviceWithAddressString:address_string];
    if (device == nil) {
      errno = ENODEV;
      return -1;
    }

    BoseRFCOMMDelegate *delegate = [[BoseRFCOMMDelegate alloc] init];
    if (delegate == nil) {
      errno = ENOMEM;
      return -1;
    }

    IOBluetoothRFCOMMChannel *rfcomm_channel = nil;
    const IOReturn status =
        [device openRFCOMMChannelSync:&rfcomm_channel
                        withChannelID:(BluetoothRFCOMMChannelID)channel
                             delegate:delegate];
    if (status != kIOReturnSuccess || rfcomm_channel == nil) {
      errno = ECONNREFUSED;
      return -1;
    }

    (void)[rfcomm_channel setDelegate:delegate];

    BoseTransportConnection *connection = [[BoseTransportConnection alloc] init];
    if (connection == nil) {
      (void)[rfcomm_channel closeChannel];
      errno = ENOMEM;
      return -1;
    }

    connection.channel  = rfcomm_channel;
    connection.delegate = delegate;
    return store_connection(connection);
  }
}

int transport_close(int handle) {
  @autoreleasepool {
    BoseTransportConnection *connection = remove_connection(handle);
    if (connection == nil) {
      errno = EBADF;
      return -1;
    }

    const IOReturn status = [connection.channel closeChannel];
    if (status != kIOReturnSuccess && status != kIOReturnNotOpen) {
      errno = EIO;
      return -1;
    }

    return 0;
  }
}

int transport_read(int handle, void *buffer, size_t buffer_n) {
  @autoreleasepool {
    static const CFTimeInterval read_timeout_seconds = 5.0;
    static const CFTimeInterval poll_interval_seconds = 0.01;

    if (buffer_n == 0) {
      return 0;
    }

    if (buffer == NULL || buffer_n > INT_MAX) {
      errno = EINVAL;
      return -1;
    }

    BoseTransportConnection *connection = get_connection(handle);
    if (connection == nil) {
      errno = EBADF;
      return -1;
    }

    const CFAbsoluteTime deadline =
        CFAbsoluteTimeGetCurrent() + read_timeout_seconds;
    while (CFAbsoluteTimeGetCurrent() < deadline) {
      const NSUInteger received_n =
          [connection.delegate consumeIntoBuffer:buffer maxLength:buffer_n];
      if (received_n > 0) {
        return (int)received_n;
      }

      if ([connection.delegate isClosed]) {
        return 0;
      }

      CFRunLoopRunInMode(kCFRunLoopDefaultMode, poll_interval_seconds, false);
    }

    errno = EAGAIN;
    return -1;
  }
}

int transport_write(int handle, const void *buffer, size_t buffer_n) {
  @autoreleasepool {
    if (buffer_n == 0) {
      return 0;
    }

    if (buffer == NULL || buffer_n > INT_MAX) {
      errno = EINVAL;
      return -1;
    }

    BoseTransportConnection *connection = get_connection(handle);
    if (connection == nil) {
      errno = EBADF;
      return -1;
    }

    const BluetoothRFCOMMMTU mtu = [connection.channel getMTU];
    const size_t             chunk_max = mtu > 0 ? (size_t)mtu : 256;

    const uint8_t *to_send = buffer;
    size_t         total   = 0;
    while (total < buffer_n) {
      size_t chunk_n = buffer_n - total;
      if (chunk_n > chunk_max) {
        chunk_n = chunk_max;
      }

      const IOReturn status =
          [connection.channel writeSync:(void *)&to_send[total]
                                 length:(UInt16)chunk_n];
      if (status != kIOReturnSuccess) {
        errno = EIO;
        return total > 0 ? (int)total : -1;
      }

      total += chunk_n;
    }

    return (int)total;
  }
}

int transport_list_paired_devices(struct TransportDevice *devices,
                                  size_t max_devices, size_t *num_devices) {
  @autoreleasepool {
    if (num_devices == NULL) {
      errno = EINVAL;
      return -1;
    }

    NSArray *paired_devices = [IOBluetoothDevice pairedDevices];
    if (paired_devices == nil) {
      *num_devices = 0;
      return 0;
    }

    *num_devices = paired_devices.count;
    if (devices == NULL || max_devices == 0) {
      return 0;
    }

    const size_t count = paired_devices.count < max_devices
                             ? paired_devices.count
                             : max_devices;

    for (size_t i = 0; i < count; ++i) {
      IOBluetoothDevice *device = paired_devices[i];
      NSString *name_or_address = [device nameOrAddress];
      NSString *address_string  = [device addressString];

      devices[i].name[0]    = '\0';
      devices[i].address[0] = '\0';

      if (name_or_address != nil) {
        (void)strncpy(devices[i].name, name_or_address.UTF8String,
                      TRANSPORT_DEVICE_NAME_MAX - 1);
        devices[i].name[TRANSPORT_DEVICE_NAME_MAX - 1] = '\0';
      }

      if (address_string != nil) {
        (void)strncpy(devices[i].address, address_string.UTF8String,
                      TRANSPORT_DEVICE_ADDRESS_MAX - 1);
        devices[i].address[TRANSPORT_DEVICE_ADDRESS_MAX - 1] = '\0';

        for (size_t j = 0; devices[i].address[j] != '\0'; ++j) {
          if (devices[i].address[j] == '-') {
            devices[i].address[j] = ':';
          }
        }
      }

      devices[i].connected = [device isConnected] ? 1 : 0;
    }

    return 0;
  }
}
