# syntax=docker/dockerfile:1

# ── Build Stage ──────────────────────────────────────────────────────
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake ninja-build git curl zip unzip tar pkg-config ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /src

# Layer cache: install dependencies before copying source
COPY vcpkg.json CMakeLists.txt CMakePresets.json ./
COPY cmake/ cmake/
RUN cmake -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=OFF \
        -DBUILD_BENCHMARKS=OFF \
        -DBUILD_DOCS=OFF \
        -DENABLE_RENDERING=OFF \
    || true  # allow partial configure to trigger vcpkg install

# Copy source and build
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

HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD ["./hello_world"]
