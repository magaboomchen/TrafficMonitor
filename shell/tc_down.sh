#!/bin/bash
INTERFACE=ovirtmgmt
echo "del qdisc of interface:" $INTERFACE
tc qdisc del dev $INTERFACE root