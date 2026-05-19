#ifndef BOSE_CONNECT_SRC_LIBRARY_ALIAS_STORE_H
#define BOSE_CONNECT_SRC_LIBRARY_ALIAS_STORE_H

#include <stddef.h>

#define ALIAS_MAX_LEN      64
#define ADDRESS_STRING_LEN 18

struct AliasEntry {
  char alias[ALIAS_MAX_LEN];
  char address[ADDRESS_STRING_LEN];
};

int alias_store_add(const char *alias, const char *address);

int alias_store_remove(const char *alias);

int alias_store_resolve(const char *alias, char address[ADDRESS_STRING_LEN]);

int alias_store_read_all(struct AliasEntry *entries, size_t max_entries,
                         size_t *num_entries);

#endif
