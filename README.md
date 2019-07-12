# Virtualized Desktop Traffic Monitor

## Prerequisites
libcap-devel
gcc-c++

## Installation
make

## Usage
monitor [-i interface] [-ip ipAddress] [-f file]

## Example
monitor -i ovirtmgmt -ip 10.18.15.177  
monitor -i ovirtmgmt -f ./tmp/ip_list.txt

## IP address format in file
Each line only has one ip address, such as:  
10.18.15.177  
10.18.15.109  
10.18.15.113  