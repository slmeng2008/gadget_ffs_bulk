CC=/home/shlmeng/rv1126_repo/buildroot/output/rockchip_rv1126_rv1109_am_tx3/host/bin/arm-linux-gnueabihf-gcc
INC := /home/shlmeng/rv1126_repo/buildroot/output/rockchip_rv1126_rv1109_am_tx3/host/arm-buildroot-linux-gnueabihf/sysroot/usr/include/
CFLAGS	:= -Wall -O2 -g -DAIO -I$(INC) -I/home/shlmeng/RV1126_repo/buildroot/output/rockchip_rv1126_rv1109_am_tx3/build/libaio-0.3.110/src/ -static
LIBS	:= -lpthread -laio

obj=aio_simple.o
target=aio_simple

$(target):$(obj)
	$(CC) $(CFLAGS) $(obj) -o $(target) $(LIBS)
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY:clean
clean:
	rm $(obj) $(target) -f
