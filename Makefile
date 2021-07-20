include Makefile.header

WSL_WINPATH = /mnt/c/Users/Calvin/Desktop/
WINPATH = C:/Users/Calvin/Desktop/

BUILD = ./build/

LFLAGS += -nostdlib -Wl,-gc-sections
CFLAGS += -fno-builtin -nostdlib -nostdinc -fno-stack-protector -Wall

KCFLAGS = $(CFLAGS) -O
KCFLAGS += -I./os/kernel/include -I./os/driver/include -I./os/lib/include
KCFLAGS += -I./os/vfs/include -I./os/fatfs/include -I./os/hal/include
KLFLAGS = $(LFLAGS)

all: k210
	cp ./build/k210.bin ./k210.bin

qemu: KCFLAGS += -D_QEMU # -D_STRACE -D_DEBUG
qemu: KLFLAGS += -T ./qemu.ld
k210: KCFLAGS += -D_K210 # -D_STRACE -D_DEBUG
k210: KLFLAGS += -T ./k210.ld

buildos:
	$(CC) $(KCFLAGS) -c -o $(BUILD)boot.o ./os/kernel/boot.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)intr_s.o ./os/kernel/intr.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)intr.o ./os/kernel/intr.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)timer.o ./os/kernel/timer.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)switch.o ./os/kernel/switch.S
	$(CC) $(KCFLAGS) -c -o $(BUILD)sched.o ./os/kernel/sched.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)process.o ./os/kernel/process.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)procsyscall.o ./os/kernel/procsyscall.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)syscall.o ./os/kernel/syscall.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)cpu.o ./os/kernel/cpu.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)page.o ./os/kernel/page.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)slob.o ./os/kernel/slob.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)kmalloc.o ./os/kernel/kmalloc.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)vmm.o ./os/kernel/vmm.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)osmain.o ./os/kernel/osmain.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)spinlock.o ./os/kernel/spinlock.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)sem.o ./os/kernel/sem.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)panic.o ./os/kernel/panic.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)printk.o ./os/kernel/printk.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)console.o ./os/kernel/console.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)string.o ./os/lib/string.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)plic.o ./os/driver/plic.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)diskio.o ./os/fatfs/diskio.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)fatfs_drv.o ./os/fatfs/fatfs_drv.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)fatfs_vfs.o ./os/fatfs/fatfs_vfs.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)ff.o ./os/fatfs/ff.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)ffsystem.o ./os/fatfs/ffsystem.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)ffunicode.o ./os/fatfs/ffunicode.c


	$(CC) $(KCFLAGS) -c -o $(BUILD)vfs_device.o ./os/vfs/vfs_device.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)vfs_file.o ./os/vfs/vfs_file.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)vfs_fs.o ./os/vfs/vfs_fs.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)vfs_inode.o ./os/vfs/vfs_inode.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)vfs.o ./os/vfs/vfs.c

	$(CC) $(KCFLAGS) -c -o $(BUILD)hal_sd.o ./os/hal/hal_sd.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)fs.o ./os/kernel/fs.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)pipe.o ./os/kernel/pipe.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)cfgfs.o ./os/kernel/cfgfs.c

k210: buildos
	$(CC) $(KCFLAGS) -c -o $(BUILD)fpioa.o ./os/driver/fpioa.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)gpiohs.o ./os/driver/gpiohs.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)spi.o ./os/driver/spi.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)utils.o ./os/driver/utils.c
	$(CC) $(KCFLAGS) -c -o $(BUILD)sdcard.o ./os/driver/sdcard.c

	$(CC) $(KLFLAGS) -o $(BUILD)kernel $(BUILD)boot.o \
									   $(BUILD)intr_s.o \
									   $(BUILD)intr.o \
									   $(BUILD)timer.o \
									   $(BUILD)switch.o \
									   $(BUILD)sched.o \
									   $(BUILD)process.o \
									   $(BUILD)procsyscall.o \
									   $(BUILD)syscall.o \
									   $(BUILD)cpu.o \
									   $(BUILD)page.o \
									   $(BUILD)slob.o \
									   $(BUILD)kmalloc.o \
									   $(BUILD)vmm.o \
									   $(BUILD)osmain.o \
									   $(BUILD)spinlock.o \
									   $(BUILD)sem.o \
									   $(BUILD)panic.o \
									   $(BUILD)printk.o \
									   $(BUILD)console.o \
									   $(BUILD)plic.o \
									   $(BUILD)string.o \
									   $(BUILD)fpioa.o \
									   $(BUILD)gpiohs.o \
									   $(BUILD)spi.o \
									   $(BUILD)utils.o \
									   $(BUILD)sdcard.o \
									   \
									   \
									   $(BUILD)diskio.o \
									   $(BUILD)fatfs_drv.o \
									   $(BUILD)fatfs_vfs.o \
									   $(BUILD)ff.o \
									   $(BUILD)ffsystem.o \
									   $(BUILD)ffunicode.o \
									   \
									   $(BUILD)vfs_device.o \
									   $(BUILD)vfs_file.o \
									   $(BUILD)vfs_fs.o \
									   $(BUILD)vfs_inode.o \
									   $(BUILD)vfs.o \
									   \
									   $(BUILD)hal_sd.o \
									   $(BUILD)fs.o \
									   $(BUILD)pipe.o \
									   $(BUILD)cfgfs.o
									   
									   
	$(OBJCOPY) $(BUILD)kernel --strip-all -O binary $(BUILD)kernel.bin
	$(NM) $(BUILD)/kernel | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)'| sort > ./kernel.map
	cp ./sbi/k210/rustsbi-k210.bin ./build/k210.bin
	dd if=./build/kernel.bin of=./build/k210.bin bs=128k seek=1

