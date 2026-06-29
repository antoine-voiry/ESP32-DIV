FROM python:3.11-slim

# System build tools — native_tests GCC compilation + pio toolchain extraction
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        g++ \
        curl \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# PlatformIO Core (6.x, not 7)
RUN pip install --no-cache-dir "platformio>=6.1,<7"

# ─── Pre-cache ESP32 platform + Xtensa toolchain + all hardware lib_deps ─────
# COPY only the ini so this Docker layer is invalidated only when ini changes
COPY platformio.ini /tmp/preload/platformio.ini
# Stub main.cpp so pio pkg install is satisfied without a real build
RUN mkdir -p /tmp/preload/src && \
    printf '#include <Arduino.h>\nvoid setup(){} void loop(){}' \
        > /tmp/preload/src/main.cpp && \
    cd /tmp/preload && \
    pio pkg install -e esp32_v1_hardware && \
    rm -rf /tmp/preload
# ─────────────────────────────────────────────────────────────────────────────

# Wokwi CLI (pinned for reproducibility)
ARG WOKWI_CLI_VERSION=0.14.0
RUN curl -fsSL \
      "https://github.com/wokwi/wokwi-cli/releases/download/v${WOKWI_CLI_VERSION}/wokwi-cli-linux-x64" \
      -o /usr/local/bin/wokwi-cli && \
    chmod +x /usr/local/bin/wokwi-cli

WORKDIR /workspace

# Default entrypoint: native unit tests
CMD ["pio", "test", "-e", "native_tests"]
