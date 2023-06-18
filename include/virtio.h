#ifndef _VIRTIO_H
#define _VIRTIO_H


#include "spinlock.h"
#include "buf.h"

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
//#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
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
#define QUEUE_SIZE				8
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



//virtio_mmio device
typedef volatile struct {
volatile	uint32_t MagicValue;
volatile	uint32_t Version;
volatile	uint32_t DeviceID;
volatile	uint32_t VendorID;
volatile	uint32_t DeviceFeatures;
volatile	uint32_t DeviceFeaturesSel;
volatile	uint32_t _reserved0[2];
volatile	uint32_t DriverFeatures;
volatile	uint32_t DriverFeaturesSel;
volatile	uint32_t _reserved1[2];
volatile	uint32_t QueueSel;
volatile	uint32_t QueueNumMax;
volatile	uint32_t QueueNum;
volatile	uint32_t _reserved2[2];
volatile	uint32_t QueueReady;
volatile	uint32_t _reserved3[2];
volatile	uint32_t QueueNotify;
volatile	uint32_t _reserved4[3];
volatile	uint32_t InterruptStatus;
volatile	uint32_t InterruptACK;
volatile	uint32_t _reserved5[2];
volatile	uint32_t Status;
volatile	uint32_t _reserved6[3];
volatile	uint32_t QueueDescLow;
volatile	uint32_t QueueDescHigh;
volatile	uint32_t _reserved7[2];
volatile	uint32_t QueueAvailLow;
volatile	uint32_t QueueAvailHigh;
volatile	uint32_t _reserved8[2];
volatile	uint32_t QueueUsedLow;
volatile	uint32_t QueueUsedHigh;
volatile	uint32_t _reserved9[21];
volatile	uint32_t ConfigGeneration;
volatile	uint32_t Config[0];
} virtio_regs;

//legacy device
typedef volatile struct{
volatile	__virtio32 MagicValue;
volatile	__virtio32 Version;
volatile	__virtio32 DeviceID;
volatile	__virtio32 VendorID;
volatile    __virtio32 HostFeatures;
volatile    __virtio32 HostFeaturesSel;
volatile    __virtio32 _reserved0[2];
volatile    __virtio32 GuestFeatures;
volatile    __virtio32 GuestFeaturesSel;
volatile    __virtio32 GuestPageSize;
volatile    __virtio32 _reserved1[1];
volatile    __virtio32 QueueSel;
volatile    __virtio32 QueueNumMax;
volatile    __virtio32 QueueNum;
volatile    __virtio32 QueueAlign;
volatile    __virtio32 QueuePFN;
volatile	__virtio32 _reserved2[3];
volatile    __virtio32 QueueNotify;
volatile    __virtio32 _reserved3[3];
volatile    __virtio32 InterruptStatus;
volatile    __virtio32 InterruptACK;
volatile    __virtio32 _reserved4[2];
volatile    __virtio32 Status;
volatile    __virtio32 _reserved5[35];
volatile	__virtio32 Config[0];
}virtio_regs_legacy;

volatile virtio_regs_legacy *g_regs;

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
void virtio_disk_rw(Buf *b, int write);
//int virtio_rw(uint64_t sector, uint8_t *buffer, int write);
#endif