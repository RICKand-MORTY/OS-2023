#include <memory.h>
#include "../../lib/printk.h"


#define BITMAPS_SIZE ((TOTAL_PAGES / (sizeof(u64) * 8)) + 1)

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
    printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
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
    printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
    return (phy_start_address + index * sizeof(u64) * 8 + offset * PAGE_SIZE);
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
    printk("pages_bitmaps[index]:%016lx,index=%d\n",pages_bitmaps[index],index);
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
    printk("pages_bitmaps[index]:%016lx,index=%d,offset=%d\n",pages_bitmaps[bitmap_index],bitmap_index, offset);
    printk("phsy addr = %016lx",phy_start_address);
}