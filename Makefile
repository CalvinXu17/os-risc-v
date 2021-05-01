include Makefile.header

BUILD = ./build/

LDFLAGS += -nostdlib --gc-sections
CFLAGS += -fno-builtin -nostdlib -nostdinc -fno-stack-protector -ffunction-sections -fdata-sections -Wall # -O
CFLAGS += -I./os/kernel/include -I./os/driver/include -I./os/lib/include -I./os/fat32/include
qemu: CFLAGS += -D_QEMU
qemu: LDFLAGS += -T ./qemu.ld
k210: CFLAGS += -D_K210
k210: LDFLAGS += -T ./k210.ld

buildos:
	$(AS) -o $(BUILD)boot.o ./os/kernel/boot.S
	$(AS) -o $(BUILD)intr_s.o ./os/kernel/intr.S
	$(CC) $(CFLAGS) -c -O -o $(BUILD)intr.o ./os/kernel/intr.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)timer.o ./os/kernel/timer.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)panic.o ./os/kernel/panic.c
	$(CC) $(CFLAGS) -c -o $(BUILD)printk.o ./os/kernel/printk.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)console.o ./os/kernel/console.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)start.o ./os/kernel/start.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)cpu.o ./os/kernel/cpu.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)page.o ./os/kernel/page.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)slob.o ./os/kernel/slob.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)kmalloc.o ./os/kernel/kmalloc.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)spinlock.o ./os/kernel/spinlock.c

	$(CC) $(CFLAGS) -c -O -o $(BUILD)plic.o ./os/driver/plic.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)fpioa.o ./os/driver/fpioa.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)gpiohs.o ./os/driver/gpiohs.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)spi.o ./os/driver/spi.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)sysctl.o ./os/driver/sysctl.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)utils.o ./os/driver/utils.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)sdcard_test.o ./os/driver/sdcard_test.c

	$(CC) $(CFLAGS) -c -O -o $(BUILD)disk.o ./os/fat32/disk.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)fat32.o ./os/fat32/fat32.c

	$(CC) $(CFLAGS) -c -O -o $(BUILD)string.o ./os/lib/string.c
	
	$(LD) $(LDFLAGS) -o $(BUILD)kernel $(BUILD)boot.o \
									   $(BUILD)start.o \
									   $(BUILD)cpu.o \
									   $(BUILD)panic.o \
									   $(BUILD)printk.o \
									   $(BUILD)intr_s.o \
									   $(BUILD)intr.o \
									   $(BUILD)timer.o \
									   $(BUILD)page.o \
									   $(BUILD)slob.o \
									   $(BUILD)kmalloc.o \
									   $(BUILD)spinlock.o \
									   $(BUILD)console.o \
									   $(BUILD)plic.o \
									   $(BUILD)fpioa.o \
									   $(BUILD)gpiohs.o \
									   $(BUILD)spi.o \
									   $(BUILD)sysctl.o \
									   $(BUILD)utils.o \
									   $(BUILD)sdcard_test.o \
									   $(BUILD)disk.o \
									   $(BUILD)fat32.o \
									   $(BUILD)string.o
	$(OBJCOPY) $(BUILD)kernel --strip-all -O binary $(BUILD)kernel.bin
	$(NM) $(BUILD)/kernel | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)'| sort > ./kernel.map

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
