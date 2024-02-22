# This file is a part of the UMSKT Project
#
# Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# @FileCreated by Neo on 06/19/2023
# @Maintainer Neo

# Stage 1: Install Prereqisites
FROM alpine:latest as prereqisites

# Install build dependencies
RUN apk add --no-cache \
  bash \
  build-base \
  cmake \
  git \
  musl-dev


# Stage 2: Build
FROM prereqisites as builder

WORKDIR /src
COPY . /src

# Build UMSKT from the local directory
RUN mkdir /src/build \
  && cmake -B /src/build -DCPM_SOURCE_CACHE=/src/.cpm-cache -DCMAKE_BUILD_TYPE=Release -DUMSKT_MUSL_STATIC=ON \
  && cmake --build /src/build -j 10

# Stage 3: Output
FROM scratch as output

COPY --from=builder /src/build/umskt /
COPY --from=builder /src/build/libumskt_static.a /
COPY --from=builder /src/build/libumskt.so /

# invoke via
# docker build -o type=tar,dest=build-musl/umskt.tar .
