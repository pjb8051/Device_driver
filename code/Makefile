APP := ledkey_app_pjb
MOD := ledkey_dev_pjb
SRC := $(APP).o
obj-m := $(MOD).o

CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
CC := arm-linux-gnueabihf-gcc
KDIR := `your git clone directory`/linux
PWD := $(shell pwd)

default:$(APP)
	$(MAKE) -C $(KDIR) M=$(PWD) modules $(CROSS)
$(APP):$(SRC)
	$(CC) $(APP).c -o $(APP)
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf modules.order
	rm -rf Module.symvers
	rm -rf $(MOD).mod
	rm -rf .tmp_versions
	rm -rf $(APP)
