#include "XixFsType.h"
#include "XixFsErrorInfo.h"
#include "XixFsDebug.h"
#include "XixFsAddrTrans.h"
#include "XixFsDiskForm.h"
#include "XixFsDrv.h"
#include "XixFsRawDiskAccessApi.h"
#include "XixFsProto.h"



#include "ndasscsiioctl.h"
#include "lurdesc.h"

#define NTSTRSAFE_LIB
#include <ntddscsi.h>

#include <Ntstrsafe.h>





#ifndef RtlInitEmptyUnicodeString

#define RtlInitEmptyUnicodeString(_ucStr,_buf,_bufSize) \
		((_ucStr)->Buffer = (_buf), \
		(_ucStr)->Length = 0, \
		(_ucStr)->MaximumLength = (USHORT)(_bufSize))

#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, XixFsGetScsiportAdapter)
#pragma alloc_text(PAGE, XixFsCheckXifsd)
#pragma alloc_text(PAGE, XixFsNdasLock)
#pragma alloc_text(PAGE, XixFsNdasUnLock)
#pragma alloc_text(PAGE, XixFsNdasQueryLock)
#endif



/*
 *	Function must be done within waitable thread context
 */


BOOLEAN	
XixFsGetScsiportAdapter(
  	IN	PDEVICE_OBJECT				DiskDeviceObject,
  	IN	PDEVICE_OBJECT				*ScsiportAdapterDeviceObject
	) 
{
	SCSI_ADDRESS		ScsiAddress;
	NTSTATUS			ntStatus;
	UNICODE_STRING		ScsiportAdapterName;
	WCHAR				ScsiportAdapterNameBuffer[32];
	WCHAR				ScsiportAdapterNameTemp[32]	= L"";
    OBJECT_ATTRIBUTES	objectAttributes;
    HANDLE				fileHandle					= NULL;
	IO_STATUS_BLOCK		IoStatus;
	PFILE_OBJECT		ScsiportDeviceFileObject	= NULL;



	ntStatus = XixFsRawDevIoCtrl ( 
					DiskDeviceObject,
					IOCTL_SCSI_GET_ADDRESS,
					NULL,
					0,
					(uint8 *)&ScsiAddress,
					sizeof(SCSI_ADDRESS),
					FALSE,
					NULL
					);

	if(!NT_SUCCESS(ntStatus)) {
		DebugTrace( DEBUG_LEVEL_ERROR,  (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
			("XixFsGetScsiportAdapter: LfsFilterDeviceIoControl() failed.\n"));
		goto error_out;

	}

    DebugTrace( DEBUG_LEVEL_ALL,  (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("XixFsGetScsiportAdapter: ScsiAddress=Len:%d PortNumber:%d PathId:%d TargetId:%d Lun:%d\n",
						(LONG)ScsiAddress.Length,
						(LONG)ScsiAddress.PortNumber,
						(LONG)ScsiAddress.PathId,
						(LONG)ScsiAddress.TargetId,
						(LONG)ScsiAddress.Lun
						));

	RtlStringCchPrintfW(ScsiportAdapterNameTemp,
						sizeof(ScsiportAdapterNameTemp) / sizeof(ScsiportAdapterNameTemp[0]),
						L"\\Device\\ScsiPort%d",
						ScsiAddress.PortNumber);


	RtlInitEmptyUnicodeString( &ScsiportAdapterName, ScsiportAdapterNameBuffer, sizeof( ScsiportAdapterNameBuffer ) );

    RtlAppendUnicodeToString( &ScsiportAdapterName, ScsiportAdapterNameTemp );

	InitializeObjectAttributes( &objectAttributes,
							&ScsiportAdapterName,
							OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
							NULL,
							NULL);

    //
	// open the file object for the given device
	//
    ntStatus = ZwCreateFile( &fileHandle,
						   SYNCHRONIZE|FILE_READ_DATA,
						   &objectAttributes,
						   &IoStatus,
						   NULL,
						   0,
						   FILE_SHARE_READ | FILE_SHARE_WRITE,
						   FILE_OPEN,
						   FILE_SYNCHRONOUS_IO_NONALERT,
						   NULL,
						   0);
    if (!NT_SUCCESS( ntStatus )) {
	    DebugTrace( DEBUG_LEVEL_ERROR,  (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB),
			("XixFsGetScsiportAdapter: ZwCreateFile() failed.\n"));
		goto error_out;

	}

    ntStatus = ObReferenceObjectByHandle( fileHandle,
										FILE_READ_DATA,
										*IoFileObjectType,
										KernelMode,
										&ScsiportDeviceFileObject,
										NULL);
    if(!NT_SUCCESS( ntStatus )) {

		DebugTrace( DEBUG_LEVEL_ERROR,  (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
			("XixFsGetScsiportAdapter: ObReferenceObjectByHandle() failed.\n"));
        goto error_out;
    }

	*ScsiportAdapterDeviceObject = IoGetRelatedDeviceObject(
											ScsiportDeviceFileObject
									    );

	if(*ScsiportAdapterDeviceObject == NULL) {

		DebugTrace( DEBUG_LEVEL_ERROR,  (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
			("XixFsGetScsiportAdapter: IoGetRelatedDeviceObject() failed.\n"));
		ObDereferenceObject(ScsiportDeviceFileObject);
        goto error_out;
	}

	ObDereferenceObject(ScsiportDeviceFileObject);
	ZwClose(fileHandle);
	ObReferenceObject(*ScsiportAdapterDeviceObject);

	return TRUE;

error_out:
	
	*ScsiportAdapterDeviceObject = NULL;
	if(fileHandle)
		ZwClose(fileHandle);

	return FALSE;
}

	//	Added by ILGU HONG for 08312006
NTSTATUS
XixFsGetNdadDiskInfo(
	IN PDEVICE_OBJECT	DeviceObject,
	OUT	PNDSCIOCTL_PRIMUNITDISKINFO	PrimUnitDisk
	)
{
	NTSTATUS		RC = STATUS_SUCCESS;
	PDEVICE_OBJECT		scsiAdpaterDeviceObject = NULL;
	BOOLEAN				result = FALSE;
	PSRB_IO_CONTROL		pSrbIoCtl = NULL;
	uint32				OutbuffSize = 0;
	PNDASSCSI_QUERY_INFO_DATA		QueryInfo;
	PNDSCIOCTL_PRIMUNITDISKINFO	pPrimUnitDisk;

	PAGED_CODE();

	ASSERT(DeviceObject);

	
	DebugTrace(DEBUG_LEVEL_ALL, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsGetNdadDeviceInfo \n"));

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail GetScsiportAdpater!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}

	OutbuffSize = sizeof(SRB_IO_CONTROL) + sizeof(NDASSCSI_QUERY_INFO_DATA) + sizeof(NDSCIOCTL_PRIMUNITDISKINFO);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePoolWithTag(NonPagedPool, OutbuffSize, TAG_BUFFER);
	
	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_QUERYINFO_EX;
		pSrbIoCtl->Length =  sizeof(NDASSCSI_QUERY_INFO_DATA) + sizeof(NDSCIOCTL_PRIMUNITDISKINFO);
		
		QueryInfo = (PNDASSCSI_QUERY_INFO_DATA)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		pPrimUnitDisk = (PNDSCIOCTL_PRIMUNITDISKINFO)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));

		QueryInfo->Length = sizeof(NDASSCSI_QUERY_INFO_DATA);
		QueryInfo->InfoClass = NdscPrimaryUnitDiskInformation;
		QueryInfo->QueryDataLength = 0;




		//Fist step Send Disk 
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(NT_SUCCESS(RC)){

			if(pPrimUnitDisk->Length == sizeof(NDSCIOCTL_PRIMUNITDISKINFO)) {
				RtlCopyMemory(PrimUnitDisk, pPrimUnitDisk, sizeof(NDSCIOCTL_PRIMUNITDISKINFO));
			}

			DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
				("Success XixFsGetNdadDeviceInfo Status (0x%x)\n", RC));
		}else{	
			// Send port 
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(!NT_SUCCESS(RC)){
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Fail XixFsGetNdadDeviceInfo Status (0x%x)\n", RC));
			}else{

				if(pPrimUnitDisk->Length == sizeof(NDSCIOCTL_PRIMUNITDISKINFO)) {
					RtlCopyMemory(PrimUnitDisk, pPrimUnitDisk, sizeof(NDSCIOCTL_PRIMUNITDISKINFO));
				}

				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Success XixFsGetNdadDeviceInfo Status (0x%x)\n", RC));
			}

		}




	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);

	}

	DebugTrace(DEBUG_LEVEL_ALL, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Exit XixFsGetNdadDeviceInfo Status (0x%x)\n", RC));

	return RC;

}




