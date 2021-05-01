include Makefile.header

BUILD = ./build/
INC = ./os/kernel/include

LDFLAGS += -nostdlib --gc-sections
CFLAGS += -fno-builtin -nostdlib -fno-stack-protector -ffunction-sections -fdata-sections -Wall # -O
CFLAGS += -I$(INC)
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
	$(CC) $(CFLAGS) -c -O -o $(BUILD)start.o ./os/kernel/start.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)cpu.o ./os/kernel/cpu.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)page.o ./os/kernel/page.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)slob.o ./os/kernel/slob.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)kmalloc.o ./os/kernel/kmalloc.c
	$(CC) $(CFLAGS) -c -O -o $(BUILD)spinlock.o ./os/kernel/spinlock.c
	$(LD) $(LDFLAGS) -o $(BUILD)kernel $(BUILD)boot.o $(BUILD)start.o $(BUILD)cpu.o $(BUILD)panic.o $(BUILD)printk.o $(BUILD)intr_s.o $(BUILD)intr.o $(BUILD)timer.o $(BUILD)page.o $(BUILD)slob.o $(BUILD)kmalloc.o $(BUILD)spinlock.o
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
	rm -fr !(.gitignore)