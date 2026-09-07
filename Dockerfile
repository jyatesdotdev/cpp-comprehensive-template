# syntax=docker/dockerfile:1

# ── Build Stage ──────────────────────────────────────────────────────
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake ninja-build git curl zip unzip tar pkg-config ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg (pinned to the same commit as vcpkg.json builtin-baseline)
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && git -C "$VCPKG_ROOT" fetch origin 04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4 \
    && git -C "$VCPKG_ROOT" checkout 04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4 \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /src

# Layer cache: install vcpkg deps from the manifest before copying sources
COPY vcpkg.json ./
RUN "$VCPKG_ROOT/vcpkg" install --triplet x64-linux

COPY CMakeLists.txt CMakePresets.json ./
COPY cmake/ cmake/
COPY include/ include/
COPY src/ src/
COPY examples/ examples/
RUN cmake -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DBUILD_DOCS=OFF \
        -DENABLE_RENDERING=OFF \
    && cmake --build build --target hello_world

# ── Runtime Stage ────────────────────────────────────────────────────
FROM debian:bookworm-slim AS runtime

LABEL org.opencontainers.image.title="CppComprehensiveTemplate" \
      org.opencontainers.image.description="Modern C++ comprehensive project template" \
      org.opencontainers.image.version="1.0.0" \
      org.opencontainers.image.source="https://github.com/example/cpp-comprehensive-template"

RUN apt-get update && apt-get install -y --no-install-recommends \
        libstdc++6 tini \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd -r app && useradd -r -g app -d /app -s /sbin/nologin app

WORKDIR /app
COPY --from=build /src/build/examples/hello_world .

USER app
ENTRYPOINT ["tini", "--"]
CMD ["./hello_world"]
