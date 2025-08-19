FROM debian:trixie

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        bash-completion \
        clang-tidy \
        gdb \
        git \
        g++ \
        meson \
        ninja-build \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        openssh-client \
        sudo \
        && \
        rm -rf /var/lib/apt/lists/*

RUN useradd --shell /bin/bash --create-home ubuntu
RUN usermod -aG sudo ubuntu && \
    echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu

USER ubuntu
