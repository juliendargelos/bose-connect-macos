#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "based.h"
#include "transport.h"
#include "util.h"

#define ANY              0x00
#define CN_BASE_PACK_LEN 4
#define MAX_NAME_PACKAGE (CN_BASE_PACK_LEN + MAX_NAME_LEN - 1)
#define GET_DEVICE_ID_SEND                                                     \
  { 0x00, 0x03, 0x01, 0x00 }
#define GET_DEVICE_ID_ACK                                                      \
  { 0x00, 0x03, 0x03, 0x03 }
#define GET_NAME_ACK                                                           \
  { 0x01, 0x02, 0x03, ANY, 0x00 }
#define GET_NAME_MASK                                                          \
  { 0xff, 0xff, 0xff, 0x00, 0xff }
#define SET_NAME_SEND                                                          \
  { 0x01, 0x02, 0x02, ANY }
#define GET_PROMPT_LANGUAGE_ACK                                                \
  { 0x01, 0x03, 0x03, 0x05, ANY, 0x00, ANY, ANY, 0xde }
#define GET_PROMPT_LANGUAGE_MASK                                               \
  { 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff }
#define SET_PROMPT_LANGUAGE_SEND                                               \
  { 0x01, 0x03, 0x02, 0x01, ANY }
#define GET_AUTO_OFF_ACK                                                       \
  { 0x01, 0x04, 0x03, 0x01, ANY }
#define GET_AUTO_OFF_MASK                                                      \
  { 0xff, 0xff, 0xff, 0xff, 0x00 }
#define SET_AUTO_OFF_SEND                                                      \
  { 0x01, 0x04, 0x02, 0x01, ANY }
#define GET_NOISE_CANCELLING_ACK                                               \
  { 0x01, 0x06, 0x03, 0x02, ANY, 0x0b }
#define GET_NOISE_CANCELLING_MASK                                              \
  { 0xff, 0xff, 0xff, 0xff, 0x00, 0xff }
#define SET_NOISE_CANCELLING_SEND                                              \
  { 0x01, 0x06, 0x02, 0x01, ANY }
#define SET_NOISE_MODE_SEND                                                     \
  { 0x1f, 0x03, 0x05, 0x02, ANY, 0x01, ANY }
#define SET_NOISE_MODE_ACK                                                      \
  { 0x1f, 0x03, 0x07, 0x00 }
#define NOISE_CANCELLING_14 0x4014
#define NOISE_CANCELLING_20 0x4020
#define NOISE_CANCELLING_0C 0x400c
#define NOISE_CANCELLING_75 0x4075
#define GET_DEVICE_STATUS_SEND                                                 \
  { 0x01, 0x01, 0x05, 0x00 }
#define GET_DEVICE_STATUS_ACK                                                  \
  { 0x01, 0x01, 0x07, 0x00 }
#define GET_FIRMWARE_VERSION_SEND                                              \
  { 0x00, 0x05, 0x01, 0x00 }
#define GET_FIRMWARE_VERSION_ACK                                               \
  { 0x00, 0x05, 0x03, 0x05 }
#define GET_SERIAL_NUMBER_SEND                                                 \
  { 0x00, 0x07, 0x01, 0x00 }
#define GET_SERIAL_NUMBER_ACK                                                  \
  { 0x00, 0x07, 0x03 }
#define GET_BATTERY_LEVEL_SEND                                                 \
  { 0x02, 0x02, 0x01, 0x00 }
#define GET_BATTERY_LEVEL_ACK                                                  \
  { 0x02, 0x02, 0x03, 0x01 }
#define GET_PAIRED_DEVICES_SEND                                                \
  { 0x04, 0x04, 0x01, 0x00 }
#define GET_PAIRED_DEVICES_ACK                                                 \
  { 0x04, 0x04, 0x03 }
#define INIT_CONNECTION_SEND                                                   \
  { 0x00, 0x01, 0x01, 0x00 }
#define INIT_CONNECTION_ACK                                                    \
  { 0x00, 0x01, 0x03, 0x05 }
