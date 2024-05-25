cmd_/home/user/drivers_so/Module.symvers :=  sed 's/ko$$/o/'  /home/user/drivers_so/modules.order | scripts/mod/modpost -m     -o /home/user/drivers_so/Module.symvers -e -i Module.symvers -T - 
