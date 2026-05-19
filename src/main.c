#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "library/based.h"
#include "library/alias_store.h"
#include "library/transport.h"
#include "library/util.h"
#include "main.h"

#define OPTION_DEVICE_ID         5
#define OPTION_CONNECT_DEVICE    2
#define OPTION_DISCONNECT_DEVICE 3
#define OPTION_REMOVE_DEVICE     4
#define OPTION_ADD_ALIAS         6
#define OPTION_REMOVE_ALIAS      7
#define OPTION_ALIAS             8
#define OPTION_LIST_DEVICES      9
#define OPTION_SEND_PACKET       1

static int resolve_target_address(const char *address_argument,
                                  const char *alias_argument,
                                  char out_address[ADDRESS_STRING_LEN]);

static void usage() {
  const char *message =
      "Usage: %s [options] [address]\n"

      "\t-h, --help\n"
      "\t\tPrint the help message.\n"

      "\t-i, --info\n"
      "\t\tPrint all the device information.\n"

      "\t-d, --device-status\n"
      "\t\tPrint the device status information. This includes its name,"
      " language,\n"
      "\t\tvoice-prompts, auto-off and noise cancelling settings.\n"

      "\t-f, --firmware-version\n"
      "\t\tPrint the firmware version on the device.\n"

      "\t-s, --serial-number\n"
      "\t\tPrint the serial number of the device.\n"

      "\t-b, --battery-level\n"
      "\t\tPrint the battery level of the device as a percent.\n"

      "\t-a, --paired-devices\n"
      "\t\tPrint the devices currently connected to the device.\n"
      "\t\t!: indicates the current device\n"
      "\t\t*: indicates other connected devices\n"

      "\t--device-id\n"
      "\t\tPrint the device id followed by the index revision.\n"

      "\t-n <name>, --name=<name>\n"
      "\t\tChange the name of the device.\n"

      "\t-o <minutes>, --auto-off=<minutes>\n"
      "\t\tChange the auto-off time.\n"
      "\t\tminutes: never, 5, 20, 40, 60, 180\n"

      "\t-c <level>, --noise-cancelling=<level>\n"
      "\t\tChange the noise cancelling level.\n"
      "\t\tlevel: high, low, off\n"

      "\t-l <language>, --prompt-language=<language>\n"
      "\t\tChange the voice-prompt language.\n"
      "\t\tlanguage: en, fr, it, de, es, pt, zh, ko, nl, ja, sv\n"

      "\t-v <switch>, --voice-prompts=<switch>\n"
      "\t\tChange whether voice-prompts are on or off.\n"
      "\t\tswitch: on, off\n"

      "\t-p <status>, --pairing=<status>\n"
      "\t\tChange whether the device is pairing.\n"
      "\t\tstatus: on, off\n"

      "\t-e, --self-voice=<level>\n"
      "\t\tChange the self voice level.\n"
      "\t\tlevel: high, medium, low, off\n"

      "\t--connect-device=<address>\n"
      "\t\tAttempt to connect to the device at address.\n"

      "\t--disconnect-device=<address>\n"
      "\t\tDisconnect the device at address.\n"

      "\t--remove-device=<address>\n"
      "\t\tRemove the device at address from the pairing list.\n"

      "\t--alias=<name>\n"
      "\t\tUse a saved alias instead of a Bluetooth address argument.\n"

      "\t--add-alias=<name> <address>\n"
      "\t\tSave or update alias for a Bluetooth address.\n"

      "\t--remove-alias=<name>\n"
      "\t\tRemove a saved alias.\n"

      "\t--list-devices\n"
      "\t\tList paired Bluetooth devices and saved aliases.\n";

  printf(message, PROGRAM_NAME);
}

static int do_add_alias(const char *alias, const char *address) {
  if (alias_store_add(alias, address) != 0) {
    perror("Could not add alias");
    return 1;
  }

  printf("Saved alias '%s' -> %s\n", alias, address);
  return 0;
}

static int resolve_alias_address_for_add(const char *address_argument,
                                         const char *alias_argument,
                                         char out_address[ADDRESS_STRING_LEN]) {
  if (address_argument != NULL) {
    return resolve_target_address(address_argument, NULL, out_address);
  }

  if (alias_argument == NULL) {
    fprintf(stderr,
            "Address argument is required for --add-alias unless --alias is "
            "provided.\n");
    return 1;
  }

  if (alias_store_resolve(alias_argument, out_address) != 0) {
    perror("Could not resolve alias for --add-alias");
    return 1;
  }

  return 0;
}

