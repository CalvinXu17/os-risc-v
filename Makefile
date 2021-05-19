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
	$(AS) -o $(BUILD)boot.o ./os/kernel/boot.S
	$(AS) -o $(BUILD)intr_s.o ./os/kernel/intr.S
	$(CC) $(CFLAGS) -c -o $(BUILD)intr.o ./os/kernel/intr.c
	$(CC) $(CFLAGS) -c -o $(BUILD)timer.o ./os/kernel/timer.c

	$(AS) -o $(BUILD)switch.o ./os/kernel/switch.S
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
	$(CC) $(CFLAGS) -c -o $(BUILD)sysctl.o ./os/driver/sysctl.c
	$(CC) $(CFLAGS) -c -o $(BUILD)utils.o ./os/driver/utils.c
	$(CC) $(CFLAGS) -c -o $(BUILD)sdcard_test.o ./os/driver/sdcard_test.c

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
									   $(BUILD)sysctl.o \
									   $(BUILD)utils.o \
									   $(BUILD)sdcard_test.o \
									   $(BUILD)string.o \
									   $(BUILD)xdisk.o \
									   $(BUILD)xdriver.o \
									   $(BUILD)xfat_buf.o \
									   $(BUILD)xfat_obj.o \
									   $(BUILD)xfat.o \
									   
									   
	$(OBJCOPY) $(BUILD)kernel --strip-all -O binary $(BUILD)kernel.bin
	$(NM) $(BUILD)/kernel | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)'| sort > ./kernel.map


builduser: CFLAGS += -I./os/user/include

builduser:
	$(CC) $(CFLAGS) -c -S -O -o $(BUILD)app.S ./os/user/app.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)app.o ./os/user/app.c
	$(LD) -m elf64lriscv -nostdlib -Ttext 0x0 -e AppMain -o $(BUILD)app $(BUILD)app.o
	$(OBJCOPY) $(BUILD)app --strip-all -O binary $(BUILD)app.bin

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
	cp ./build/k210.bin /mnt/c/Users/Calvin/Desktop/k210.bin
	powershell.exe C:/Users/Calvin/Desktop/run.bat
clean:
	rm -f ./build/*.o
cleanx:
	rm -f ./build/*.o
	rm -f ./build/*.bin
	rm -f ./build/kernel
	rm -f ./build/app
