#!/bin/bash

# BUILD AND TEST IN A PODMAN/DOCKER CONTAINER

# Exit immediately if a command does not succeed
set -e

cd /opt/project

# -------------------------------------------------------
# Unit test builds (all debugging enabled)
# -------------------------------------------------------

make clean
make debug
make test

# -------------------------------------------------------
# Production build
# -------------------------------------------------------

make clean
make release
make test
