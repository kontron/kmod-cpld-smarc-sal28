obj-m := sl28cpld.o sl28cpld-gpio.o
sl28cpld-y := core.o
sl28cpld-gpio-y := gpio.o

ccflags-y := -Wall -Werror

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$$PWD $@
