#!/bin/sh

west build -p always -b nrf52840dk/nrf52840 app -- -DEXTRA_CONF_FILE=debug.conf