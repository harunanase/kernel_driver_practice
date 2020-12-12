#ifndef __CDEVICE_H__
#define __CDEVICE_H__

#include <linux/cdev.h>
#include <linux/ioctl.h>

#ifndef HARUDEV_MAJOR
#define HARUDEV_MAJOR 0   /* dynamic major by default */
#endif

#define NODE_DATA_SIZE 512

struct node {
	void *data;
	struct node *next;
};


struct haruDev {	
	int size;
	struct node *head;
	struct cdev cdev;
};



#define HARUDEV_IOC_MAGIC 'h'

#define HARUDEV_IOCRESET _IO(HARUDEV_IOC_MAGIC, 0)
#define HARUDEV_IOCGSIZE _IOR(HARUDEV_IOC_MAGIC, 1, int)

#define HARUDEV_IOC_MAXNR 1
#endif