static int do_remove_alias(const char *alias) {
  if (alias_store_remove(alias) != 0) {
    perror("Could not remove alias");
    return 1;
  }

  printf("Removed alias '%s'\n", alias);
  return 0;
}

static int print_saved_aliases(void) {
  struct AliasEntry aliases[256];
  size_t            num_aliases = 0;

  if (alias_store_read_all(aliases, 256, &num_aliases) != 0) {
    perror("Could not read aliases");
    return 1;
  }

  printf("Saved aliases: %zu\n", num_aliases);
  for (size_t i = 0; i < num_aliases; ++i) {
    printf("\t%s -> %s\n", aliases[i].alias, aliases[i].address);
  }

  return 0;
}

static int do_list_devices(void) {
  struct TransportDevice devices[256];
  size_t                 num_devices = 0;

  if (transport_list_paired_devices(devices, 256, &num_devices) != 0) {
    perror("Could not list Bluetooth devices");
    return 1;
  }

  printf("Paired Bluetooth devices: %zu\n", num_devices);
  for (size_t i = 0; i < num_devices && i < 256; ++i) {
    printf("\t%s | %s | %s\n", devices[i].connected ? "*" : " ",
           devices[i].address[0] ? devices[i].address : "(unknown)",
           devices[i].name[0] ? devices[i].name : "(unnamed)");
  }
  printf("\t[*] Indicates currently connected.\n");

  return print_saved_aliases();
}

static int resolve_target_address(const char *address_argument,
                                  const char *alias_argument,
                                  char out_address[ADDRESS_STRING_LEN]) {
  if (alias_argument != NULL) {
    if (alias_store_resolve(alias_argument, out_address) != 0) {
      perror("Could not resolve alias");
      return 1;
    }
    return 0;
  }

  if (address_argument == NULL) {
    fprintf(stderr, "A Bluetooth address argument is required.\n");
    return 1;
  }

  if (strlen(address_argument) >= ADDRESS_STRING_LEN) {
    fprintf(stderr, "Invalid Bluetooth address: %s\n", address_argument);
    return 1;
  }

  strncpy(out_address, address_argument, ADDRESS_STRING_LEN - 1);
  out_address[ADDRESS_STRING_LEN - 1] = '\0';

  if (parse_bdaddr(out_address, NULL) != 0) {
    fprintf(stderr, "Invalid Bluetooth address: %s\n", out_address);
    return 1;
  }

  return 0;
}

int do_get_information(char *address) {
  enum { SECS_TO_SLEEP = 6, NANO_TO_SLEEP = 0 };
  struct timespec remaining;
  struct timespec request = {SECS_TO_SLEEP, NANO_TO_SLEEP};

  while (do_get_device_id(address)) {
    nanosleep(&request, &remaining);
  }
  nanosleep(&request, &remaining);

  while (do_get_serial_number(address)) {
    nanosleep(&request, &remaining);
  }
  nanosleep(&request, &remaining);

  while (do_get_firmware_version(address)) {
    nanosleep(&request, &remaining);
  }
  nanosleep(&request, &remaining);

  while (do_get_battery_level(address)) {
    nanosleep(&request, &remaining);
  }
  nanosleep(&request, &remaining);

  while (do_get_device_status(address)) {
    nanosleep(&request, &remaining);
  }
  nanosleep(&request, &remaining);

  while (do_get_paired_devices(address)) {
    nanosleep(&request, &remaining);
  }

  return 0;
}

static int do_set_name(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  int    status      = 1;
  size_t name_length = strlen(arg);
  if (name_length >= MAX_NAME_LEN) {
    fprintf(stderr, "Name exceeds %d character maximum. Actual size is %zu.\n",
            MAX_NAME_LEN - 1, name_length);
  } else {
    char name_buffer[MAX_NAME_LEN] = {0};
    str_copy(name_buffer, arg, MAX_NAME_LEN);
    status = set_name(sock, name_buffer);
  }

  transport_close(sock);
  return status;
}

static int do_set_prompt_language(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  enum PromptLanguage pl = get_language(arg);

  if (pl == PL_UNKNOWN) {
    fprintf(stderr, "Invalid prompt language argument: %s\n", arg);
    usage();
    transport_close(sock);
    return 1;
  }

  const int status = set_prompt_language(sock, pl);
  transport_close(sock);
  return status;
}

