#!/bin/bash

# Check if token is provided
if [ -z "$1" ]; then
  echo "Usage: ./run.sh <API_AUTH_TOKEN>"
  exit 1
fi

API_AUTH_TOKEN=$1
IMAGE_NAME="flask-auth-api"
CONTAINER_NAME="auth-api"

# Build Docker image
echo "Building Docker image..."
docker build -t $IMAGE_NAME .

# Stop and remove any existing container
echo "Cleaning up old container (if any)..."
docker rm -f $CONTAINER_NAME 2>/dev/null || true

# Run the container
echo "Running container..."
docker run -d \
  -p 5000:5000 \
  -e API_AUTH_TOKEN=$API_AUTH_TOKEN \
  --name $CONTAINER_NAME \
  $IMAGE_NAME