#define SET_PARING_SEND_PACKAGE                                                \
  { 0x04, 0x08, 0x05, 0x01, ANY }
#define SET_PARING_ACK_PACKAGE                                                 \
  { 0x04, 0x08, 0x06, 0x01, ANY }
#define SET_SELF_VOICE_SEND_PACKAGE                                            \
  { 0x01, 0x0b, 0x02, 0x02, 0x01, ANY, 0x38 }
#define SET_SELF_VOICE_ACK_PACKAGE                                             \
  { 0x01, 0x0b, 0x03, 0x03, 0x01, ANY, 0x0f }
#define GET_DEVICE_INFO_SEND_PACKAGE                                           \
  { 0x04, 0x05, 0x01, BT_ADDR_LEN }
#define GET_DEVICE_INFO_ACK_PACKAGE                                            \
  { 0x04, 0x05, 0x03 }
#define CONNECT_DEVICE_SEND                                                    \
  { 0x04, 0x01, 0x05, BT_ADDR_LEN + 1, 0x00 }
#define CONNECT_DEVICE_ACK                                                     \
  { 0x04, 0x01, 0x07, BT_ADDR_LEN }
#define DISCONNECT_DEVICE_SEND                                                 \
  { 0x04, 0x02, 0x05, BT_ADDR_LEN }
#define DISCONNECT_DEVICE_ACK                                                  \
  { 0x04, 0x02, 0x07, BT_ADDR_LEN }
#define REMOVE_DEVICE_SEND                                                     \
  { 0x04, 0x03, 0x05, BT_ADDR_LEN }
#define REMOVE_DEVICE_ACK                                                      \
  { 0x04, 0x03, 0x06, BT_ADDR_LEN }
#define BYTES_POSITION_2  2
#define BYTES_POSITION_3  3
#define BYTES_POSITION_4  4
#define BYTES_POSITION_5  5
#define BYTES_POSITION_10 10
#define BYTES_POSITION_11 11

int has_noise_cancelling(unsigned int device_id) {
  switch (device_id) {
  case NOISE_CANCELLING_14:
  case NOISE_CANCELLING_20:
  case NOISE_CANCELLING_0C:
  case NOISE_CANCELLING_75:
    return 1;
  default:
    return 0;
  }
}

static int masked_memory_cmp(const uint8_t *ptr1, uint8_t *ptr2, size_t num,
                             const uint8_t *mask) {
  while (num-- > 0) {
    uint8_t mask_byte = *mask++;
    uint8_t byte1     = *ptr1++ & mask_byte;
    uint8_t byte2     = *ptr2++ & mask_byte;

    if (byte1 != byte2) {
      return byte1 - byte2;
    }
  }

  return 0;
}

static int read_exact(int sock, void *receive, size_t receive_n) {
  uint8_t *buffer = receive;
  size_t   total  = 0;

  while (total < receive_n) {
    int status = transport_read(sock, &buffer[total], receive_n - total);
    if (status <= 0) {
      return status;
    }

    total += (size_t)status;
  }

  return (int)total;
}

static int read_check(int sock, uint8_t *receive, size_t receive_n,
                      const uint8_t *ack, const uint8_t *mask) {
  int status = read_exact(sock, receive, receive_n);
  if (status != (int)receive_n) {
    return status ? status : 1;
  }

  return abs(mask ? masked_memory_cmp(ack, receive, receive_n, mask)
                  : memcmp(ack, receive, receive_n));
}

static int write_check(int sock, const void *send, size_t send_n,
                       const void *ack, size_t ack_n) {
  uint8_t buffer[ack_n];

  int status = transport_write(sock, send, send_n);
  if (status != (int)send_n) {
    return status ? status : 1;
  }
  return read_check(sock, buffer, sizeof(buffer), ack, NULL);
}

int send_packet(int sock, const void *send, size_t send_n,
                uint8_t received[MAX_BT_PACK_LEN]) {
  int status = transport_write(sock, send, send_n);
  if (status != (int)send_n) {
    return status ? status : 1;
  }

  return transport_read(sock, received, MAX_BT_PACK_LEN);
}

