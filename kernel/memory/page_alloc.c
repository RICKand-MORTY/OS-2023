#include <memory.h>
#include "../../lib/printk.h"



unsigned long pages_bitmaps[BITMAPS_SIZE];
unsigned long phy_start_address = 0;
unsigned long global_free_pages = 0;
unsigned long global_total_pages = 0;

void mem_init(u64 mem_start, u64 mem_end)
{
    u64 free_memory=0;
    int i=0;

    for(int i=0;i<BITMAPS_SIZE;i++)
    {
        pages_bitmaps[i]=0;
    }

    mem_start = PAGE_ALIGN_UP(mem_start);
    phy_start_address = mem_start;
    mem_end &= PAGE_MASK;
    free_memory = mem_start - mem_end;
    while (mem_start < mem_end)
    {
        global_free_pages++;
        mem_start += PAGE_SIZE;
    }
    global_total_pages = global_free_pages;
    printk("%lu Byte memory available, %lu free pages, global total pages:%lu!\n", free_memory, global_free_pages, TOTAL_PAGES);
}

/*alloc a page,if not free page,return -1,else return page address*/
unsigned long page_alloc()
{
    unsigned long i=1;
    int index = 0;
    int offset=0;
    if(global_free_pages == 0)
    {
        return 1;
    }
    //printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
    while (pages_bitmaps[index] == 0xffffffffffffffff)
    {
        index++;
    }
    while(pages_bitmaps[index] & i)
    {
        offset++;
        i = i << 1;
    }
    pages_bitmaps[index] |= i; 
    global_free_pages--;
    //printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
    return (phy_start_address + index * sizeof(u64) * 8 + offset * PAGE_SIZE);
}

//连续分配多页内存
unsigned long more_page_alloc(int count)
{
    unsigned long i=1;
    int num = 0;
    int begin = 0;
    int index = 0;
    int offset=0;
    if(count <= 0) 
    {
        return 1; 
    }
    if(global_free_pages < count) 
    {
        return 1; 
    }
    //printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
    while (pages_bitmaps[index] == 0xffffffffffffffff) 
    {
        index++;
    }
    while(pages_bitmaps[index] & i) 
    {
        offset++;
        i = i << 1;
    }
    
    begin = index * sizeof(u64) * 8 + offset; 
    num = 1; 
    
    while(num < count) // 循环直到找到连续的count个页面或者到达位图末尾
    {
        offset++;
        i = i << 1;
        if(offset == sizeof(u64) * 8) 
        {
            index++;
            offset = 0;
            i = 1;
            if(index >= sizeof(pages_bitmaps) / sizeof(u64)) 
            {
                break; 
            }
        }
        if(pages_bitmaps[index] & i) 
        {
            begin = index * sizeof(u64) * 8 + offset; 
            num = 1; 
        }
        else 
        {
            num++;
        }
    }

    if(num == count) 
    {
        index = begin / (sizeof(u64) * 8); 
        offset = begin % (sizeof(u64) * 8); 
        i = 1 << offset; 
        for(num = 0; num < count; num++) 
        {
            pages_bitmaps[index] |= i; 
            offset++;
            i = i << 1;
            if(offset == sizeof(u64) * 8) 
            {
                index++;
                offset = 0;
                i = 1;
            }
        }
        global_free_pages -= count; 
        //printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
        return (phy_start_address + begin * PAGE_SIZE); 
    }
    else 
    {
        return 1;
    }
}

int more_page_free(void *buf, unsigned int count)
{
    unsigned long i=1;
    int num = 0;
    int begin = 0;
    int index = 0;
    int offset=0;
    if(count <= 0) 
    {
        return -1; 
    }
    if(buf == NULL) 
    {
        return -1; 
    }
    if((unsigned long)buf < phy_start_address || (unsigned long)buf >= phy_start_address + sizeof(pages_bitmaps) / sizeof(u64) * sizeof(u64) * 8 * PAGE_SIZE) // 检查buf是否在物理内存范围内
    {
        //不在有效范围
        printk("more_page_free: bad address!\n");
        return -1; 
    }
    if((unsigned long)buf % PAGE_SIZE != 0) 
    {
        // 检查buf是否对齐到页面边界
        printk("more_page_free: address not align!\n");
        return -1; 
    }
    
    begin = ((unsigned long)buf - phy_start_address) / PAGE_SIZE; 
    index = begin / (sizeof(u64) * 8); 
    offset = begin % (sizeof(u64) * 8); 
    i = 1 << offset; 
    
    //释放前先检查
    for(num = 0; num < count; num++) 
    {
        if((pages_bitmaps[index] & i) == 0) 
        {
            // 如果当前页面对应的位图为0，说明未分配，返回错误
            printk("more_page_free: pages not be alloced!\n");
            return -1; 
        }
        offset++;
        i = i << 1;
        if(offset == sizeof(u64) * 8) 
        {
            index++;
            offset = 0;
            i = 1;
            if(index >= sizeof(pages_bitmaps) / sizeof(u64)) 
            {
                // 如果所有位图都已经处理完毕，说明超出范围，返回错误
                printk("more_page_free: out of limit!\n");
                return -1; 
            }
        }
    }

    index = begin / (sizeof(u64) * 8); 
    offset = begin % (sizeof(u64) * 8); 
    i = 1 << offset; 
    
    for(num = 0; num < count; num++) 
    {
        pages_bitmaps[index] &= ~i; 
        offset++;
        i = i << 1;
        if(offset == sizeof(u64) * 8) 
        {
            index++;
            offset = 0;
            i = 1;
        }
    }
    
    global_free_pages += count; 
    
    return 0; 
}


int page_free(unsigned int count)
{
    if((global_total_pages-global_free_pages)<count)
    {
        return 1;
    }
    unsigned long i = 1000000000000000;
    int index = 0;
    while (pages_bitmaps[index] == 0xffffffffffffffff)
    {
        index++;
    }
    if(pages_bitmaps[index] == 0)
    {
        index--;
    }
    while(!(pages_bitmaps[index] & i))
    {
        i = i >> 1;
    }
    loop:
    while(count>0)
    {
        pages_bitmaps[index] &= ~i; 
        i = i >> 1;
        count--;
        global_free_pages++;
        if(i == 0 && count >0)
        {
            index--;
        }
    }
    //printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
    return 0;
}

int page_free_addr(unsigned long addr)
{
    if(addr>=ADDR_END)
    {
        return -1;
    }
    int page_index = (addr - phy_start_address) / PAGE_SIZE;
    int bitmap_index = page_index / (sizeof(u64) * 8);
    int offset = page_index % (sizeof(u64) * 8);
    int i = 1 << (offset);
    pages_bitmaps[bitmap_index] &= ~i;
    //printk("pages_bitmaps[index]:%016lx,index=%d,offset=%d\n",pages_bitmaps[bitmap_index],bitmap_index, offset);
    //printk("phsy addr = %016lx",phy_start_address);
}