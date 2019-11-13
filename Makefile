KDIR		:= /lib/modules/$(shell uname -r)/build
#KDIR    := ~/src/linux
SOURCES		:= $(patsubst %.o,%.c,$(OBJECTS))

obj-m += interceptor.o

all:		$(obj-m)

$(obj-m):	interceptor.c
		$(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd) modules	

test:		interceptor.c 
		echo "Datein sind alt...\n" $(MODULES)

clean:		
		rm -rf *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers .tmp_versions/
