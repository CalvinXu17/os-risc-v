include Makefile.header

BUILD = ./build/

LDFLAGS += -nostdlib --gc-sections
CFLAGS += -fno-builtin -nostdlib -nostdinc -fno-stack-protector -ffunction-sections -fdata-sections -Wall -D_DEBUG -O
CFLAGS += -I./os/kernel/include -I./os/driver/include -I./os/lib/include -I./os/fat32/include -I./os/xfat/include
qemu: CFLAGS += -D_QEMU
qemu: LDFLAGS += -T ./qemu.ld
k210: CFLAGS += -D_K210
k210: LDFLAGS += -T ./k210.ld

buildos:
	$(CC) $(CFLAGS) -c -o $(BUILD)boot.o ./os/kernel/boot.S
	$(CC) $(CFLAGS) -c -o $(BUILD)intr_s.o ./os/kernel/intr.S
	$(CC) $(CFLAGS) -c -o $(BUILD)intr.o ./os/kernel/intr.c
	$(CC) $(CFLAGS) -c -o $(BUILD)timer.o ./os/kernel/timer.c

	$(CC) $(CFLAGS) -c -o $(BUILD)switch.o ./os/kernel/switch.S
	$(CC) $(CFLAGS) -c -o $(BUILD)sched.o ./os/kernel/sched.c
	$(CC) $(CFLAGS) -c -o $(BUILD)process.o ./os/kernel/process.c
	$(CC) $(CFLAGS) -c -o $(BUILD)syscall.o ./os/kernel/syscall.c

	$(CC) $(CFLAGS) -c -o $(BUILD)cpu.o ./os/kernel/cpu.c
	$(CC) $(CFLAGS) -c -o $(BUILD)page.o ./os/kernel/page.c
	$(CC) $(CFLAGS) -c -o $(BUILD)slob.o ./os/kernel/slob.c
	$(CC) $(CFLAGS) -c -o $(BUILD)kmalloc.o ./os/kernel/kmalloc.c
	$(CC) $(CFLAGS) -c -o $(BUILD)osmain.o ./os/kernel/osmain.c

	$(CC) $(CFLAGS) -c -o $(BUILD)spinlock.o ./os/kernel/spinlock.c
	$(CC) $(CFLAGS) -c -o $(BUILD)sem.o ./os/kernel/sem.c
	$(CC) $(CFLAGS) -c -o $(BUILD)panic.o ./os/kernel/panic.c
	$(CC) $(CFLAGS) -c -o $(BUILD)printk.o ./os/kernel/printk.c
	$(CC) $(CFLAGS) -c -o $(BUILD)console.o ./os/kernel/console.c
	$(CC) $(CFLAGS) -c -o $(BUILD)string.o ./os/lib/string.c
	
	$(CC) $(CFLAGS) -c -o $(BUILD)plic.o ./os/driver/plic.c
	$(CC) $(CFLAGS) -c -o $(BUILD)fpioa.o ./os/driver/fpioa.c
	$(CC) $(CFLAGS) -c -o $(BUILD)gpiohs.o ./os/driver/gpiohs.c
	$(CC) $(CFLAGS) -c -o $(BUILD)spi.o ./os/driver/spi.c
	$(CC) $(CFLAGS) -c -o $(BUILD)utils.o ./os/driver/utils.c
	$(CC) $(CFLAGS) -c -o $(BUILD)sdcard.o ./os/driver/sdcard.c

	$(CC) $(CFLAGS) -c -o $(BUILD)xdisk.o ./os/xfat/xdisk.c
	$(CC) $(CFLAGS) -c -o $(BUILD)xdriver.o ./os/xfat/xdriver.c
	$(CC) $(CFLAGS) -c -o $(BUILD)xfat_buf.o ./os/xfat/xfat_buf.c
	$(CC) $(CFLAGS) -c -o $(BUILD)xfat_obj.o ./os/xfat/xfat_obj.c
	$(CC) $(CFLAGS) -c -o $(BUILD)xfat.o ./os/xfat/xfat.c

	$(LD) $(LDFLAGS) -o $(BUILD)kernel $(BUILD)boot.o \
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

UBUILD = ./build/user/
UCFLAGS = -mcmodel=medany -march=rv64imafdc -fno-builtin -nostdlib -nostdinc -fno-stack-protector -ffunction-sections -fdata-sections -Wall -O

builduser: UCFLAGS += -I./os/user/include

builduser:
	$(CC) $(UCFLAGS) -c -o $(UBUILD)crt.o ./os/user/lib/crt.S
	$(CC) $(UCFLAGS) -c -o $(UBUILD)main.o ./os/user/lib/main.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)stdio.o ./os/user/lib/stdio.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)stdlib.o ./os/user/lib/stdlib.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)string.o ./os/user/lib/string.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)syscall.o ./os/user/lib/syscall.c
	$(CC) $(UCFLAGS) -c -o $(UBUILD)app.o ./os/user/app.c
	$(LD) -m elf64lriscv -nostdlib --gc-sections -T ./os/user/user.ld -Ttext 0x1000 \
			-o $(UBUILD)app $(UBUILD)crt.o \
							$(UBUILD)main.o \
							$(UBUILD)app.o \
							$(UBUILD)stdio.o \
							$(UBUILD)stdlib.o \
							$(UBUILD)string.o \
							$(UBUILD)syscall.o

	$(OBJCOPY) $(UBUILD)app --strip-all -O binary $(UBUILD)app.bin
	cp $(UBUILD)app.bin /mnt/c/Users/Calvin/Desktop/app.bin
	powershell.exe C:/Users/Calvin/Desktop/runbin.bat app

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

all: k210
	dd if=/dev/zero of=disk.img bs=3m count=1024
	mkfs.vfat -F 32 disk.img
	cp ./build/k210.bin ./k210.bin

run: k210
	cp ./build/k210.bin /mnt/c/Users/Calvin/Desktop/k210.bin
	powershell.exe C:/Users/Calvin/Desktop/run.bat
	
clean:
	rm -f ./build/*.o
	rm -f ./build/user/*.o
cleanx:
	rm -f ./build/*.o
	rm -f ./build/*.bin
	rm -f ./build/user/*
	rm -f ./build/kernel
	rm -f ./build/app

