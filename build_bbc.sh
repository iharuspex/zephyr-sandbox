#!/bin/sh

west build -p always -b bbc_microbit app -- -DEXTRA_CONF_FILE=debug.conf
# west build -b bbc_microbit app -- -DEXTRA_CONF_FILE=debug.conf