int get_voice_status(const char *arg) {
  if (strcmp(arg, "on") == 0) {
    return 1;
  }

  if (strcmp(arg, "off") == 0) {
    return 0;
  }

  return -1;
}

static int do_set_voice_prompts(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  int voice_status = get_voice_status(arg);

  if (voice_status == -1) {
    fprintf(stderr, "Invalid voice prompt argument: %s\n", arg);
    usage();
    transport_close(sock);
    return 1;
  }

  const int status = set_voice_prompts(sock, voice_status);
  transport_close(sock);
  return status;
}

static int do_set_auto_off(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  char *end_pointer = NULL;
  errno             = 0;
  long parsed       = strtol(arg, &end_pointer, MAX_DECIMAL_UNIT);

  if (errno != 0) {
    perror("Error trying to set auto off.\n");
    transport_close(sock);
    return 1;
  }

  if (end_pointer == arg) {
    fprintf(stderr, "No digits were found.\n");
    transport_close(sock);
    return 1;
  }

  enum AutoOff ao = AO_NEVER;
  switch (parsed) {
  case AO_5_MIN:
  case AO_20_MIN:
  case AO_40_MIN:
  case AO_60_MIN:
  case AO_180_MIN:
    ao = (enum AutoOff)parsed;
    break;
  default:
    if (strcmp(arg, "never") != 0) {
      fprintf(stderr, "Invalid auto-off argument: %s\n", arg);
      usage();
      transport_close(sock);
      return 1;
    }
  }

  const int status = set_auto_off(sock, ao);
  transport_close(sock);
  return status;
}

enum NoiseCancelling get_noise_cancelling(const char *arg) {
  if (strcmp(arg, "high") == 0) {
    return NC_HIGH;
  }

  if (strcmp(arg, "low") == 0) {
    return NC_LOW;
  }

  if (strcmp(arg, "off") == 0) {
    return NC_OFF;
  }

  return NC_UNKNOWN;
}

static int do_set_noise_cancelling(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  enum NoiseCancelling nc = get_noise_cancelling(arg);

  if (nc == NC_UNKNOWN) {
    fprintf(stderr, "Invalid noise cancelling argument: %s\n", arg);
    usage();
    transport_close(sock);
    return 1;
  }

  unsigned int device_id = 0;
  unsigned int index     = 0;
  int          status    = get_device_id(sock, &device_id, &index);
  if (status) {
    transport_close(sock);
    return status;
  }

  if (!has_noise_cancelling(device_id)) {
    fprintf(stderr, "This device does not have noise cancelling.\n");
    usage();
    transport_close(sock);
    return 1;
  }

  status = set_noise_cancelling(sock, nc);
  transport_close(sock);
  return status;
}

char *get_language_string(enum PromptLanguage language) {
  if (language == PL_EN) {
    return "EN";
  }

  if (language == PL_FR) {
    return "FR";
  }

  if (language == PL_IT) {
    return "IT";
  }

  if (language == PL_DE) {
    return "DE";
  }

  if (language == PL_ES) {
    return "ES";
  }

  if (language == PL_PT) {
    return "PT";
  }

  if (language == PL_ZH) {
    return "ZH";
  }

  if (language == PL_KO) {
    return "KO";
  }

  if (language == PL_NL) {
    return "NL";
  }

  if (language == PL_JA) {
    return "JA";
  }

  if (language == PL_SV) {
    return "SV";
  }

  if (language == PL_RU) {
    return "RU";
  }

  if (language == PL_PL) {
    return "PL";
  }

  return "";
}

static int do_get_device_status(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  printf("Status:\n");
  char                 name[MAX_NAME_LEN];
  enum PromptLanguage  promptLanguage  = PL_UNKNOWN;
  enum AutoOff         autoOff         = AO_UNKNOWN;
  enum NoiseCancelling noiseCancelling = NC_UNKNOWN;

  int status = get_device_status(sock, name, &promptLanguage, &autoOff,
                                 &noiseCancelling);
  if (status) {
    transport_close(sock);
    return status;
  }

  printf("\tName: %s\n", name);

  char  unknown_language[] = "Unknown [0x00]";
  char *language           = get_language_string((promptLanguage & VP_MASK));

  if (strcmp("", language) == 0) {
    char      language_value[4]  = "";
    const int position_hex_value = 11;

    unit_to_hex_string(promptLanguage, &language_value[0]);

    unknown_language[position_hex_value]     = language_value[0];
    unknown_language[position_hex_value + 1] = language_value[1];
    language                                 = unknown_language;
  }

  printf("\tLanguage: %s\n", language);
  printf("\tVoice Prompts: %s\n", (promptLanguage & VP_MASK) ? "on" : "off");

  printf("\tAuto-Off: ");
  if (autoOff) {
    printf("%d", autoOff);
  } else {
    printf("never");
  }
  printf("\n");

  char *cancellingLevel = NULL;
  if (noiseCancelling != NC_DNE) {
    switch (noiseCancelling) {
    case NC_HIGH:
      cancellingLevel = "high";
      break;
    case NC_LOW:
      cancellingLevel = "low";
      break;
    case NC_OFF:
      cancellingLevel = "off";
      break;
    default:
      transport_close(sock);
      return 1;
    }
    printf("\tNoise Cancelling: %s\n", cancellingLevel);
  }

  transport_close(sock);
  return 0;
}

