FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Added 'bear' and 'gawk' to the list
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    binutils-arm-none-eabi \
    ccache \
    cppcheck \
    clang-format \
    gawk \
    make \
    bear \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# ... rest of your Dockerfile (CCACHE envs, etc.)
WORKDIR /workspace

CMD ["sh", "-c", "make all -j$(nproc)"]
