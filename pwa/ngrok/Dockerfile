FROM alpine:latest

# Install deps
RUN apk add --no-cache curl gettext

# Download and install ngrok manually
RUN curl -L https://bin.equinox.io/c/bNyj1mQVY4c/ngrok-stable-linux-amd64.zip -o ngrok.zip \
    && unzip ngrok.zip -d /usr/local/bin \
    && chmod +x /usr/local/bin/ngrok \
    && rm ngrok.zip

# Copy config template and entrypoint script
COPY ngrok.yml.template /ngrok.yml.template
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