NTSTATUS
XixFsAddUserBacl(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PNDAS_BLOCK_ACE	NdasBace,
	OUT PBLOCKACE_ID	BlockAceId
){
	NTSTATUS		RC = STATUS_SUCCESS;
	BOOLEAN			result;
	PSRB_IO_CONTROL	pSrbIoCtl;
	int				OutbuffSize;
	PNDSCIOCTL_ADD_USERBACL	addNdasBace;
	PBLOCKACE_ID	blockAceId;
	PDEVICE_OBJECT	scsiAdpaterDeviceObject;

	PAGED_CODE();

	ASSERT(DeviceObject);	


	
	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixFsAddUserBacl!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}	
	

	OutbuffSize = sizeof(SRB_IO_CONTROL) + sizeof(NDASSCSI_QUERY_INFO_DATA) + sizeof(NDSCIOCTL_ADD_USERBACL);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePool(NonPagedPool , OutbuffSize);

	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}	


	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_ADD_USERBACL;
		pSrbIoCtl->Length = sizeof(NDSCIOCTL_ADD_USERBACL);

		addNdasBace = (PNDSCIOCTL_ADD_USERBACL)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		blockAceId = (PBLOCKACE_ID)&addNdasBace->NdasBlockAce.BlockAceId;
		RtlCopyMemory(&addNdasBace->NdasBlockAce, NdasBace, sizeof(NDAS_BLOCK_ACE));

		
		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(NT_SUCCESS(RC)) {
			*BlockAceId = *blockAceId;
		} else {
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(NT_SUCCESS(RC)) {
				*BlockAceId = *blockAceId;
			} else {
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
				("Fail XixFsAddUserBacl Not NetDisk. status = %x\n", RC));
			}			
		}

	


	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);
	}

	return RC;
}




