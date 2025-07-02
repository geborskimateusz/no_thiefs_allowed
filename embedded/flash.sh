#!/bin/bash

# Define the USB port (update if needed)
USB_PORT="ttyUSB0"

# Build Docker image
docker build -f docker/Dockerfile -t esp32-docker .

# Run Docker container with device access and execute build + flash + monitor
docker run --rm -it \
  --device=/dev/${USB_PORT} \
  esp32-docker \
  bash -c "idf.py fullclean && idf.py build && idf.py -p /dev/${USB_PORT} flash && idf.py -p /dev/${USB_PORT} monitor"
