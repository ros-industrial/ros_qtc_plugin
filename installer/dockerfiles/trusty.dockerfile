FROM ubuntu:14.04

MAINTAINER Levi Armstrong

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update
RUN apt-get install -y wget
RUN apt-get install -y git
RUN apt-get install -y p7zip p7zip-full p7zip-rar
RUN apt-get install -y make
RUN apt-get install -y libyaml-cpp-dev
RUN apt-get install -y pkg-config
RUN apt-get install -y autoconf
RUN apt-get install -y g++-4.9 gcc-4.9 g++ gcc 
RUN ln -sf /usr/bin/gcc-4.9 /usr/bin/gcc
RUN ln -sf /usr/bin/g++-4.9 /usr/bin/g++
RUN apt-get install -y libboost-dev
RUN apt-get install -y cmake
RUN apt-get install -y libgl1-mesa-dev
RUN apt-get install -y libfontconfig1
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y python
RUN apt-get install -y chrpath
RUN apt-get upgrade -y
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US
