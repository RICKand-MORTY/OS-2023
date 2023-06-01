#ifndef _VIRTIO_H
#define _VIRTIO_H


#define VIRTIO_REGS_SIZE        		0x00001000
#define VIRTIO_MAGIC   					0x74726976UL
#define VIRTIO_VENDORID					0x554d4551UL
#define VIRTIO_VERSION_LEGACY 			0x1
#define VIRTIO_VERSION 					0x2
#define VIRTIO_DEV_NET 					0x1
#define VIRTIO_DEV_BLK 					0x2

//virtio_regs status
#define VIRTIO_STATUS_ACKNOWLEDGE        (1)
#define VIRTIO_STATUS_DRIVER             (2)
#define VIRTIO_STATUS_FAILED             (128)
#define VIRTIO_STATUS_FEATURES_OK        (8)
#define VIRTIO_STATUS_DRIVER_OK          (4)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (64)


// device feature bits
#define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29
#define VIRTIO_BLK_F_BARRIER 		(0)	//Device supports request barriers.
#define VIRTIO_BLK_F_SCSI 			(7) //Device supports scsi packet commands.


//request type
#define VIRTIO_BLK_T_IN 					0
#define VIRTIO_BLK_T_OUT 					1
#define VIRTIO_BLK_T_FLUSH 					4
#define VIRTIO_BLK_T_GET_ID 				8
#define VIRTIO_BLK_T_GET_LIFETIME 			10
#define VIRTIO_BLK_T_DISCARD 				11
#define VIRTIO_BLK_T_WRITE_ZEROES 			13
#define VIRTIO_BLK_T_SECURE_ERASE 			14

//vring
#define QUEUE_SIZE				1024
//desc flags
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 		1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 		2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 	4
#define VIRTQ_USED_F_NO_NOTIFY 	1
#define VIRTQ_USED_F_INDIRECT 	0x04

#define BIT(x) (1UL << (x))
#define VIRTIO_F_VERSION_1		((uint64_t) BIT(32))

//operation type
#define VIRTIO_BLK_T_IN 	0		//read
#define VIRTIO_BLK_T_OUT 	1		//write


typedef unsigned long long 	__virtio64;
typedef unsigned int 		__virtio32;
typedef unsigned short		__virtio16;

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;


// virtio mmio control registers, mapped starting at 0x10001000.
// from qemu virtio_mmio.h
#define VIRTIO0 					0x10001000
#define VIRTIO_MMIO_MAGIC_VALUE		0x000 // 0x74726976
#define VIRTIO_MMIO_VERSION		0x004 // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID		0x008 // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID		0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES	0x010
#define VIRTIO_MMIO_DRIVER_FEATURES	0x020
#define VIRTIO_MMIO_QUEUE_SEL		0x030 // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034 // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM		0x038 // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_READY		0x044 // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050 // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064 // write-only
#define VIRTIO_MMIO_STATUS		0x070 // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW	0x080 // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH	0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW	0x090 // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH	0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW	0x0a0 // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH	0x0a4

//virtio_mmio device
typedef volatile struct {
	uint32_t MagicValue;
	uint32_t Version;
	uint32_t DeviceID;
	uint32_t VendorID;
	uint32_t DeviceFeatures;
	uint32_t DeviceFeaturesSel;
	uint32_t _reserved0[2];
	uint32_t DriverFeatures;
	uint32_t DriverFeaturesSel;
	uint32_t _reserved1[2];
	uint32_t QueueSel;
	uint32_t QueueNumMax;
	uint32_t QueueNum;
	uint32_t _reserved2[2];
	uint32_t QueueReady;
	uint32_t _reserved3[2];
	uint32_t QueueNotify;
	uint32_t _reserved4[3];
	uint32_t InterruptStatus;
	uint32_t InterruptACK;
	uint32_t _reserved5[2];
	uint32_t Status;
	uint32_t _reserved6[3];
	uint32_t QueueDescLow;
	uint32_t QueueDescHigh;
	uint32_t _reserved7[2];
	uint32_t QueueAvailLow;
	uint32_t QueueAvailHigh;
	uint32_t _reserved8[2];
	uint32_t QueueUsedLow;
	uint32_t QueueUsedHigh;
	uint32_t _reserved9[21];
	uint32_t ConfigGeneration;
	uint32_t Config[0];
} virtio_regs;

//legacy device
typedef volatile struct {
	__virtio32 MagicValue;
	__virtio32 Version;
	__virtio32 DeviceID;
	__virtio32 VendorID;
    __virtio32 HostFeatures;
    __virtio32 HostFeaturesSel;
    __virtio32 _reserved0[2];
    __virtio32 GuestFeatures;
    __virtio32 GuestFeaturesSel;
    __virtio32 GuestPageSize;
    __virtio32 _reserved1[1];
    __virtio32 QueueSel;
    __virtio32 QueueNumMax;
    __virtio32 QueueNum;
    __virtio32 QueueAlign;
    __virtio32 QueuePFN;
	__virtio32 _reserved2[3];
    __virtio32 QueueNotify;
    __virtio32 _reserved3[3];
    __virtio32 InterruptStatus;
    __virtio32 InterruptACK;
    __virtio32 _reserved4[2];
    __virtio32 Status;
    __virtio32 _reserved5[35];
	__virtio32 Config[0];
}virtio_regs_legacy;

virtio_regs *g_regs;

//from qemu-7.0.0
/* Virtio ring descriptors: 16 bytes. These can chain together via "next". */
struct vring_desc {
	/* Address (guest-physical) */
	__virtio64 addr;
	/* Length */
	__virtio32 len;
	/* The flags as indicated above */
	__virtio16 flags;
	/* We chain unused descriptors via this, too */
	__virtio16 next;
};

struct vring_avail {
	__virtio16 flags;
	__virtio16 idx;
	__virtio16 ring[QUEUE_SIZE];	//index of desc
};

struct vring_used_elem {
	/* Index of start of used descriptor chain */
	__virtio32 id;
	/* Total length of the descriptor chain which was used (written to) */
	__virtio32 len;
};

struct vring_used {
	__virtio16 flags;
	__virtio16 idx;
	struct vring_used_elem ring[QUEUE_SIZE];
};

struct vring {
	unsigned int num;
	struct vring_desc *desc;
	struct vring_avail *avail;
	struct vring_used *used;
};

struct virtio_blk_req {
    // header
    __virtio32 type; // VIRTIO_BLK_T_IN for read
    __virtio32 reverse; 
    __virtio64 sector; // sector number
	uint8_t data[512];
	uint8_t status;
    // descriptor table
    struct vring_desc desc[3];
    // avail ring
    struct vring_avail avail;
    // used ring
    struct vring_used used;    
    } req;

extern struct vring g_vring;
void virtio_init();
int virtio_blk_rw(uint32_t type, uint32_t sector, uint8_t *data);
int virtio_rw(uint64_t sector, uint8_t *buffer, int write);
#endif