cmd_/home/user/driver/driver-controller/modulo.mod := printf '%s\n'   modulo.o | awk '!x[$$0]++ { print("/home/user/driver/driver-controller/"$$0) }' > /home/user/driver/driver-controller/modulo.mod
