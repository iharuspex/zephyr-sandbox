#!/bin/sh

west build -p always -b stm32_min_dev app -- -DEXTRA_CONF_FILE=debug.conf