int init_connection(int sock) {
  static const uint8_t send[] = INIT_CONNECTION_SEND;
  static const uint8_t ack[]  = INIT_CONNECTION_ACK;

  int status = write_check(sock, send, sizeof(send), ack, sizeof(ack));
  if (status) {
    return status;
  }

  // Throw away the initial firmware version
  uint8_t garbage[BYTES_POSITION_5];
  status = read_exact(sock, garbage, sizeof(garbage));

  if (status != (int)sizeof(garbage)) {
    return status ? status : 1;
  }

  return 0;
}

int get_device_id(int sock, unsigned int *device_id, unsigned int *index) {
  static const uint8_t send[] = GET_DEVICE_ID_SEND;
  static const uint8_t ack[]  = GET_DEVICE_ID_ACK;

  int status = write_check(sock, send, sizeof(send), ack, sizeof(ack));
  if (status) {
    return status;
  }

  uint16_t device_id_half_word = 0;
  status = read_exact(sock, &device_id_half_word, sizeof(device_id_half_word));
  if (status != (int)sizeof(device_id_half_word)) {
    return status ? status : 1;
  }

  *device_id = __builtin_bswap16(device_id_half_word);

  uint8_t index_byte = 0;
  status             = read_exact(sock, &index_byte, 1);
  if (status != 1) {
    return status ? status : 1;
  }
  *index = index_byte;

  return 0;
}

static int get_name(int sock, char name[MAX_NAME_LEN]) {
  static const uint8_t ack[]  = GET_NAME_ACK;
  static const uint8_t mask[] = GET_NAME_MASK;
  uint8_t              buffer[sizeof(ack)];

  int status = read_check(sock, buffer, sizeof(buffer), ack, mask);
  if (status) {
    return status;
  }

  size_t length = (size_t)(buffer[BYTES_POSITION_3] - 1);
  status        = read_exact(sock, name, length);
  if (status != (int)length) {
    return status ? status : 1;
  }
  name[length] = '\0';

  return 0;
}

int set_name(int sock, const char *name) {
  static uint8_t send[MAX_NAME_PACKAGE] = SET_NAME_SEND;
  size_t         length                 = strlen(name);

  send[BYTES_POSITION_3] = (uint8_t)length;
  str_copy((char *)&send[CN_BASE_PACK_LEN], name, MAX_NAME_LEN);

  size_t send_size = CN_BASE_PACK_LEN + length;
  int    status    = transport_write(sock, send, send_size);
  if (status != (int)send_size) {
    return status ? status : 1;
  }

  char got_name[MAX_NAME_LEN];
  status = get_name(sock, got_name);
  if (status) {
    return status;
  }

  return abs(strcmp(name, got_name));
}

enum PromptLanguage get_language(const char *language) {
  if (strcmp(language, "en") == 0) {
    return PL_EN;
  }

  if (strcmp(language, "fr") == 0) {
    return PL_FR;
  }

  if (strcmp(language, "it") == 0) {
    return PL_IT;
  }

  if (strcmp(language, "de") == 0) {
    return PL_DE;
  }

  if (strcmp(language, "es") == 0) {
    return PL_ES;
  }

  if (strcmp(language, "pt") == 0) {
    return PL_PT;
  }

  if (strcmp(language, "zh") == 0) {
    return PL_ZH;
  }

  if (strcmp(language, "ko") == 0) {
    return PL_KO;
  }

  if (strcmp(language, "pl") == 0) {
    return PL_RU;
  }

  if (strcmp(language, "ru") == 0) {
    return PL_PL;
  }

  if (strcmp(language, "nl") == 0) {
    return PL_NL;
  }

  if (strcmp(language, "ja") == 0) {
    return PL_JA;
  }

  if (strcmp(language, "sv") == 0) {
    return PL_SV;
  }