enum Pairing get_paring_status(const char *arg) {
  if (strcmp(arg, "on") == 0) {
    return P_ON;
  }

  if (strcmp(arg, "off") == 0) {
    return P_OFF;
  }

  return P_UNKNOWN;
}

static int do_set_pairing(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  enum Pairing p = get_paring_status(arg);

  if (p == P_UNKNOWN) {
    fprintf(stderr, "Invalid pairing argument: %s\n", arg);
    usage();
    transport_close(sock);
    return 1;
  }

  const int status = set_pairing(sock, p);
  transport_close(sock);
  return status;
}

enum SelfVoice get_self_voice_status(const char *arg) {
  if (strcmp(arg, "high") == 0) {
    return SV_HIGH;
  }

  if (strcmp(arg, "medium") == 0) {
    return SV_MEDIUM;
  }

  if (strcmp(arg, "low") == 0) {
    return SV_LOW;
  }

  if (strcmp(arg, "off") == 0) {
    return SV_OFF;
  }

  return SV_UNKNOWN;
}

static int do_set_self_voice(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  enum SelfVoice p = get_self_voice_status(arg);

  if (p == SV_UNKNOWN) {
    fprintf(stderr, "Invalid self voice argument: %s\n", arg);
    usage();
    transport_close(sock);
    return 1;
  }

  const int status = set_self_voice(sock, p);
  transport_close(sock);
  return status;
}

static int do_get_firmware_version(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  printf("Firmware version: ");
  char version[VER_STR_LEN];
  int  status = get_firmware_version(sock, version);

  if (status) {
    transport_close(sock);
    return status;
  }

  printf("%s\n", version);

  transport_close(sock);
  return 0;
}

static int do_get_serial_number(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  printf("Serial number: ");
  char serial[MAX_SERIAL_SIZE];
  int  status = get_serial_number(sock, serial);

  if (status) {
    transport_close(sock);
    return status;
  }

  printf("%s\n", serial);

  transport_close(sock);
  return 0;
}

static int do_get_battery_level(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  printf("Battery level: ");
  unsigned int level  = 0;
  int          status = get_battery_level(sock, &level);

  if (status) {
    transport_close(sock);
    return status;
  }

  printf("%u\n", level);

  transport_close(sock);
  return 0;
}

int get_paired_devices_connected(enum DevicesConnected connected) {
  if (connected == DC_ONE) {
    return 1;
  }
  if (connected == DC_TWO) {
    return 2;
  }

  return -1;
}

char get_paired_device_status(enum DeviceStatus status) {
  if (status == DS_THIS) {
    return '!';
  }

  if (status == DS_CONNECTED) {
    return '*';
  }

  if (status == DS_DISCONNECTED) {
    return ' ';
  }

  return ':';
}
static int do_get_paired_devices(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  printf("Paired devices: ");
  bdaddr_t              devices[MAX_NUM_DEVICES];
  size_t                num_devices = 0;
  enum DevicesConnected connected   = DC_UNKNOWN;

  int status = get_paired_devices(sock, devices, &num_devices, &connected);
  if (status) {
    transport_close(sock);
    return status;
  }

  if (connected == DC_UNKNOWN) {
    printf("\n\t"
           "Error: 0x%02X connected devices. Outside of the range "
           "(0x01 and 0x03)."
           "\n",
           connected);
    transport_close(sock);
    return 1;
  }

  int num_connected = get_paired_devices_connected(connected);

  printf("%zu\n", num_devices);
  printf("\tConnected: %d\n", num_connected);

  for (size_t i = 0; i < num_devices; ++i) {
    struct Device device;
    status = get_device_info(sock, devices[i], &device);
    if (status) {
      transport_close(sock);
      return status;
    }

    char address_converted[18];
    reverse_ba2str(&device.address, address_converted);

    char status_symbol = get_paired_device_status(device.status);

    if (status_symbol == ':') {
      transport_close(sock);
      return 1;
    }

    printf("\tDevice: %c | %s | %s\n", status_symbol, address_converted,
           device.name);
  }

  printf("\t[!] Indicates the current device.\n");
  printf("\t[*] Indicates other connected devices.\n");

  transport_close(sock);
  return 0;
}

