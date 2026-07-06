#ifndef _UAPI_HWBUTTON_H
#define _UAPI_HWBUTTON_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define HWBUTTON_MAGIC 'H'

#define HWBUTTON_IOC_GET_VALUE _IOR(HWBUTTON_MAGIC, 1, __u32)
#define HWBUTTON_IOC_TRIGGER _IO(HWBUTTON_MAGIC, 2)

#endif
