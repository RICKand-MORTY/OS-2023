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

    csr.h：与系统寄存器有关宏定义\  
    io.h:  用于读写\
    pt_offset.h：保存现场时寄存器在栈中偏移\
    sbi.h: 用于m模式调用(ecall)\
    timer.h:与计时器中断有关\

/lib:通用库

    lib.h通用函数库\
    printk.c和printk.c：打印\

/kernel:内核源文件

    trap:异常和中断处理

	    stacktrace.c:栈回溯\
	    timer.c:定时器\
	    trap.c异常处理\
    
/kernel/exception:异常处理