  return PL_UNKNOWN;
}
static int get_prompt_language(int sock, enum PromptLanguage *language) {
  uint8_t header[CN_BASE_PACK_LEN] = {0};
  int     status                    = read_exact(sock, header, sizeof(header));
  if (status != (int)sizeof(header)) {
    return status ? status : 1;
  }

  if (header[0] != 0x01 || header[1] != 0x03 || header[2] != 0x03) {
    return 1;
  }

  const int payload_len = header[BYTES_POSITION_3];
  if (payload_len <= 0 || payload_len > MAX_BT_PACK_LEN) {
    return 1;
  }

  uint8_t payload[MAX_BT_PACK_LEN] = {0};
  status = read_exact(sock, payload, (size_t)payload_len);
  if (status != payload_len) {
    return status;
  }

  *language = (enum PromptLanguage)payload[0];
  return 0;
}

int set_prompt_language(int sock, enum PromptLanguage language) {
  static uint8_t send[]  = SET_PROMPT_LANGUAGE_SEND;
  send[BYTES_POSITION_4] = language;

  int status = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  enum PromptLanguage got_language = PL_UNKNOWN;
  status                           = get_prompt_language(sock, &got_language);
  if (status) {
    return status;
  }

  return (int)(language - got_language);
}

int set_voice_prompts(int sock, int on) {
  char                 name[MAX_NAME_LEN];
  enum PromptLanguage  pl = PL_UNKNOWN;
  enum AutoOff         ao = AO_UNKNOWN;
  enum NoiseCancelling nc = NC_UNKNOWN;

  int status = get_device_status(sock, name, &pl, &ao, &nc);
  if (status) {
    return status;
  }

  if (on) {
    pl |= VP_MASK;
  } else {
    pl &= ~VP_MASK;
  }

  return set_prompt_language(sock, pl);
}

static int get_auto_off(int sock, enum AutoOff *minutes) {
  static const uint8_t ack[]  = GET_AUTO_OFF_ACK;
  static const uint8_t mask[] = GET_AUTO_OFF_MASK;
  uint8_t              buffer[sizeof(ack)];

  int status = read_check(sock, buffer, sizeof(buffer), ack, mask);
  if (status) {
    return status;
  }

  *minutes = (enum AutoOff)buffer[BYTES_POSITION_4];
  return 0;
}

int set_auto_off(int sock, enum AutoOff minutes) {
  static uint8_t send[]  = SET_AUTO_OFF_SEND;
  send[BYTES_POSITION_4] = minutes;

  int status = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  enum AutoOff got_minutes = AO_UNKNOWN;
  status                   = get_auto_off(sock, &got_minutes);
  if (status) {
    return status;
  }

  return (int)(minutes - got_minutes);
}

static int get_noise_cancelling(int sock, enum NoiseCancelling *level) {
  uint8_t header[CN_BASE_PACK_LEN] = {0};
  int     status                    = read_exact(sock, header, sizeof(header));
  if (status != (int)sizeof(header)) {
    return status ? status : 1;
  }

  if (header[0] != 0x01 || (header[1] != 0x05 && header[1] != 0x06)) {
    return 1;
  }

  const int payload_len = header[BYTES_POSITION_3];
  if (payload_len <= 0 || payload_len > MAX_BT_PACK_LEN) {
    return 1;
  }

  uint8_t payload[MAX_BT_PACK_LEN] = {0};
  status = read_exact(sock, payload, (size_t)payload_len);
  if (status != payload_len) {
    return status ? status : 1;
  }

  if (header[1] == 0x05) {
    if (header[2] != 0x03 || payload_len != 3 || payload[0] != 0x0b) {
      return 1;
    }

    *level = (enum NoiseCancelling)payload[1];
    return 0;
  }

  if (header[2] == 0x03) {
    if (payload_len < 2 || payload[1] != 0x0b) {
      return 1;
    }
    *level = (enum NoiseCancelling)payload[0];
    return 0;
  }

  if (payload_len != 1) {
    return 1;
  }

  *level = (enum NoiseCancelling)payload[0];
  return 0;
}

