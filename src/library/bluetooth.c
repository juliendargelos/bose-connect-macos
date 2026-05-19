#include <stdlib.h>

#include "bluetooth.h"
#include "util.h"

int parse_bdaddr(const char *str, bdaddr_t *ba) {
  const int expected_length = 17;

  if (str == NULL) {
    return -1;
  }

  for (int i = 0; i < BT_ADDR_LEN; ++i) {
    const int octet_start = i * 3;
    uint8_t   octet       = 0;
    if (str_to_byte(&str[octet_start], &octet) != 0) {
      return -1;
    }

    if (ba != NULL) {
      ba->b[i] = octet;
    }

    if (i < BT_ADDR_LEN - 1 && str[octet_start + 2] != ':') {
      return -1;
    }
  }

  if (str[expected_length] != '\0') {
    return -1;
  }

  return 0;
}

void reverse_ba2str(const bdaddr_t *ba, char *str) {
  int size = sizeof(ba->b);

  for (unsigned int position = 0; position < size; position++) {
    unsigned int string_position = position * 3;
    unit_to_hex_string(ba->b[position], &str[string_position]);
    str[string_position + 2] = (char)':';
  }

  str[(size * 3) - 1] = 0;
}

void reverse_str2ba(const char *str, bdaddr_t *ba) {
  if (parse_bdaddr(str, ba) != 0) {
    memory_set(ba, 0, sizeof(*ba));
    return;
  }
}