# build fs img
# dd if=/dev/zero of=disk.img bs=3m count=1024
# mkfs.vfat -F 32 disk.img
mount:
	sudo mount -o loop ./disk.img  /mnt/vdisk
umount:
	sudo umount /mnt/vdisk

qemu: buildos builduser
	$(CC) $(KCFLAGS) -c -o $(BUILD)vdisk.o ./os/driver/vdisk.c

	$(CC) $(KLFLAGS) -o $(BUILD)kernel $(BUILD)boot.o \
									   $(BUILD)intr_s.o \
									   $(BUILD)intr.o \
									   $(BUILD)timer.o \
									   $(BUILD)switch.o \
									   $(BUILD)sched.o \
									   $(BUILD)process.o \
									   $(BUILD)procsyscall.o \
									   $(BUILD)syscall.o \
									   $(BUILD)cpu.o \
									   $(BUILD)page.o \
									   $(BUILD)slob.o \
									   $(BUILD)kmalloc.o \
									   $(BUILD)vmm.o \
									   $(BUILD)osmain.o \
									   $(BUILD)spinlock.o \
									   $(BUILD)sem.o \
									   $(BUILD)panic.o \
									   $(BUILD)printk.o \
									   $(BUILD)console.o \
									   $(BUILD)plic.o \
									   $(BUILD)string.o \
									   \
									   $(BUILD)vdisk.o \
									   \
									   \
									   $(BUILD)diskio.o \
									   $(BUILD)fatfs_drv.o \
									   $(BUILD)fatfs_vfs.o \
									   $(BUILD)ff.o \
									   $(BUILD)ffsystem.o \
									   $(BUILD)ffunicode.o \
									   \
									   $(BUILD)vfs_device.o \
									   $(BUILD)vfs_file.o \
									   $(BUILD)vfs_fs.o \
									   $(BUILD)vfs_inode.o \
									   $(BUILD)vfs.o \
									   \
									   $(BUILD)hal_sd.o \
									   $(BUILD)fs.o \
									   $(BUILD)pipe.o \
									   $(BUILD)cfgfs.o
									   
	$(OBJCOPY) $(BUILD)kernel --strip-all -O binary $(BUILD)kernel.bin
	$(NM) $(BUILD)/kernel | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)'| sort > ./kernel.map
	-sudo umount /mnt/vdisk
	rm -f ./disk.img
	dd if=/dev/zero of=disk.img bs=1024k count=64
	mkfs.vfat -F 32 disk.img
	sudo mount -o loop ./disk.img  /mnt/vdisk
	sudo cp -r /home/calvin/vdisk/* /mnt/vdisk/
	-sudo umount /mnt/vdisk
	qemu-system-riscv64 -machine virt \
				-cpu rv64 \
				-smp 2 \
				-m 64M \
				-nographic \
				-bios default \
				-kernel $(BUILD)kernel.bin \
				-drive file=disk.img,if=none,format=raw,id=x0 \
				-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
				-device loader,file=$(BUILD)kernel.bin,addr=0x80200000

UBUILD = $(BUILD)user/
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
	cp $(UBUILD)app /home/calvin/vdisk/app
	cp $(UBUILD)app.bin $(WSL_WINPATH)app.bin
	powershell.exe $(WINPATH)runbin.bat app

run: k210
	cp ./build/k210.bin $(WSL_WINPATH)k210.bin
	powershell.exe $(WINPATH)run.bat

qemu-k210: k210
	-sudo umount /mnt/vdisk
	rm -f ./disk.img
	dd if=/dev/zero of=disk.img bs=1024k count=64
	mkfs.vfat -F 32 disk.img
	sudo mount -o loop ./disk.img  /mnt/vdisk
	sudo cp -r /home/calvin/vdisk/* /mnt/vdisk/
	-sudo umount /mnt/vdisk
	/opt/riscv64/qemu-k210/bin/qemu-system-riscv64 -machine virt \
				-cpu rv64 \
				-smp 2 \
				-m 8M \
				-nographic \
				-bios ./sbi/k210/rustsbi-k210.bin \
				-kernel $(BUILD)kernel.bin \
				-drive file=disk.img,if=none,format=raw,id=x0 \
				-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
				-device loader,file=$(BUILD)kernel.bin,addr=0x80200000
	
clean:
	rm -f ./build/*.o
	rm -f ./build/user/*.o
cleanx:
	rm -f ./build/*.o
	rm -f ./build/*.bin
	rm -f ./build/user/*.o
	rm -f ./build/user/*.bin
	rm -f ./build/kernel
	rm -f ./build/user/app

