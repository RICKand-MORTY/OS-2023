广东工业大学番禺校区--接下来我要表扬一款游戏

### 主要参考

我们的OS主要参考了笨叔编写的《RISC-V体系结构编程与实践》一书

其他参考：

《一个64位操作系统的设计与实现》--田宇著

riscv-xv6

### 使用说明

make all:编译

make run:启动QEMU

make clean:清除所有生成的文件

make debug:启动QEMU和GDB调试

### 完成情况

由于时间所限，我们的OS只实现了简单的中断处理、内存管理、进程调度功能和小部分的文件系统以及几个系统调用,由于最后未能成功修复bug，因此我们的virtio驱动仍然存在问题,无法使用，且文件系统完成度太低，因此文件系统未给出文档,请见谅

### 成员分工

曲为楷负责代码的开发，梁斯俊和贺畅负责bug的调试和文档的编写

### 目录结构

```bash
.vscode:vscode相关
bootloader:opensbi生成的sbi固件，由于使用了QEMU默认的sbi固件，因此该文件夹内的opensbi固件未使用
doc:文档说明
include:头文件
	LinkList.h:链表操作
	buf.h:与文件系统相关，用于读写虚拟磁盘的缓冲区
	csr.h:控制寄存器相关
	fs.h:与文件系统相关,由于未完成，目前内容仅有超级块的定义
	io.h:与读取写入相关
	log.h:文件系统相关，磁盘日志
	memory.h:mmu和内存管理相关
	page_table.h:页表相关的宏定义
	plic.h:plic相关
	process.h:进程管理相关
	pt_offset.h:与上下文现场保存相关
	sbi.h:S模式下系统调用opensbi固件接口
	scheduler.h:进程调度相关
	sleeplock.h:睡眠锁
	syscall.h:系统调用
	timer.h:定时器
	trap.h:中断和异常
	uart.h:16550串口控制器
	virtio,h:virtio设备驱动
kernel:内核文件夹
	driver:virtio驱动，存在问题
	fs:文件系统相关
	memeory:内存管理和mmu映射
	process:进程管理
	trap:中断和异常
	boot.S:opensbi引导到跳转至内核
	entry.S:汇编相关定义
	linker.ld:链接脚本
	main.c:内核主函数在内
lib:通用库，包含输出和基本函数
usr:与U模式相关
```

### 总结

由于时间所限，加上我们水平有限，所以没有在规定时间内完成作品，不过也在这段时间内学到了很多东西，学校的操作系统课注重理论，而比赛让我们从实践的层面对操作系统有了更深刻的认识，在编写作品期间也学到了不少提高开发效率的方法，比如从最开始使用命令行的gdb调试到后面学会使用vscode连接gdb调试，大大提高了我们debug的效率。没能够完成这个OS是我们觉得比较遗憾的地方，但是从中学到的东西也算是自我的提升。

