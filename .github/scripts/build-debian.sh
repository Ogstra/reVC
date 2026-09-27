#!/bin/bash
# Builds the game inside a Debian 11 container (see release.yml).
# Debian 11 is past its support period, its packages move from deb.debian.org to archive.debian.org,
# fall back to the archive for the parts that are gone.
set -e

PACKAGES="build-essential cmake ninja-build pkg-config libglfw3-dev libopenal-dev libmpg123-dev libsndfile1-dev"
export DEBIAN_FRONTEND=noninteractive

install_packages() {
	apt-get -o Acquire::Check-Valid-Until=false update &&
	apt-get install -y --no-install-recommends $PACKAGES
}

if ! install_packages; then
	echo "Retrying with the security updates from archive.debian.org"
	sed -i -e 's|http://deb.debian.org/debian-security|http://archive.debian.org/debian-security|g' /etc/apt/sources.list
	if ! install_packages; then
		echo "Retrying with everything from archive.debian.org"
		sed -i -e 's|http://deb.debian.org/debian|http://archive.debian.org/debian|g' -e '/bullseye-updates/d' /etc/apt/sources.list
		install_packages
	fi
fi

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLIBRW_PLATFORM=GL3 -DLIBRW_GL3_GFXLIB=GLFW
cmake --build build
chown -R "$HOST_UID:$HOST_GID" build
