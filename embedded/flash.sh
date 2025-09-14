#!/bin/bash

# Define the USB port (update if needed)
USB_PORT="ttyUSB0"

idf.py fullclean && idf.py build && idf.py -p /dev/ttyUSB0 flash && idf.py -p /dev/ttyUSB0 monitor
