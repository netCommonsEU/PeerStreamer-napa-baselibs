NAPA-BASELIBS
=============

NAPA-BASELIBS is a collection of open-source libraries for P2P streaming,
developed in the NAPA-WINE project.

If you need the NAPA libraries for the PeerStreamer framework, refer to the
[PeerStreamer's build system]
(https://github.com/netCommonsEU/PeerStreamer-build).

# Documentation for Developers

Tested on Ubuntu 16.04 LTS (it should work on any Linux distribution with
proper developing tools installed).

## Prerequisites

Developments versions of the following libraries are needed:
 - libevent2 (http://monkey.org/~provos/libevent/ - note that we
   use version 2.0, generally referred to as libevent2)
 - libconfuse (http://www.nongnu.org/confuse/)

These libraries are authomatically downloaded by the build script (build_all.sh).

## Build

Execute:

`build_all.sh -q`


