typedef struct _stackframe
{
    //fp and ra register in stack
    unsigned long s_fp;
    unsigned long s_ra;
}stackframe;
extern char _text[], _etext[];

static int kernel_text(unsigned long addr)
{
    //judge if address in kernel text section
    if(addr >= (unsigned long )_text && addr<=(unsigned long)_etext)
    {
        return 1;
    }
    return 0;
}

static void search_stackframe()
{
    volatile unsigned long sp=0;
    unsigned long fp=0,pc=0;
    stackframe *frame;
    unsigned long tmp=0;
    //get sp
    __asm__ volatile
    (
        "addi %0,x2,0"
        :"+r"(sp)
        :
        :"memory"
    );
    pc=(unsigned long)search_stackframe;
    fp = (unsigned long)__builtin_frame_address(0);
    while (1)
    {
        if(kernel_text(pc)==0)
        {
            break;
        }
        tmp=sp+sizeof(stackframe);
        if((fp<tmp||fp &0xf))
        {
            break;
        }
        frame=(stackframe*)(fp-16);
        sp=fp;
        fp=frame->s_fp;
        pc = frame->s_ra-4;
        if (kernel_text(pc))
			printk("[0x%016lx - 0x%016lx]  pc 0x%016lx\n", sp, fp, pc);
    }
}

void stack_trace(void)
{
    printk("Call Frame:\n");
    search_stackframe();
}