NTSTATUS
XixFsRemoveUserBacl(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	BLOCKACE_ID		BlockAceId
){
	NTSTATUS		RC = STATUS_SUCCESS;
	BOOLEAN			result;
	PSRB_IO_CONTROL	pSrbIoCtl;
	int				OutbuffSize;
	PNDSCIOCTL_REMOVE_USERBACL	removeNdasBace;
	PDEVICE_OBJECT	scsiAdpaterDeviceObject;

	PAGED_CODE();

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixFsRemoveUserBacl!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}	

	OutbuffSize = sizeof(SRB_IO_CONTROL) + sizeof(NDASSCSI_QUERY_INFO_DATA) + sizeof(NDSCIOCTL_REMOVE_USERBACL);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePool(NonPagedPool , OutbuffSize);

	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}	

	
	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_REMOVE_USERBACL;
		pSrbIoCtl->Length = sizeof(NDSCIOCTL_REMOVE_USERBACL);

		
		removeNdasBace = (PNDSCIOCTL_REMOVE_USERBACL)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		removeNdasBace->NdasBlockAceId = BlockAceId;

		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(!NT_SUCCESS(RC)) {
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(!NT_SUCCESS(RC)) {
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
				("Fail XixFsRemoveUserBacl Not NetDisk. status = %x\n", RC));
			}
		}

	


	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);
	}

	return RC;
}



