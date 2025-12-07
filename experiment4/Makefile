# Makefile
obj-m := gpio_pwm_driver.o
KDIR := /home/student/linux_4.19.190.7.9
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