static int do_connect_device(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }

  bdaddr_t bd_address;
  reverse_str2ba(arg, &bd_address);
  int connection = connect_device(sock, bd_address);

  transport_close(sock);
  return connection;
}

static int do_disconnect_device(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  bdaddr_t bd_address;
  reverse_str2ba(arg, &bd_address);
  int disconnection = disconnect_device(sock, bd_address);

  transport_close(sock);
  return disconnection;
}

static int do_remove_device(char *address, const char *arg) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  bdaddr_t bd_address;
  reverse_str2ba(arg, &bd_address);
  int removed = remove_device(sock, bd_address);

  transport_close(sock);
  return removed;
}

static int do_get_device_id(char *address) {
  int sock = get_socket(address);
  if (sock == -1) {
    return 1;
  }
  printf("Device ID: ");
  unsigned int device_id = 0;
  unsigned int index     = 0;
  int          status    = get_device_id(sock, &device_id, &index);

  if (status) {
    transport_close(sock);
    return status;
  }

  printf("0x%04x | Index: %u\n", device_id, index);

  transport_close(sock);
  return 0;
}

static int do_send_packet(char *address, const char *arg) {
  int char_type_pointer_size = sizeof(char *);
  int sock                   = get_socket(address);

  if (sock == -1) {
    return 1;
  }

  uint8_t send[char_type_pointer_size / 2];
  for (size_t i = 0; arg[i * 2]; ++i) {
    if (str_to_byte(&arg[i * 2], &send[i]) != 0) {
      transport_close(sock);
      return 1;
    }
  }

  uint8_t received[MAX_BT_PACK_LEN];
  int     received_n = send_packet(sock, send, sizeof(send), received);
  if (received_n < 0) {
    transport_close(sock);
    return received_n;
  }

  printf("Received package:\n\t");
  for (size_t i = 0; i < received_n; ++i) {
    printf("%02x ", received[i]);
  }
  printf("\n");

  transport_close(sock);
  return 0;
}

int get_socket(char *address) {
  if (parse_bdaddr(address, NULL) != 0) {
    fprintf(stderr, "Invalid bluetooth address: %s\n", address);
    return -1;
  }

  int sock = transport_open(address, BOSE_CHANNEL);
  if (sock < 0) {
    perror("Could not connect to Bluetooth device");
    return -1;
  }

  int connection = init_connection(sock);
  if (connection) {
    transport_close(sock);
    return -1;
  }

  return sock;
}