NTSTATUS
XixfsGetNdasBacl(
	IN	PDEVICE_OBJECT	DeviceObject,
	OUT PNDAS_BLOCK_ACL	NdasBacl,
	IN BOOLEAN			SystemOrUser
)
{

	NTSTATUS		RC = STATUS_SUCCESS;
	BOOLEAN			result;
	PSRB_IO_CONTROL	pSrbIoCtl;
	PNDASSCSI_QUERY_INFO_DATA	QueryInfo;
	int				OutbuffSize = 0;
	PUCHAR			outBuff = NULL;
	PDEVICE_OBJECT	scsiAdpaterDeviceObject;


	PAGED_CODE();

	ASSERT(DeviceObject);	


	// need somemore
	ASSERT( NdasBacl->Length >= sizeof(NDASSCSI_QUERY_INFO_DATA) );
	
	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixfsGetNdasBacl!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}	


	

	if(NdasBacl->Length < sizeof(NDASSCSI_QUERY_INFO_DATA)){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixfsGetNdasBacl : TOO SAMLL BUFFER !!! \n"));
		return STATUS_UNSUCCESSFUL;
	}

	OutbuffSize = sizeof(SRB_IO_CONTROL) + NdasBacl->Length;
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePool(NonPagedPool , OutbuffSize);

	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}	

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_QUERYINFO_EX;
		pSrbIoCtl->Length = OutbuffSize;


		
		QueryInfo = (PNDASSCSI_QUERY_INFO_DATA)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		outBuff = (PUCHAR)QueryInfo;

		QueryInfo->Length = sizeof(NDASSCSI_QUERY_INFO_DATA);
		QueryInfo->InfoClass = SystemOrUser?NdscSystemBacl:NdscUserBacl;
		QueryInfo->QueryDataLength = 0;
		
		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);
		
		if(NT_SUCCESS(RC)){
			RtlCopyMemory(NdasBacl, outBuff, NdasBacl->Length);
		}else{
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);
			
			if(NT_SUCCESS(RC)){
				RtlCopyMemory(NdasBacl, outBuff, NdasBacl->Length);
			}
		}


	
		
	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);
	}

	return RC;
}





NTSTATUS
XixFsCheckXifsd(
	PDEVICE_OBJECT			DeviceObject,
	PPARTITION_INFORMATION	partitionInfo,
	PBLOCKACE_ID			BlockAceId,
	PBOOLEAN				IsWriteProtected
)
{
	NTSTATUS		RC = STATUS_SUCCESS;
	NDSCIOCTL_PRIMUNITDISKINFO DiskInfo;
	NDAS_BLOCK_ACE	Partition_BACL;
	ASSERT(DeviceObject);
	
	*IsWriteProtected = FALSE;
	
	DebugTrace(DEBUG_LEVEL_ALL, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsSetXifsd \n"));

	try{
		RC = XixFsGetNdadDiskInfo(DeviceObject, &DiskInfo);

		if(NT_SUCCESS(RC)) {
				
			// is device support ndas lock
			if((DiskInfo.Lur.SupportedFeatures & (NDASFEATURE_GP_LOCK|NDASFEATURE_ADV_GP_LOCK)) == 0 ){
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Enter XixFsSetXifsd : don't support lock \n"));
				RC = STATUS_UNSUCCESSFUL;
				try_return(RC);
			}

		
		
			if(DiskInfo.Lur.DeviceMode != DEVMODE_SHARED_READWRITE){

				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Failed XixFsSetXifsd : don't support read write \n"));
// Changed by ILGU HONG for readonly 09052006
				*IsWriteProtected = TRUE;
				RC = STATUS_SUCCESS;
// Changed by ILGU HONG for readonly end
				try_return(RC);
			}

				
			if(DiskInfo.Lur.EnabledFeatures  & NDASFEATURE_RO_FAKE_WRITE){
				
				RtlZeroMemory(&Partition_BACL, sizeof(PNDAS_BLOCK_ACE));
				Partition_BACL.AccessMode = NBACE_ACCESS_WRITE;
				Partition_BACL.BlockEndAddr = (partitionInfo->StartingOffset.QuadPart + partitionInfo->PartitionLength.QuadPart)/512 -1;
				Partition_BACL.BlockStartAddr = partitionInfo->StartingOffset.QuadPart/512;

				RC = XixFsAddUserBacl(DeviceObject, &Partition_BACL,BlockAceId);
				
				if(!NT_SUCCESS(RC)){
					DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
						("Faile XixFsSetXifsd : XixFsAddUserBacl \n"));					
				}

			}


		}
	}finally{

	}


	return RC;	
}




