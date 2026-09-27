#!/bin/bash
# Builds the game inside the Linux container picked in release.yml
set -e

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
	build-essential cmake ninja-build pkg-config libglfw3-dev libopenal-dev libmpg123-dev libsndfile1-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBRW_PLATFORM=GL3 -DLIBRW_GL3_GFXLIB=GLFW
cmake --build build
chown -R "$HOST_UID:$HOST_GID" build
