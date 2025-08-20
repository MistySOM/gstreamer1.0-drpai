FROM ubuntu:24.04

RUN sed -i -e 's|http://archive.ubuntu.com/ubuntu/|http://ca.archive.ubuntu.com/ubuntu/|' /etc/apt/sources.list.d/ubuntu.sources

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    bash-completion \
    ca-certificates \
    clang-tidy \
    file \
    findutils \
    gdb \
    git \
    g++ \    
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    net-tools \
    ninja-build \
    openssh-client \
    python3-pip \
    rsync \
    sshpass \
    sudo \
    unzip \
    wget \
    xz-utils \
    && \
    rm -rf /var/lib/apt/lists/*

RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER ubuntu
