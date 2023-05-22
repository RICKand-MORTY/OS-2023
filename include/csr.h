#ifndef _CSR_H
#define _CSR_H


//sie register
#define SIE_SSIE 0X02UL     //software interrupt
#define SIE_STIE 0x20UL     //timer interrupt
#define SIE_SEIE 0x200UL    //external interrupt

//sstatus register 
#define SSTATUS_SIE 0X02UL  //enable S mode interrupt
#define SSTATUS_SPIE 0x20UL      //previous interrupt status
#define SSTATUS_SPP 0x100UL      //Mode before interrupt

#define local_irq_enable() 	csr_set(sstatus, SSTATUS_SIE)      //enable SSTATUS_SIE (enable interrupt on S mode)
#define local_irq_disable() csr_clr(sstatus, SSTATUS_SIE)     //disable SSTATUS_SIE


#define write_csr(csr,val)						\
({												\
	__asm__ __volatile__  (						\
		"csrw "#csr", %0"						\
		:										\
		:"rK"((unsigned long)(val))				\
		:"memory"								\
	);											\
})

#define read_csr(csr)							\
({												\
	register unsigned long _res;				\
	__asm__ __volatile__(						\
		"csrr %0, " #csr						\
		:"=r"(_res)								\
		:										\
		:"memory"								\
	);											\
	_res;										\	
})

#define csr_set(csr,val)						\
({												\
	__asm__ __volatile__(						\
		"csrs " #csr ", %0"						\
		:										\
		:"rK"(val)								\
		:"memory"								\
	);											\
})

#define csr_clr(csr,val)						\
({												\
	__asm__ __volatile__(						\
		"csrc " #csr ", %0"						\
		:										\
		:"rK"(val)								\
		:"memory"								\
	);											\
})
#endif