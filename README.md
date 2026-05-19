# Bose Connect macOS

--- Not Official App ---

This project is based on
[`airvzxf/bose-connect-app-linux`](https://github.com/airvzxf/bose-connect-app-linux.git),
with a maintained macOS-native focus.

The original reverse-engineering lineage comes from [Denton-L project][Denton-L].
This project keeps the original GPL-3.0 license.

This program attempts to reverse engineer the `Bose Connect` app behavior for
desktop usage on macOS.

### Usage

```text
Usage: bose-connect [options] [address]
  # address: Optional when --alias=<name> is used.

  -h, --help
    Print the help message.
  -i, --info
    Print all the device information.
  -d, --device-status
    Print the device status information. This includes its name, language,
    voice-prompts, auto-off and noise cancelling settings.
  -f, --firmware-version
    Print the firmware version on the device.
  -s, --serial-number
    Print the serial number of the device.
  -b, --battery-level
    Print the battery level of the device as a percent.
  -a, --paired-devices
    Print the devices currently connected to the device.
    !: indicates the current device
    *: indicates other connected devices
  --device-id
    Print the device id followed by the index revision.
  -n <name>, --name=<name>
    Change the name of the device.
  -o <minutes>, --auto-off=<minutes>
    Change the auto-off time.
    minutes: never, 5, 20, 40, 60, 180
  -c <level>, --noise-cancelling=<level>
    Change the noise cancelling level.
    level: high, low, off
  -m <mode>, --mode=<mode>
    Change the noise cancelling mode.
    mode: quiet, aware, custom-1, custom-2
  -l <language>, --prompt-language=<language>
    Change the voice-prompt language.
    language: en, fr, it, de, es, pt, zh, ko, nl, ja, sv
  -v <switch>, --voice-prompts=<switch>
    Change whether voice-prompts are on or off.
    switch: on, off
  -p <status>, --pairing=<status>
    Change whether the device is pairing.
    status: on, off
  -e, --self-voice=<level>
    Change the self voice level.
    level: high, medium, low, off
  --connect-device=<address>
    Attempt to connect to the device at address.
  --disconnect-device=<address>
    Disconnect the device at address.
  --remove-device=<address>
    Remove the device at address from the pairing list.
  --alias=<name>
    Use a saved alias instead of the Bluetooth address argument.
  --add-alias=<name> <address>
    Save or update an alias for a Bluetooth address.
  --remove-alias=<name>
    Remove a saved alias.
  --list-devices
    List paired Bluetooth devices and saved aliases.
```

## Build and Installation

The executable produced by the build will be
`./src/build/bose-connect` and the installation will be
`/usr/local/bin/bose-connect`.

### Dependencies

* macOS (Apple Silicon)
* Xcode Command Line Tools (`xcode-select --install`)
* CMake

Install CMake using Homebrew:

```bash
brew install cmake
```

The app now uses Apple's `IOBluetooth` framework directly.

### Docker (Legacy path)

The Docker workflow is kept only as a legacy setup. The native build below is
the primary path for macOS.

Follow the next steps:

```bash
# Set up the host's user ID and group.
echo "USER_ID=$(id -u "${USER}")" >./src/.env-user
echo "GROUP_ID=$(id -g "${USER}")" >>./src/.env-user

# Clean previous docker composes.
docker-compose \
  --project-directory ./src \
  --env-file ./src/.env-user \
  down

# Start the docker compose.
docker-compose \
  --project-directory ./src \
  --env-file ./src/.env-user \
  up \
  --detach \
  --build

# Build the application.
docker exec \
  --user $(id -u "${USER}") \
  --interactive \
  --tty \
  bose-connect \
  /root/bose-connect/script/build-prod.bash

# Enjoy.
./src/build/bose-connect
```

*Note: The native macOS path is the maintained setup for this repository.*

### Local

The local build requires `clang`, `make`, and `cmake`.

```bash
# Execute the Bash script.
./src/script/build-prod.bash

# Enjoy.
./src/build/bose-connect
```

### Install

Run `./src/script/install-prod.bash` to install the application. It will place
in `/usr/local/bin/bose-connect`. The `PREFIX` and `DESTDIR`
variables are assignable and have the traditional meaning. For more information
reefer to the [official web site of CMake][cmake-install].

The app expects the Bose device Bluetooth address, for example:

```bash
./src/build/bose-connect --battery-level E4:58:BC:3C:B7:AF
```

Aliases are stored in `~/.bose-connect-devices` as simple `alias=address`
lines:

```bash
./src/build/bose-connect --add-alias=qc35 E4:58:BC:3C:B7:AF
./src/build/bose-connect --battery-level --alias=qc35
./src/build/bose-connect --list-devices
```

### Uninstall

Run the script `./src/script/uninstall.bash`.

## Contribute

Check the file [CONTRIBUTING.md][contributing] for more information. It
includes the instructions for build with special configuration for development.

## To-Do's List

Visit the document with all the checkpoints in [TODO.md][todo.md].

## Development Notes

For more information about the details of how use the firmwares to found
functionality, please review the file [DEVELOPMENT.md][details-file].

## Disclaimer

This has only been tested on Bose `QuietComfort` with firmware 1.0.6. I cannot
ensure that this program works on any other devices.


[Denton-L]: https://github.com/Denton-L/based-connect

[details-file]: ./DEVELOPMENT.md

[contributing]: ./CONTRIBUTING.md

[cmake-install]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project

[new-issue]: https://github.com/airvzxf/bose-connect/issues/new