static int map_noise_cancelling_for_device(unsigned int device_id,
                                           enum NoiseCancelling raw_level,
                                           enum NoiseCancelling *mapped_level) {
  const uint8_t raw_value = (uint8_t)raw_level;

  if (device_id == NOISE_CANCELLING_75) {
    switch (raw_value) {
    case 0x00:
    case 0x01:
    case 0x02:
      *mapped_level = NC_HIGH;
      return 0;
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
      *mapped_level = NC_LOW;
      return 0;
    case 0x08:
    case 0x09:
    case 0x0a:
      *mapped_level = NC_OFF;
      return 0;
    default:
      return 1;
    }
  }

  switch (raw_level) {
  case NC_HIGH:
  case NC_LOW:
  case NC_OFF:
    *mapped_level = raw_level;
    return 0;
  default:
    return 1;
  }
}

static int map_noise_cancelling_to_raw(unsigned int device_id,
                                       enum NoiseCancelling level,
                                       uint8_t *raw_level) {
  if (device_id == NOISE_CANCELLING_75) {
    switch (level) {
    case NC_OFF:
      *raw_level = 0x0a;
      return 0;
    case NC_LOW:
      *raw_level = 0x05;
      return 0;
    case NC_HIGH:
      *raw_level = 0x00;
      return 0;
    default:
      return 1;
    }
  }

  switch (level) {
  case NC_OFF:
  case NC_LOW:
  case NC_HIGH:
    *raw_level = (uint8_t)level;
    return 0;
  default:
    return 1;
  }
}

int set_noise_cancelling(int sock, unsigned int device_id,
                         enum NoiseCancelling level) {
  static uint8_t send[]  = SET_NOISE_CANCELLING_SEND;
  static const uint8_t get_legacy[] = { 0x01, 0x05, 0x01, 0x00 };
  uint8_t        raw_level = 0;
  int status = map_noise_cancelling_to_raw(device_id, level, &raw_level);
  if (status) {
    return status;
  }

  send[BYTES_POSITION_4] = raw_level;

  status = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  enum NoiseCancelling got_raw_level = NC_UNKNOWN;
  if (device_id == NOISE_CANCELLING_75) {
    enum NoiseCancelling ignored_level = NC_UNKNOWN;
    status = get_noise_cancelling(sock, &ignored_level);
    if (status) {
      return status;
    }

    status = transport_write(sock, get_legacy, sizeof(get_legacy));
    if (status != (int)sizeof(get_legacy)) {
      return status ? status : 1;
    }

    status = get_noise_cancelling(sock, &got_raw_level);
    if (status) {
      return status;
    }
  } else {
    status = get_noise_cancelling(sock, &got_raw_level);
    if (status) {
      return status;
    }
  }

  enum NoiseCancelling got_level = NC_UNKNOWN;
  status = map_noise_cancelling_for_device(device_id, got_raw_level,
                                            &got_level);
  if (status) {
    return status;
  }

  return level == got_level ? 0 : 1;
}

int set_noise_mode(int sock, unsigned int mode_index) {
  if (mode_index > 0xff) {
    return 1;
  }

  static uint8_t send[]  = SET_NOISE_MODE_SEND;
  static const uint8_t ack[] = SET_NOISE_MODE_ACK;

  uint8_t mode = (uint8_t)mode_index;

  send[BYTES_POSITION_4] = (uint8_t)mode_index;
  send[6] = (uint8_t)(0xc5u - mode);

  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}

int get_device_status(int sock, char name[MAX_NAME_LEN],
                      enum PromptLanguage *language, enum AutoOff *minutes,
                      enum NoiseCancelling *level) {
  unsigned int device_id = 0;
  unsigned int index     = 0;
  int          status    = get_device_id(sock, &device_id, &index);
  if (status) {
    return status;
  }
  static const uint8_t send[] = GET_DEVICE_STATUS_SEND;
  status                      = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  static const uint8_t ack[] = GET_DEVICE_STATUS_ACK;
  uint8_t              buffer[sizeof(ack)];

  status = read_check(sock, buffer, sizeof(buffer), ack, NULL);
  if (status) {
    return status;
  }

  status = get_name(sock, name);
  if (status) {
    return status;
  }

  status = get_prompt_language(sock, language);
  if (status) {
    return status;
  }

  status = get_auto_off(sock, minutes);
  if (status) {
    return status;
  }

  if (has_noise_cancelling(device_id)) {
    enum NoiseCancelling raw_level = NC_UNKNOWN;
    status                         = get_noise_cancelling(sock, &raw_level);
    if (status) {
      return status;
    }

    status = map_noise_cancelling_for_device(device_id, raw_level, level);
    if (status) {
      return status;
    }
  } else {
    *level = NC_DNE;
  }

  return status;
}

