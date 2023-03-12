# OS-2023
交叉编译工具链:riscv64-unknown-elf-

https://github.com/riscv-collab/riscv-gnu-toolchain

虚拟机:qemu

https://github.com/qemu/qemu

开源固件:opensbi

使用：

make:编译os

make clean：清理编译生成文件

make run:启动qemu

make debug：启动qemu和gdb调试


目录结构：

/bootloader:开源固件opensbi构建

/include:头文件

/lib:通用库

/kernel:内核源文件

/kernel/exception:异常处理


