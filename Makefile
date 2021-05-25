include Makefile.header

WSL_WINPATH = /mnt/c/Users/Calvin/Desktop/
WINPATH = C:/Users/Calvin/Desktop/

BUILD = ./build/

LFLAGS += -nostdlib -Wl,-gc-sections
CFLAGS += -fno-builtin -nostdlib -nostdinc -fno-stack-protector -Wall

KCFLAGS = $(CFLAGS) -D_DEBUG -O
KCFLAGS += -I./os/kernel/include -I./os/driver/include -I./os/lib/include -I./os/fat32/include -I./os/xfat/include
KLFLAGS = $(LFLAGS)

qemu: KCFLAGS += -D_QEMU
qemu: KLFLAGS += -T ./qemu.ld
KCFLAGS += -D_K210
KLFLAGS += -T ./k210.ld

all: buildos							   
	cp ./sbi/k210/rustsbi-k210.bin ./build/k210.bin
	dd if=./build/kernel.bin of=./build/k210.bin bs=128k seek=1
	cp ./build/k210.bin ./k210.bin

buildos:
	$(CC) $(KCFLAGS) -c -o $(BUILD)boot.o ./os/kernel/boot.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)intr_s.o ./os/kernel/intr.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)intr.o ./os/kernel/intr.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)timer.o ./os/kernel/timer.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)switch.o ./os/kernel/switch.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)sched.o ./os/kernel/sched.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)process.o ./os/kernel/process.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)syscall.o ./os/kernel/syscall.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)cpu.o ./os/kernel/cpu.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)page.o ./os/kernel/page.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)slob.o ./os/kernel/slob.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)kmalloc.o ./os/kernel/kmalloc.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)osmain.o ./os/kernel/osmain.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)spinlock.o ./os/kernel/spinlock.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)sem.o ./os/kernel/sem.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)panic.o ./os/kernel/panic.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)printk.o ./os/kernel/printk.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)console.o ./os/kernel/console.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)string.o ./os/lib/string.c
	
	$(CC) $(KCFLAGS) -c -o $(BUILD)plic.o ./os/driver/plic.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)fpioa.o ./os/driver/fpioa.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)gpiohs.o ./os/driver/gpiohs.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)spi.o ./os/driver/spi.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)utils.o ./os/driver/utils.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)sdcard.o ./os/driver/sdcard.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)xdisk.o ./os/xfat/xdisk.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)xdriver.o ./os/xfat/xdriver.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)xfat_buf.o ./os/xfat/xfat_buf.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)xfat_obj.o ./os/xfat/xfat_obj.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)xfat.o ./os/xfat/xfat.c

	$(CC) $(KLFLAGS) -o $(BUILD)kernel $(BUILD)boot.o \
									   $(BUILD)intr_s.o \
									   $(BUILD)intr.o \
									   $(BUILD)timer.o \
									   $(BUILD)switch.o \
									   $(BUILD)sched.o \
									   $(BUILD)process.o \
									   $(BUILD)syscall.o \
									   $(BUILD)cpu.o \
									   $(BUILD)page.o \
									   $(BUILD)slob.o \
									   $(BUILD)kmalloc.o \
									   $(BUILD)osmain.o \
									   $(BUILD)spinlock.o \
									   $(BUILD)sem.o \
									   $(BUILD)panic.o \
									   $(BUILD)printk.o \
									   $(BUILD)console.o \
									   $(BUILD)plic.o \
									   $(BUILD)fpioa.o \
									   $(BUILD)gpiohs.o \
									   $(BUILD)spi.o \
									   $(BUILD)utils.o \
									   $(BUILD)sdcard.o \
									   $(BUILD)string.o \
									   $(BUILD)xdisk.o \
									   $(BUILD)xdriver.o \
									   $(BUILD)xfat_buf.o \
									   $(BUILD)xfat_obj.o \
									   $(BUILD)xfat.o \
									   
									   
	$(OBJCOPY) $(BUILD)kernel --strip-all -O binary $(BUILD)kernel.bin
	$(NM) $(BUILD)/kernel | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)'| sort > ./kernel.map

UBUILD = $(BUILD)/user/
UCFLAGS = $(CFLAGS) -O -I./os/user/include
ULFLAGS = $(LFLAGS) -T ./os/user/user.ld

builduser:
	$(CC) $(UCFLAGS) -c -o $(UBUILD)crt.o ./os/user/lib/crt.S
	$(CC) $(UCFLAGS) -c -o $(UBUILD)clone.o ./os/user/lib/clone.S
	$(CC) $(UCFLAGS) -c -o $(UBUILD)main.o ./os/user/lib/main.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)stdio.o ./os/user/lib/stdio.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)stdlib.o ./os/user/lib/stdlib.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)string.o ./os/user/lib/string.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)syscall.o ./os/user/lib/syscall.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)app.o ./os/user/app.c
	$(CC) $(ULFLAGS) -Ttext 0x1000 \
			-o $(UBUILD)app $(UBUILD)crt.o \
							$(UBUILD)main.o \
							$(UBUILD)app.o \
							$(UBUILD)stdio.o \
							$(UBUILD)stdlib.o \
							$(UBUILD)string.o \
							$(UBUILD)clone.o \
							$(UBUILD)syscall.o

	$(OBJCOPY) $(UBUILD)app --strip-all -O binary $(UBUILD)app.bin
	cp $(UBUILD)app.bin $(WSL_WINPATH)app.bin
	powershell.exe $(WINPATH)runbin.bat app

# import virtual disk image
# QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0 
# QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: buildos
	qemu-system-riscv64 -machine virt \
				-smp 2 \
				-m 8M \
				-nographic \
				-bios ./sbi/qemu/rustsbi-qemu.bin \
				-kernel $(BUILD)kernel.bin \
				-device loader,file=$(BUILD)kernel.bin,addr=0x80200000

k210: buildos
	cp ./sbi/k210/rustsbi-k210.bin ./build/k210.bin
	dd if=./build/kernel.bin of=./build/k210.bin bs=128k seek=1

run: k210
	cp ./build/k210.bin $(WSL_WINPATH)k210.bin
	powershell.exe $(WINPATH)run.bat
	
clean:
	rm -f ./build/*.o
	rm -f ./build/user/*.o
cleanx:
	rm -f ./build/*.o
	rm -f ./build/*.bin
	rm -f ./build/user/*
	rm -f ./build/kernel
	rm -f ./build/app

