#ifndef __NDAS_FS_PUBLIC_H__
#define __NDAS_FS_PUBLIC_H__


#define FSCTL_NDAS_FS_UNLOAD		CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x3FCA, METHOD_NEITHER,  FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define FSCTL_NDAS_FS_PURGE			CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x3FDA, METHOD_BUFFERED, FILE_READ_ACCESS)
#define FSCTL_NDAS_FS_FLUSH			CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x3FDB, METHOD_BUFFERED, FILE_READ_ACCESS)

#endif


