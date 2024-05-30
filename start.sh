#!/bin/bash

GREEN='\033[0;32m'
NC='\033[0m'
echo -e "${GREEN}Compiling with make${NC}"

sudo make -C /lib/modules/$(uname -r)/build M=$PWD

echo -e "${GREEN}After compiling${NC}"

echo -e "${GREEN}Removing old modulo.ko${NC}"

sudo rmmod modulo.ko

echo -e "${GREEN}Adding new modulo.ko${NC}"

sudo insmod modulo.ko

echo -e "${GREEN}Last 10 lines of dmesg${NC}"

sudo dmesg | tail