/*
NTSTATUS
XixFsSetXifsd(
	PXIFS_IRPCONTEXT IrpContext, 
	PXIFS_VCB VCB)
{

	PDEVICE_OBJECT		DeviceObject = NULL;
	PDEVICE_OBJECT		scsiAdpaterDeviceObject = NULL;
	BOOLEAN				result = FALSE;
	NTSTATUS			RC;
	PSRB_IO_CONTROL		pSrbIoCtl = NULL;
	uint32				OutbuffSize = 0;


	PAGED_CODE();

	ASSERT_VCB(VCB);

	DeviceObject = VCB->TargetDeviceObject;
	ASSERT(DeviceObject);

	
	DebugTrace(DEBUG_LEVEL_ALL, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsSetXifsd \n"));

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail GetScsiportAdpater!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}

	OutbuffSize = sizeof(SRB_IO_CONTROL);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePoolWithTag(NonPagedPool, OutbuffSize, TAG_BUFFER);
	
	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, LANSCSIMINIPORT_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = LANSCSIMINIPORT_IOCTL_UPDATE_XIFS;
		pSrbIoCtl->Length =  0;
		
		RC = XixFsRawDevIoCtrl ( 
						scsiAdpaterDeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(!NT_SUCCESS(RC)){
			DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
				("Fail XixFsRawDevIoCtrl Status (0x%x)\n", RC));
		}else{
			DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
				("Success XixFsRawDevIoCtrl Status (0x%x)\n", RC));
		}

	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);

	}

	DebugTrace(DEBUG_LEVEL_ALL, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Exit XixFsSetXifsd Status (0x%x)\n", RC));

	return STATUS_SUCCESS;
	
}
*/

	//	Added by ILGU HONG end

NTSTATUS
XixFsNdasLock(
	PDEVICE_OBJECT	DeviceObject
)
{
	NTSTATUS		RC;
	PSRB_IO_CONTROL	pSrbIoCtl = NULL;
	uint32			OutbuffSize = 0;
	PNDSCIOCTL_DEVICELOCK	QueryLock = NULL;
	PDEVICE_OBJECT		scsiAdpaterDeviceObject = NULL;
	BOOLEAN				result = FALSE;
	
	PAGED_CODE();

	

	

	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsNdasLock \n"));

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixFsGetScsiportAdpater!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}

	OutbuffSize = sizeof(SRB_IO_CONTROL) + sizeof(NDSCIOCTL_DEVICELOCK);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePoolWithTag(NonPagedPool, OutbuffSize, TAG_BUFFER);
	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_DEVICE_LOCK;
		pSrbIoCtl->Length = sizeof(NDSCIOCTL_DEVICELOCK);

		QueryLock = (PNDSCIOCTL_DEVICELOCK)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));

		QueryLock->LockId = NDSCLOCK_ID_XIFS;
		QueryLock->LockOpCode = NDSCLOCK_OPCODE_ACQUIRE;
		QueryLock->AdvancedLock = FALSE;
		QueryLock->AddressRangeValid = FALSE;
		QueryLock->RequireLockAcquisition = FALSE;
		QueryLock->StartingAddress = 0;
		QueryLock->EndingAddress = 0;
		QueryLock->ContentionTimeOut = 0;
	
		
		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(NT_SUCCESS(RC)){
			DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
				("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
					RC,
					QueryLock->LockData[0],	QueryLock->LockData[1],
					QueryLock->LockData[2],	QueryLock->LockData[3],
					QueryLock->LockData[4],	QueryLock->LockData[5],
					QueryLock->LockData[6],	QueryLock->LockData[7] ));
		}else{
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(!NT_SUCCESS(RC)){
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Fail XixFsRawDevIoCtrl Status (0x%x)\n", RC));
			}else{
				DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
					("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
						RC,
						QueryLock->LockData[0],	QueryLock->LockData[1],
						QueryLock->LockData[2],	QueryLock->LockData[3],
						QueryLock->LockData[4],	QueryLock->LockData[5],
						QueryLock->LockData[6],	QueryLock->LockData[7] ));
			}
		}


	

	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);

	}

	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Exit XixFsNdasLock Status (0x%x)\n", RC));

	return RC;
}


NTSTATUS
XixFsNdasUnLock(
	PDEVICE_OBJECT	DeviceObject
)
{

	NTSTATUS		RC;
	PSRB_IO_CONTROL	pSrbIoCtl = NULL;
	uint32			OutbuffSize = 0;
	PNDSCIOCTL_DEVICELOCK	QueryLock = NULL;
	PDEVICE_OBJECT		scsiAdpaterDeviceObject = NULL;
	BOOLEAN				result = FALSE;
	

	PAGED_CODE();
	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsNdasUnLock \n"));

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail GetScsiportAdpater!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}

	OutbuffSize = sizeof(SRB_IO_CONTROL) +  sizeof(NDSCIOCTL_DEVICELOCK);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePoolWithTag(NonPagedPool, OutbuffSize, TAG_BUFFER);
	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_DEVICE_LOCK;
		pSrbIoCtl->Length = sizeof(NDSCIOCTL_DEVICELOCK);

		QueryLock = (PNDSCIOCTL_DEVICELOCK)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		QueryLock->LockId = NDSCLOCK_ID_XIFS;
		QueryLock->LockOpCode = NDSCLOCK_OPCODE_RELEASE;
		QueryLock->AdvancedLock = FALSE;
		QueryLock->AddressRangeValid = FALSE;
		QueryLock->RequireLockAcquisition = FALSE;
		QueryLock->StartingAddress = 0;
		QueryLock->EndingAddress = 0;
		QueryLock->ContentionTimeOut = 0;
		

		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(NT_SUCCESS(RC)){
			DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
				("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
					RC,
					QueryLock->LockData[0],	QueryLock->LockData[1],
					QueryLock->LockData[2],	QueryLock->LockData[3],
					QueryLock->LockData[4],	QueryLock->LockData[5],
					QueryLock->LockData[6],	QueryLock->LockData[7] ));
		}else{
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(!NT_SUCCESS(RC)){
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Fail XixFsRawDevIoCtrl Status (0x%x)\n", RC));
			}else{
				DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
					("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
						RC,
						QueryLock->LockData[0],	QueryLock->LockData[1],
						QueryLock->LockData[2],	QueryLock->LockData[3],
						QueryLock->LockData[4],	QueryLock->LockData[5],
						QueryLock->LockData[6],	QueryLock->LockData[7] ));
			}
		}


	

	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);
	}

	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Exit XixFsNdasUnLock Status (0x%x)\n", RC));

	return RC;

}


NTSTATUS
XixFsNdasQueryLock(
	PDEVICE_OBJECT	DeviceObject
)
{



	NTSTATUS		RC;
	PSRB_IO_CONTROL	pSrbIoCtl = NULL;
	uint32			OutbuffSize = 0;
	PNDSCIOCTL_DEVICELOCK	QueryLock = NULL;
	PDEVICE_OBJECT		scsiAdpaterDeviceObject = NULL;
	BOOLEAN				result = FALSE;


	PAGED_CODE();
	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Enter XixFsNdasQueryLock \n"));

	result = XixFsGetScsiportAdapter(DeviceObject, &scsiAdpaterDeviceObject);
	if(result != TRUE){
		DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
			("Fail XixFsGetScsiportAdpater!!! \n"));
		return STATUS_UNSUCCESSFUL;
	}


	OutbuffSize = sizeof(SRB_IO_CONTROL) +  sizeof(NDSCIOCTL_DEVICELOCK);
	OutbuffSize = SECTORALIGNSIZE_512(OutbuffSize);
	pSrbIoCtl = (PSRB_IO_CONTROL)ExAllocatePoolWithTag(NonPagedPool, OutbuffSize, TAG_BUFFER);
	if(!pSrbIoCtl){
		ObDereferenceObject(scsiAdpaterDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	try{
		RtlZeroMemory(pSrbIoCtl, OutbuffSize);
		
		pSrbIoCtl->HeaderLength = sizeof(SRB_IO_CONTROL);
		RtlCopyMemory(pSrbIoCtl->Signature, NDASSCSI_IOCTL_SIGNATURE, 8);
		pSrbIoCtl->Timeout = 10;
		pSrbIoCtl->ControlCode = NDASSCSI_IOCTL_DEVICE_LOCK;
		pSrbIoCtl->Length =  sizeof(NDSCIOCTL_DEVICELOCK);

		QueryLock = (PNDSCIOCTL_DEVICELOCK)((PUCHAR)pSrbIoCtl + sizeof(SRB_IO_CONTROL));
		QueryLock->LockId = NDSCLOCK_ID_XIFS;
		QueryLock->LockOpCode = NDSCLOCK_OPCODE_QUERY_OWNER;
		QueryLock->AdvancedLock = FALSE;
		QueryLock->AddressRangeValid = FALSE;
		QueryLock->RequireLockAcquisition = FALSE;
		QueryLock->StartingAddress = 0;
		QueryLock->EndingAddress = 0;
		QueryLock->ContentionTimeOut = 0;
		


		//Fist step Send Disk
		RC = XixFsRawDevIoCtrl ( 
						DeviceObject,
						IOCTL_SCSI_MINIPORT,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						(uint8 *)pSrbIoCtl,
						OutbuffSize,
						FALSE,
						NULL
						);

		if(NT_SUCCESS(RC)){
			DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
				("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
					RC,
					QueryLock->LockData[0],	QueryLock->LockData[1],
					QueryLock->LockData[2],	QueryLock->LockData[3],
					QueryLock->LockData[4],	QueryLock->LockData[5],
					QueryLock->LockData[6],	QueryLock->LockData[7] ));
		}else{
			// Send port
			RC = XixFsRawDevIoCtrl ( 
							scsiAdpaterDeviceObject,
							IOCTL_SCSI_MINIPORT,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							(uint8 *)pSrbIoCtl,
							OutbuffSize,
							FALSE,
							NULL
							);

			if(!NT_SUCCESS(RC)){
				DebugTrace(DEBUG_LEVEL_ERROR, DEBUG_TARGET_ALL,
					("Fail XixFsRawDevIoCtrl Status (0x%x)\n", RC));
			}else{
				DebugTrace(DEBUG_LEVEL_INFO, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
					("Status of Lock (0x%x) Info(0x%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x)\n",
						RC,
						QueryLock->LockData[0],	QueryLock->LockData[1],
						QueryLock->LockData[2],	QueryLock->LockData[3],
						QueryLock->LockData[4],	QueryLock->LockData[5],
						QueryLock->LockData[6],	QueryLock->LockData[7] ));
			}
		}


	

	}finally{
		ObDereferenceObject(scsiAdpaterDeviceObject);
		ExFreePool(pSrbIoCtl);
	}

	DebugTrace(DEBUG_LEVEL_TRACE, (DEBUG_TARGET_RESOURCE| DEBUG_TARGET_FCB|DEBUG_TARGET_LOCK),
		("Exit XixFsNdasUnLock Status (0x%x)\n", RC));

	return RC;

}