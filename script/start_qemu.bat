start qemu-system-i386  -m 512M -s -S -serial stdio -drive file=disk.img,index=0,media=disk,format=raw -d pcall,page,mmu,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation
