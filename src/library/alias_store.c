#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "alias_store.h"
#include "bluetooth.h"

#define CONFIG_FILE_NAME ".bose-connect-devices"
#define MAX_ALIAS_ENTRIES 256

static int is_alias_valid(const char *alias) {
  if (alias == NULL || alias[0] == '\0') {
    return 0;
  }

  size_t length = 0;
  while (alias[length] != '\0') {
    unsigned char character = (unsigned char)alias[length];
    if (!(isalnum(character) || character == '_' || character == '-')) {
      return 0;
    }

    length += 1;
    if (length >= ALIAS_MAX_LEN) {
      return 0;
    }
  }

  return 1;
}

static int is_address_valid(const char *address) {
  return parse_bdaddr(address, NULL) == 0;
}

static int get_config_path(char path[PATH_MAX]) {
  const char *home = getenv("HOME");
  if (home == NULL || home[0] == '\0') {
    errno = ENOENT;
    return -1;
  }

  int written = snprintf(path, PATH_MAX, "%s/%s", home, CONFIG_FILE_NAME);
  if (written <= 0 || written >= PATH_MAX) {
    errno = ENAMETOOLONG;
    return -1;
  }

  return 0;
}

static void trim_line(char *line) {
  if (line == NULL) {
    return;
  }

  size_t length = strlen(line);
  while (length > 0) {
    char last = line[length - 1];
    if (last == '\n' || last == '\r' || last == ' ' || last == '\t') {
      line[length - 1] = '\0';
      length -= 1;
    } else {
      break;
    }
  }
}

static int parse_entry_line(const char *line, struct AliasEntry *entry) {
  const char *separator = strchr(line, '=');
  if (separator == NULL || separator == line || separator[1] == '\0') {
    return -1;
  }

  size_t alias_len = (size_t)(separator - line);
  size_t addr_len  = strlen(separator + 1);
  if (alias_len == 0 || alias_len >= ALIAS_MAX_LEN ||
      addr_len == 0 || addr_len >= ADDRESS_STRING_LEN) {
    return -1;
  }

  char alias[ALIAS_MAX_LEN]          = {0};
  char address[ADDRESS_STRING_LEN]   = {0};
  memcpy(alias, line, alias_len);
  memcpy(address, separator + 1, addr_len);

  if (!is_alias_valid(alias) || !is_address_valid(address)) {
    return -1;
  }

  if (entry != NULL) {
    memcpy(entry->alias, alias, ALIAS_MAX_LEN);
    memcpy(entry->address, address, ADDRESS_STRING_LEN);
  }

  return 0;
}

static int read_entries(struct AliasEntry entries[MAX_ALIAS_ENTRIES],
                        size_t *num_entries) {
  if (num_entries == NULL) {
    errno = EINVAL;
    return -1;
  }

  *num_entries = 0;

  char path[PATH_MAX] = {0};
  if (get_config_path(path) != 0) {
    return -1;
  }

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    if (errno == ENOENT) {
      return 0;
    }
    return -1;
  }

  char  *line      = NULL;
  size_t line_size = 0;
  while (getline(&line, &line_size, file) >= 0) {
    trim_line(line);
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    struct AliasEntry entry;
    if (parse_entry_line(line, &entry) != 0) {
      continue;
    }

    if (*num_entries >= MAX_ALIAS_ENTRIES) {
      free(line);
      fclose(file);
      errno = ENOSPC;
      return -1;
    }

    entries[*num_entries] = entry;
    *num_entries += 1;
  }

  free(line);
  fclose(file);
  return 0;
}

static int write_entries(const struct AliasEntry *entries, size_t num_entries) {
  char path[PATH_MAX] = {0};
  if (get_config_path(path) != 0) {
    return -1;
  }

  char tmp_path[PATH_MAX] = {0};
  int  written = snprintf(tmp_path, PATH_MAX, "%s.tmp", path);
  if (written <= 0 || written >= PATH_MAX) {
    errno = ENAMETOOLONG;
    return -1;
  }

  FILE *file = fopen(tmp_path, "w");
  if (file == NULL) {
    return -1;
  }

  for (size_t i = 0; i < num_entries; ++i) {
    if (fprintf(file, "%s=%s\n", entries[i].alias, entries[i].address) < 0) {
      fclose(file);
      unlink(tmp_path);
      return -1;
    }
  }

  if (fclose(file) != 0) {
    unlink(tmp_path);
    return -1;
  }

  if (rename(tmp_path, path) != 0) {
    unlink(tmp_path);
    return -1;
  }

  return 0;
}

