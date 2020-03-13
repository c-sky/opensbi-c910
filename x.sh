make clean
make distclean
PLATFORM=thead/c910 \
CROSS_COMPILE=~/nixon/bin/riscv64-unknown-linux-gnu- \
FW_TEXT_START=0xc0000000 \
FW_JUMP_ADDR=0xc0200000 \
PLIC_BASE_ADDR=0x20000000 \
FW_PAYLOAD=y \
FW_PAYLOAD_OFFSET=0x00100000 \
FW_PAYLOAD_PATH=~/u-boot_riscv/u-boot.bin \
make
~/nixon/bin/riscv64-unknown-linux-gnu-objdump -d build/platform/thead/c910/firmware/fw_jump.elf > 444
cp build/platform/thead/c910/firmware/fw_jump.elf \
~/buildroot/thead_910_compat_5.1_glibc_br_defconfig/images/hw/fw_jump_3G.elf
cp build/platform/thead/c910/firmware/fw_jump.bin \
~/buildroot/thead_910_compat_5.1_glibc_br_defconfig/images/hw/fw_jump_3G.bin

~/nixon/bin/riscv64-unknown-linux-gnu-objdump -d build/platform/thead/c910/firmware/fw_payload.elf > 123
cp build/platform/thead/c910/firmware/fw_payload.elf \
~/buildroot/thead_910_compat_5.1_glibc_br_defconfig/images/hw/fw_jump_3G.elf
cp build/platform/thead/c910/firmware/fw_payload.bin \
~/buildroot/thead_910_compat_5.1_glibc_br_defconfig/images/hw/fw_jump_3G.bin
