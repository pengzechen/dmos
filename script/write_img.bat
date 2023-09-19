dd if=boot.bin of=disk.img bs=512 conv=notrunc count=1
dd if=loader.bin of=disk.img bs=512 conv=notrunc seek=1
dd if=kernel.elf of=disk.img bs=512 conv=notrunc seek=100
dd if=shell.elf of=disk.img bs=512 conv=notrunc seek=5000