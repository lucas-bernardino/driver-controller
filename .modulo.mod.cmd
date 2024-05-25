cmd_/home/user/drivers_so/modulo.mod := printf '%s\n'   modulo.o | awk '!x[$$0]++ { print("/home/user/drivers_so/"$$0) }' > /home/user/drivers_so/modulo.mod