int main(int argc, char *argv[]) {
  static const char *        short_opt  = "hidfsban:n:o:c:l:v:p:e:";
  static const struct option long_opt[] = {
      {"help", no_argument, NULL, 'h'},
      {"info", no_argument, NULL, 'i'},
      {"device-status", no_argument, NULL, 'd'},
      {"firmware-version", no_argument, NULL, 'f'},
      {"serial-number", no_argument, NULL, 's'},
      {"battery-level", no_argument, NULL, 'b'},
      {"paired-devices", no_argument, NULL, 'a'},
      {"device-id", no_argument, NULL, 5},
      {"name", required_argument, NULL, 'n'},
      {"auto-off", required_argument, NULL, 'o'},
      {"noise-cancelling", required_argument, NULL, 'c'},
      {"prompt-language", required_argument, NULL, 'l'},
      {"voice-prompts", required_argument, NULL, 'v'},
      {"pairing", required_argument, NULL, 'p'},
      {"self-voice", required_argument, NULL, 'e'},
      {"connect-device", required_argument, NULL, 2},
      {"disconnect-device", required_argument, NULL, 3},
      {"remove-device", required_argument, NULL, 4},
      {"add-alias", required_argument, NULL, OPTION_ADD_ALIAS},
      {"remove-alias", required_argument, NULL, OPTION_REMOVE_ALIAS},
      {"alias", required_argument, NULL, OPTION_ALIAS},
      {"list-devices", no_argument, NULL, OPTION_LIST_DEVICES},
      {"send-packet", required_argument, NULL, 1},
      {0, no_argument, NULL, 0}};

  int   status        = 0;
  int   command_opt   = 0;
  char *command_arg   = NULL;
  char *alias_target  = NULL;
  char *address_arg   = NULL;
  char  address[ADDRESS_STRING_LEN] = {0};

  int opt_index = 0;
  int opt       = 0;
  optind        = 1;
  while ((opt = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) > 0) {
    switch (opt) {
    case 'h':
      usage();
      return 0;
    case OPTION_ALIAS:
      alias_target = optarg;
      break;
    default:
      if (command_opt == 0) {
        command_opt = opt;
        command_arg = optarg;
      } else {
        fprintf(stderr, "Only one command option may be given.\n");
        usage();
        return 1;
      }
      break;
    }
  }

  if (optind < argc) {
    address_arg = argv[optind];
    if (optind + 1 < argc) {
      fprintf(stderr, "Only one address argument may be given.\n");
      usage();
      return 1;
    }
  }

  if (command_opt == 0) {
    fprintf(stderr, "A command option must be given.\n");
    usage();
    return 1;
  }

  if (command_opt == OPTION_LIST_DEVICES) {
    if (address_arg != NULL) {
      fprintf(stderr, "--list-devices does not take an address argument.\n");
      return 1;
    }
    return do_list_devices();
  }

  if (command_opt == OPTION_REMOVE_ALIAS) {
    if (command_arg == NULL) {
      fprintf(stderr, "Alias is required for --remove-alias.\n");
      return 1;
    }
    if (address_arg != NULL) {
      fprintf(stderr, "--remove-alias does not take an address argument.\n");
      return 1;
    }
    return do_remove_alias(command_arg);
  }

  if (command_opt == OPTION_ADD_ALIAS) {
    char alias_address[ADDRESS_STRING_LEN] = {0};
    if (command_arg == NULL) {
      fprintf(stderr, "Alias is required for --add-alias.\n");
      return 1;
    }

    if (resolve_alias_address_for_add(address_arg, alias_target,
                                      alias_address) != 0) {
      return 1;
    }

    return do_add_alias(command_arg, alias_address);
  }

  if (resolve_target_address(address_arg, alias_target, address) != 0) {
    return 1;
  }

  switch (command_opt) {
  case 'i':
    status = do_get_information(address);
    break;
  case 'd':
    status = do_get_device_status(address);
    break;
  case 'f':
    status = do_get_firmware_version(address);
    break;
  case 's':
    status = do_get_serial_number(address);
    break;
  case 'b':
    status = do_get_battery_level(address);
    break;
  case 'a':
    status = do_get_paired_devices(address);
    break;
  case OPTION_DEVICE_ID:
    status = do_get_device_id(address);
    break;
  case 'n':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --name.\n");
      return 1;
    }
    status = do_set_name(address, command_arg);
    break;
  case 'o':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --auto-off.\n");
      return 1;
    }
    status = do_set_auto_off(address, command_arg);
    break;
  case 'c':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --noise-cancelling.\n");
      return 1;
    }
    status = do_set_noise_cancelling(address, command_arg);
    break;
  case 'l':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --prompt-language.\n");
      return 1;
    }
    status = do_set_prompt_language(address, command_arg);
    break;
  case 'v':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --voice-prompts.\n");
      return 1;
    }
    status = do_set_voice_prompts(address, command_arg);
    break;
  case 'p':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --pairing.\n");
      return 1;
    }
    status = do_set_pairing(address, command_arg);
    break;
  case OPTION_CONNECT_DEVICE:
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --connect-device.\n");
      return 1;
    }
    status = do_connect_device(address, command_arg);
    break;
  case OPTION_DISCONNECT_DEVICE:
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --disconnect-device.\n");
      return 1;
    }
    status = do_disconnect_device(address, command_arg);
    break;
  case OPTION_REMOVE_DEVICE:
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --remove-device.\n");
      return 1;
    }
    status = do_remove_device(address, command_arg);
    break;
  case 'e':
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --self-voice.\n");
      return 1;
    }
    status = do_set_self_voice(address, command_arg);
    break;
  case OPTION_SEND_PACKET:
    if (command_arg == NULL) {
      fprintf(stderr, "Missing required argument for --send-packet.\n");
      return 1;
    }
    status = do_send_packet(address, command_arg);
    break;
  default:
    status = 1;
  }

  if (status < 0) {
    perror("Error trying to change setting");
  }

  return status;
}