int set_pairing(int sock, enum Pairing pairing) {
  static uint8_t send[]  = SET_PARING_SEND_PACKAGE;
  static uint8_t ack[]   = SET_PARING_ACK_PACKAGE;
  send[BYTES_POSITION_4] = pairing;
  ack[BYTES_POSITION_4]  = pairing;
  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}

int set_self_voice(int sock, enum SelfVoice selfVoice) {
  static uint8_t send[] = SET_SELF_VOICE_SEND_PACKAGE;
  static uint8_t ack[]  = SET_SELF_VOICE_ACK_PACKAGE;

  send[BYTES_POSITION_5] = selfVoice;
  ack[BYTES_POSITION_5]  = selfVoice;
  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}

int get_firmware_version(int sock, char version[VER_STR_LEN]) {
  static const uint8_t send[] = GET_FIRMWARE_VERSION_SEND;
  int status = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  uint8_t header[CN_BASE_PACK_LEN] = {0};
  status = read_exact(sock, header, sizeof(header));
  if (status != (int)sizeof(header)) {
    return status ? status : 1;
  }

  static const uint8_t ack[] = GET_FIRMWARE_VERSION_ACK;
  if (header[0] != ack[0] || header[1] != ack[1] || header[2] != ack[2]) {
    return 1;
  }

  const int payload_len = header[BYTES_POSITION_3];
  if (payload_len <= 0 || payload_len >= VER_STR_LEN) {
    return 1;
  }

  status = read_exact(sock, version, (size_t)payload_len);
  if (status != payload_len) {
    return status ? status : 1;
  }

  version[payload_len] = '\0';
  return 0;
}

int get_serial_number(int sock, char serial[MAX_SERIAL_SIZE]) {
  static const uint8_t send[] = GET_SERIAL_NUMBER_SEND;
  static const uint8_t ack[]  = GET_SERIAL_NUMBER_ACK;

  int status = write_check(sock, send, sizeof(send), ack, sizeof(ack));
  if (status) {
    return status;
  }

  uint8_t length = 0;
  status         = read_exact(sock, &length, 1);
  if (status != 1) {
    return status ? status : 1;
  }

  status = read_exact(sock, serial, length);
  if (status != (int)length) {
    return status ? status : 1;
  }
  serial[length] = '\0';

  return 0;
}

int get_battery_level(int sock, unsigned int *level) {
  static const uint8_t send[] = GET_BATTERY_LEVEL_SEND;
  int status = transport_write(sock, send, sizeof(send));
  if (status != (int)sizeof(send)) {
    return status ? status : 1;
  }

  uint8_t header[CN_BASE_PACK_LEN] = {0};
  status = read_exact(sock, header, sizeof(header));
  if (status != (int)sizeof(header)) {
    return status ? status : 1;
  }

  static const uint8_t ack[] = GET_BATTERY_LEVEL_ACK;
  if (header[0] != ack[0] || header[1] != ack[1] || header[2] != ack[2]) {
    return 1;
  }

  const int payload_len = header[BYTES_POSITION_3];
  if (payload_len <= 0 || payload_len > MAX_BT_PACK_LEN) {
    return 1;
  }

  uint8_t payload[MAX_BT_PACK_LEN] = {0};
  status = read_exact(sock, payload, (size_t)payload_len);
  if (status != payload_len) {
    return status ? status : 1;
  }

  *level = payload[0];

  return 0;
}

