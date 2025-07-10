#!/bin/sh

# Generate config file with environment variables
envsubst < /ngrok.yml.template > /tmp/ngrok.yml

# Run ngrok with generated config
exec ngrok start --config /tmp/ngrok.yml --all
