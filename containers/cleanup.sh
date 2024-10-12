#!/bin/bash

# To remove all images without an associated container,
# supply -a argument.
#
# See doc for: podman image prune -a

podman image prune $@