int get_device_info(int sock, bdaddr_t address, struct Device *device) {
  static uint8_t       send[BYTES_POSITION_10] = GET_DEVICE_INFO_SEND_PACKAGE;
  static const uint8_t ack[]                   = GET_DEVICE_INFO_ACK_PACKAGE;

  memory_copy(&send[BYTES_POSITION_4], address.b, BT_ADDR_LEN);

  int status = write_check(sock, send, sizeof(send), ack, sizeof(ack));
  if (status) {
    return status;
  }

  uint8_t length = 0;
  status         = read_exact(sock, &length, 1);
  if (status != 1) {
    return status ? status : 1;
  }

  status = read_exact(sock, &device->address.b, BT_ADDR_LEN);
  if (status != BT_ADDR_LEN) {
    return status ? status : 1;
  }
  length -= BT_ADDR_LEN;

  status = memcmp(&address.b, &device->address.b, BT_ADDR_LEN);
  if (status) {
    return abs(status);
  }

  uint8_t status_byte = 0;
  status              = read_exact(sock, &status_byte, 1);
  if (status != 1) {
    return status ? status : 1;
  }
  length -= 1;

  device->status = (enum DeviceStatus)status_byte;

  // TODO(wolf): figure out what the first byte of garbage is for
  uint8_t garbage[BYTES_POSITION_2];
  status = read_exact(sock, &garbage, sizeof(garbage));
  if (status != (int)sizeof(garbage)) {
    return status ? status : 1;
  }
  length -= sizeof(garbage);

  status = read_exact(sock, device->name, length);
  if (status != (int)length) {
    return status ? status : 1;
  }
  device->name[length] = '\0';

  return 0;
}

int get_paired_devices(int sock, bdaddr_t addresses[MAX_NUM_DEVICES],
                       size_t *num_devices, enum DevicesConnected *connected) {
  static const uint8_t send[] = GET_PAIRED_DEVICES_SEND;
  static const uint8_t ack[]  = GET_PAIRED_DEVICES_ACK;

  int status = write_check(sock, send, sizeof(send), ack, sizeof(ack));
  if (status) {
    return status;
  }

  uint8_t num_devices_byte = 0;
  status                   = read_exact(sock, &num_devices_byte, 1);
  if (status != 1) {
    return status ? status : 1;
  }

  num_devices_byte /= BT_ADDR_LEN;
  *num_devices = (size_t)(num_devices_byte - 1);

  uint8_t num_connected_byte = 0;
  status                     = read_exact(sock, &num_connected_byte, 1);
  if (status != 1) {
    return status ? status : 1;
  }
  *connected = (enum DevicesConnected)num_connected_byte;

  for (size_t i = 0; i < num_devices_byte; ++i) {
    status = read_exact(sock, &addresses[i].b, BT_ADDR_LEN);
    if (status != BT_ADDR_LEN) {
      return status ? status : 1;
    }
  }

  return 0;
}

int connect_device(int sock, bdaddr_t address) {
  static uint8_t send[BYTES_POSITION_11] = CONNECT_DEVICE_SEND;
  static uint8_t ack[BYTES_POSITION_10]  = CONNECT_DEVICE_ACK;
  memory_copy(&send[BYTES_POSITION_5], address.b, BT_ADDR_LEN);
  memory_copy(&ack[BYTES_POSITION_4], address.b, BT_ADDR_LEN);
  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}

int disconnect_device(int sock, bdaddr_t address) {
  static uint8_t send[BYTES_POSITION_10] = DISCONNECT_DEVICE_SEND;
  static uint8_t ack[BYTES_POSITION_10]  = DISCONNECT_DEVICE_ACK;
  memory_copy(&send[BYTES_POSITION_4], address.b, BT_ADDR_LEN);
  memory_copy(&ack[BYTES_POSITION_4], address.b, BT_ADDR_LEN);
  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}

int remove_device(int sock, bdaddr_t address) {
  static uint8_t send[BYTES_POSITION_10] = REMOVE_DEVICE_SEND;
  static uint8_t ack[BYTES_POSITION_10]  = REMOVE_DEVICE_ACK;
  memory_copy(&send[BYTES_POSITION_4], address.b, BT_ADDR_LEN);
  memory_copy(&ack[BYTES_POSITION_4], address.b, BT_ADDR_LEN);
  return write_check(sock, send, sizeof(send), ack, sizeof(ack));
}