int alias_store_add(const char *alias, const char *address) {
  if (!is_alias_valid(alias) || !is_address_valid(address)) {
    errno = EINVAL;
    return -1;
  }

  struct AliasEntry entries[MAX_ALIAS_ENTRIES];
  size_t            num_entries = 0;
  if (read_entries(entries, &num_entries) != 0) {
    return -1;
  }

  size_t existing_index = num_entries;
  for (size_t i = 0; i < num_entries; ++i) {
    if (strcmp(entries[i].alias, alias) == 0) {
      existing_index = i;
      break;
    }
  }

  if (existing_index < num_entries) {
    strncpy(entries[existing_index].address, address, ADDRESS_STRING_LEN - 1);
    entries[existing_index].address[ADDRESS_STRING_LEN - 1] = '\0';
  } else {
    if (num_entries >= MAX_ALIAS_ENTRIES) {
      errno = ENOSPC;
      return -1;
    }

    strncpy(entries[num_entries].alias, alias, ALIAS_MAX_LEN - 1);
    entries[num_entries].alias[ALIAS_MAX_LEN - 1] = '\0';
    strncpy(entries[num_entries].address, address, ADDRESS_STRING_LEN - 1);
    entries[num_entries].address[ADDRESS_STRING_LEN - 1] = '\0';
    num_entries += 1;
  }

  return write_entries(entries, num_entries);
}

int alias_store_remove(const char *alias) {
  if (!is_alias_valid(alias)) {
    errno = EINVAL;
    return -1;
  }

  struct AliasEntry entries[MAX_ALIAS_ENTRIES];
  size_t            num_entries = 0;
  if (read_entries(entries, &num_entries) != 0) {
    return -1;
  }

  size_t found_index = num_entries;
  for (size_t i = 0; i < num_entries; ++i) {
    if (strcmp(entries[i].alias, alias) == 0) {
      found_index = i;
      break;
    }
  }

  if (found_index == num_entries) {
    errno = ENOENT;
    return -1;
  }

  for (size_t i = found_index; i + 1 < num_entries; ++i) {
    entries[i] = entries[i + 1];
  }
  num_entries -= 1;

  return write_entries(entries, num_entries);
}

int alias_store_resolve(const char *alias, char address[ADDRESS_STRING_LEN]) {
  if (!is_alias_valid(alias) || address == NULL) {
    errno = EINVAL;
    return -1;
  }

  struct AliasEntry entries[MAX_ALIAS_ENTRIES];
  size_t            num_entries = 0;
  if (read_entries(entries, &num_entries) != 0) {
    return -1;
  }

  for (size_t i = 0; i < num_entries; ++i) {
    if (strcmp(entries[i].alias, alias) == 0) {
      strncpy(address, entries[i].address, ADDRESS_STRING_LEN - 1);
      address[ADDRESS_STRING_LEN - 1] = '\0';
      return 0;
    }
  }

  errno = ENOENT;
  return -1;
}

int alias_store_read_all(struct AliasEntry *entries, size_t max_entries,
                         size_t *num_entries) {
  if (num_entries == NULL) {
    errno = EINVAL;
    return -1;
  }

  struct AliasEntry all_entries[MAX_ALIAS_ENTRIES];
  size_t            all_count = 0;
  if (read_entries(all_entries, &all_count) != 0) {
    return -1;
  }

  if (entries != NULL && max_entries > 0) {
    size_t copy_count = all_count < max_entries ? all_count : max_entries;
    for (size_t i = 0; i < copy_count; ++i) {
      entries[i] = all_entries[i];
    }
  }

  *num_entries = all_count;
  return 0;
}
