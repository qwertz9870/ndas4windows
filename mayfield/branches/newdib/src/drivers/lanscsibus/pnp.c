/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    PNP.C

Abstract:

    This module handles plug & play calls for the toaster bus controller FDO.

Author:

 Eliyas Yakub   Sep 11, 1998
 
Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <ntddk.h>
#include <initguid.h>

#include "devreg.h"
#include "ndasbus.h"
#include "lsbusioctl.h"
#include "busenum.h"
#include "stdio.h"
#include <wdmguid.h>
#include "ndasbuspriv.h"

#ifdef __MODULE__
#undef __MODULE__
#endif // __MODULE__
#define __MODULE__ "BUS_PNP"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_AddDevice)
#pragma alloc_text (PAGE, Bus_PnP)
#pragma alloc_text (PAGE, Bus_PlugInDeviceEx)
#pragma alloc_text (PAGE, Bus_PlugInDeviceEx2)
#pragma alloc_text (PAGE, Bus_InitializePdo)
#pragma alloc_text (PAGE, Bus_UnPlugDevice)
#pragma alloc_text (PAGE, Bus_DestroyPdo)
#pragma alloc_text (PAGE, Bus_RemoveFdo)
#pragma alloc_text (PAGE, Bus_FDO_PnP)
#pragma alloc_text (PAGE, Bus_StartFdo)
#pragma alloc_text (PAGE, Bus_SendIrpSynchronously)
#endif

NTSTATUS
Bus_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description.

    Our Toaster bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  
    And be prepared for the ``start device''

Arguments:

    DriverObject - pointer to driver object.

    PhysicalDeviceObject  - Device object representing the bus to which we
                            will attach a new FDO.

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PFDO_DEVICE_DATA    deviceData;
	ULONG				tempUlong;

#if DBG
    PWCHAR              deviceName;
    ULONG               nameLength;
#endif

    PAGED_CODE ();

    Bus_KdPrint_Def (BUS_DBG_SS_TRACE, ("0x%x\n",
                                          PhysicalDeviceObject));

    status = IoCreateDevice (
                    DriverObject,               // our driver object
                    sizeof (FDO_DEVICE_DATA),   // device object extension size
                    NULL,                       // FDOs do not have names
                    FILE_DEVICE_BUS_EXTENDER,   // We are a bus
                    FILE_DEVICE_SECURE_OPEN,    // 
                    TRUE,                       // our FDO is exclusive
                    &deviceObject);             // The device object created

    if (!NT_SUCCESS (status)) 
    {
        return status;
    }

    deviceData = (PFDO_DEVICE_DATA) deviceObject->DeviceExtension;
    RtlZeroMemory (deviceData, sizeof (FDO_DEVICE_DATA));

    //
    // Set the initial state of the FDO
    //

    INITIALIZE_PNP_STATE(deviceData);

    deviceData->DebugLevel = BusEnumDebugLevel;

    deviceData->IsFDO = TRUE;
   
    deviceData->Self = deviceObject;

	ExInitializeFastMutex (&deviceData->Mutex);

	ExInitializeFastMutex (&deviceData->RegMutex);

    InitializeListHead (&deviceData->ListOfPDOs);

    // Set the PDO for use with PlugPlay functions
    
    deviceData->UnderlyingPDO = PhysicalDeviceObject;

	deviceData->PersistentPdo = Globals.PersistentPdo;
	deviceData->StartStopRegistrarEnum = Globals.PersistentPdo;


	//
	//	Read options from the PDO's registry
	//

	// Disable persistent PDO option

	status = DrReadDevKeyValueInstantly(	PhysicalDeviceObject,
									BUSENUM_BUSPDOREG_DISABLE_PERSISTENTPDO,
									REG_DWORD,
									&tempUlong,
									sizeof(tempUlong),
									NULL);
	if(NT_SUCCESS(status) && tempUlong != 0) {
		deviceData->PersistentPdo = FALSE;
		Bus_KdPrint_Def (BUS_DBG_SS_INFO, 
			("Persistent PDO option disabled.\n"));
	}


    //
    // Set the initial powerstate of the FDO
    //
    
    deviceData->DevicePowerState = PowerDeviceUnspecified;
    deviceData->SystemPowerState = PowerSystemWorking;


    //
    // Biased to 1. Transition to zero during remove device 
    // means IO    case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";
            
        default:
            return "IRP_MN_?????";
    }
}

PCHAR 
DbgSystemPowerString(
    IN SYSTEM_POWER_STATE Type
    ) 
{  
    switch (Type)
    {
        case PowerSystemUnspecified:
            return "PowerSystemUnspecified";
        case PowerSystemWorking:
            return "PowerSystemWorking";
        case PowerSystemSleeping1:
            return "PowerSystemSleeping1";
        case PowerSystemSleeping2:
            return "PowerSystemSleeping2";
        case PowerSystemSleeping3:
            return "PowerSystemSleeping3";
        case PowerSystemHibernate:
            return "PowerSystemHibernate";
        case PowerSystemShutdown:
            return "PowerSystemShutdown";
        case PowerSystemMaximum:
            return "PowerSystemMaximum";
        default:
            return "UnKnown System Power State";
    }
 }

PCHAR 
DbgDevicePowerString(
    IN DEVICE_POWER_STATE Type
    )   
{
    switch (Type)
    {
        case PowerDeviceUnspecified:
            return "PowerDeviceUnspecified";
        case PowerDeviceD0:
            return "PowerDeviceD0";
        case PowerDeviceD1:
            return "PowerDeviceD1";
        case PowerDeviceD2:
            return "PowerDeviceD2";
        case PowerDeviceD3:
            return "PowerDeviceD3";
        case PowerDeviceMaximum:
            return "PowerDeviceMaximum";
        default:
            return "UnKnown Device Power State";
    }
}

#endif


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  #include <ntddk.h>
#include <tdikrnl.h>
#include <scsi.h>
#include <ntddscsi.h>

#include "ndasbus.h"
#include "devreg.h"
#include "lstransport.h"
#include "lsbusioctl.h"
#include "lsminiportioctl.h"
#include "ndas/ndasdib.h"

#include "busenum.h"
#include "stdio.h"
#include "ndasbuspriv.h"


#ifdef __MODULE__
#undef __MODULE__
#endif // __MODULE__
#define __MODULE__ "Register"


//
//	Internal declarations
//

NTSTATUS
Reg_InitializeTdiPnPHandlers(
	PFDO_DEVICE_DATA	FdoData
);

VOID
Reg_DeregisterTdiPnPHandlers (
	PFDO_DEVICE_DATA	FdoData
);

VOID
Reg_AddAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING  DeviceName,
	IN PTDI_PNP_CONTEXT Context
);

VOID
Reg_DelAddressHandler ( 
	IN PTA_ADDRESS NetworkAddress,
	IN PUNICODE_STRING DeviceName,
	IN PTDI_PNP_CONTEXT Context
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, Reg_InitializeTdiPnPHandlers )
#pragma alloc_text( PAGE, Reg_AddAddressHandler )
#pragma alloc_text( PAGE, Reg_DelAddressHandler )
#endif


//
//	Pool tags
//

#define NDBUSREG_POOLTAG_PLUGIN		'ipRN'
#define NDBUSREG_POOLTAG_ADDTARGET	'taRN'
#define NDBUSREG_POOLTAG_WORKITEM	'iwRN'
#define	NDBUSREG_POOLTAG_KEYINFO	'ikRN'
#define NDBUSREG_POOLTAG_NAMEBUFF	'bnRN'


//
//	Registry key name of NDAS devices
//

#define NDBUS_REG_NAME			L"Devices"


//
//	NDAS device information
//

#define NDBUS_REG_DEVREGNAME	L"Dev%ld"					// Key Name

//	Miniport

#define NDBUS_REG_SLOTNO		L"SlotNo"					// 4 bytes
#define NDBUS_REG_HWIDLEN		L"HardwareIDLen"			// 4 bytes
#define NDBUS_REG_HWIDS			L"HardwareIDs"				// Variable length.
#define NDBUS_REG_DEVMAXBPR		L"MaxBlocksPerRequest"		// 4 bytes
#define NDBUS_REG_FLAGS			L"Flags"					// 4 bytes


// AddTargetData

#define NDBUS_REG_TARGETNAME	L"Target%ld"				// Key name

#define NDBUS_REG_TARGETID		L"TargetId"					// 4 bytes
#define NDBUS_REG_TARGETTYPE	L"TargetType"				// 4 bytes
#define NDBUS_REG_DESIREACC		L"DesiredAccess"			// 4 bytes
#define NDBUS_REG_TARGETBLKS	L"TargetBlks"				// 4 bytes
#define NDBUS_REG_CNTECR		L"CntEcrMethod"				// 4 bytes
#define NDBUS_REG_CNTECRKEYLEN	L"CntEcrKeyLen"				// 4 bytes
#define NDBUS_REG_CNTECRKEY		L"CntEcrKey"				// Variable length.
#define NDBUS_REG_LUROPTIONS	L"LurOptions"				// 4 bytes
#define NDBUS_REG_LURFLAGS		L"LurFlags"					// 4 bytes
#define NDBUS_REG_UNITCNT		L"UnitCount"				// 4 bytes
#define NDBUS_REG_RAID_SPB		L"RAID_SectorPerBit"		// 4 bytes
#define NDBUS_REG_RAID_SPARE	L"RAID_nSpareCount"			// 4 bytes


//
//	NDAS Unit
//

#define NDBUS_REG_UNITREG_NAME	L"Unit%ld"					// Key name

#define NDBUS_REG_ADDR			L"Address"					// 18 bytes
#define NDBUS_REG_NICADDR		L"NicAddress"				// 18 bytes
#define	NDBUS_REG_ISWAN			L"IsWan"					// 4 bytes
#define NDBUS_REG_UNITNO		L"UnitNo"					// 4 bytes
#define NDBUS_REG_UNITBLKS		L"UnitBlks"					// 8 bytes
#define NDBUS_REG_PHYBLKS		L"PhyBlks"					// 8 bytes
#define NDBUS_REG_HWTYPE		L"HWType"					// 4 bytes
#define NDBUS_REG_HWVER			L"HWVersion"				// 4 bytes
#define NDBUS_REG_USERID		L"UserID"					// 4 bytes
#define NDBUS_REG_PW			L"PW"						// 8 bytes
#define NDBUS_REG_LURNOPT		L"LurnOpt"					// 4 bytes
#define NDBUS_REG_RECONTRIAL	L"ReconnTrial"				// 4 bytes
#define NDBUS_REG_RECONINTERV	L"ReconnInterval"			// 4 bytes


//
//	RAID specific for each NDAS Unit
//

/*
#define NDBUS_REG_RAID_SECTOR		L"RAID_Sector"				// 8 bytes
#define NDBUS_REG_RAID_OFFSETFLAG	L"RAID_OffsetFlag"			// 4 bytes
#define NDBUS_REG_RAID_BMSTART		L"RAID_BMLocat"				// 8 bytes
#define NDBUS_REG_RAID_LASTWRIT		L"RAID_LastWrittenSector"	// 8 bytes
*/


//
//	Max numbers
//

#define	MAX_DEVICES_IN_NDAS_REGISTRY	(MAXLONG)



//////////////////////////////////////////////////////////////////////////
//
//	Open registry
//

//
//	Open a registry key of NDAS devices.
//
NTSTATUS
Reg_OpenNdasDeviceRoot(
		PHANDLE			NdasDeviceReg,
		ACCESS_MASK		AccessMask,
		HANDLE			BusDevRoot
	){
    NTSTATUS			status;
	HANDLE				regKey;
	OBJECT_ATTRIBUTES	objectAttributes;
	UNICODE_STRING		objectName;


	RtlInitUnicodeString(&objectName, NDBUS_REG_NAME);
	InitializeObjectAttributes( &objectAttributes,
								&objectName,
								OBJ_KERNEL_HANDLE,
								BusDevRoot,
								NULL
							);
	status = ZwCreateKey(
						&regKey,
						AccessMask,
						&objectAttributes,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						NULL
					);

	*NdasDeviceReg = regKey;

    return status;
}


//
//	Open a child key in the parent key
//

NTSTATUS
Reg_OpenChildKey(
		PHANDLE	ChildKey,
		PWCHAR	ChildNameTpl,
		ULONG	PostfixNo,
		BOOLEAN	DeleteExisting,
		HANDLE	ParentKey
		) {
	NTSTATUS					status;
	OBJECT_ATTRIBUTES			objectAttributes;
	UNICODE_STRING				objectName;
	PWSTR						nameBuffer;
	HANDLE						devReg;
	ULONG						dispos;

	status = STATUS_SUCCESS;

	nameBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, 512, NDBUSREG_POOLTAG_NAMEBUFF);
	if(!nameBuffer) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ExAllocatePoolWithTag(KEY_BASIC_INFORMATION) failed.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	objectName.Buffer = nameBuffer;
	objectName.MaximumLength = 512;

	status = swprintf(objectName.Buffer, ChildNameTpl, PostfixNo);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("RtlStringCbPrintfW() failed. NTSTATUS:%08lx\n", status));
		goto cleanup;
	}
	objectName.Length = (USHORT)wcslen(objectName.Buffer) * sizeof(WCHAR);

	InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_KERNEL_HANDLE, ParentKey, NULL);


	status = ZwCreateKey(
							&devReg,
							KEY_ALL_ACCESS,
							&objectAttributes,
							0,
							NULL,
							REG_OPTION_NON_VOLATILE,
							&dispos
						);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Inital ZwCreateKey() failed. NTSTATUS:%08lx\n", status));
		ASSERT(FALSE);
		goto cleanup;
	}


	//
	//	Delete a key if a caller specify.
	//

	if(dispos == REG_OPENED_EXISTING_KEY && DeleteExisting) {
		status = DrDeleteAllSubKeys(devReg);
		if(!NT_SUCCESS(status)) {
			ZwClose(devReg);
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ZwDeleteKey() failed. NTSTATUS:%08lx\n", status));
			goto cleanup;
		}
	}

	//
	//	Set return values
	//
	*ChildKey = devReg;

cleanup:
	if(nameBuffer)
		ExFreePool(nameBuffer);

	return status;
}


//
//	Open a registry key of an NDAS device instance
//

NTSTATUS
Reg_OpenDeviceControlRoot(
		PHANDLE		DevControlKey,
		ACCESS_MASK	AccessMask
){
	NTSTATUS					status;
	OBJECT_ATTRIBUTES			objectAttributes;
	UNICODE_STRING				objectName;
	PWSTR						nameBuffer;
	HANDLE						devReg;
	ULONG						dispos;

	status = STATUS_SUCCESS;

	nameBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, 512, NDBUSREG_POOLTAG_NAMEBUFF);
	if(!nameBuffer) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ExAllocatePoolWithTag(KEY_BASIC_INFORMATION) failed.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	objectName.Buffer = nameBuffer;
	objectName.MaximumLength = 512;
	RtlInitUnicodeString(&objectName, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\NDAS");
	objectName.Length = (USHORT)wcslen(objectName.Buffer) * sizeof(WCHAR);

	InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE, NULL, NULL);

	status = ZwCreateKey(
					&devReg,
					AccessMask,
					&objectAttributes,
					0,
					NULL,
					REG_OPTION_NON_VOLATILE,
					&dispos);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Inital ZwCreateKey() failed. NTSTATUS:%08lx\n", status));
		ASSERT(FALSE);
		goto cleanup;
	}

	//
	//	Set return values
	//
	*DevControlKey = devReg;

cleanup:
	if(nameBuffer)
		ExFreePool(nameBuffer);

	return status;
}


//
//	Open a registry key of an NDAS device instance
//

NTSTATUS
Reg_OpenDeviceInst(
		PHANDLE	DevInstKey,
		ULONG	SlotNo,
		BOOLEAN	DeleteExisting,
		HANDLE	NdasDeviceRoot
){
	return Reg_OpenChildKey(DevInstKey, NDBUS_REG_DEVREGNAME, SlotNo, DeleteExisting, NdasDeviceRoot);
}

//
//	Open a registry key of an NDAS device instance
//
NTSTATUS
Reg_OpenTarget(
		PHANDLE	TargetKey,
		ULONG	TargetId,
		BOOLEAN	DeleteExisting,
		HANDLE	NdasDeviceInst
){

	return Reg_OpenChildKey(TargetKey, NDBUS_REG_TARGETNAME, TargetId, DeleteExisting, NdasDeviceInst);
}

//
//	Open a registry key of an Unit device
//
NTSTATUS
Reg_OpenUnit(
		 PHANDLE	UnitKey,
		 ULONG		UnitIdx,
		 BOOLEAN	DeleteExisting,
		 HANDLE		TargetKey
){

	return Reg_OpenChildKey(UnitKey, NDBUS_REG_UNITREG_NAME, UnitIdx, DeleteExisting, TargetKey);
}


//////////////////////////////////////////////////////////////////////////
//
//	Read registry
//


//
//	Read an NDAS Unit device information from the registry
//

NTSTATUS
ReadUnitFromRegistry(
		HANDLE				UnitDevKey,
		PLSBUS_UNITDISK		Unit
) {
	NTSTATUS	status;
	ULONG		tempUlong;
	//
	//	read NDAS unit device information
	//

	//	Address
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_ADDR,
						REG_BINARY,
						&Unit->Address,
						sizeof(Unit->Address),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_ADDR));
		return	status;
	}
	//	NIC Address
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_NICADDR,
						REG_BINARY,
						&Unit->NICAddr,
						sizeof(Unit->NICAddr),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_NICADDR));
		return	status;
	}
	//	IsWAN
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_ISWAN,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_ISWAN));
		return	status;
	}
	Unit->IsWANConnection = (UCHAR)tempUlong;

	//	Unit number
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_UNITNO,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITNO));
		return	status;
	}
	Unit->ucUnitNumber = (UCHAR)tempUlong;

	//	Unit blocks
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_UNITBLKS,
						REG_BINARY,
						&Unit->ulUnitBlocks,
						sizeof(Unit->ulUnitBlocks),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITBLKS));
		return	status;
	}

	//	Physical blocks
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_PHYBLKS,
						REG_BINARY,
						&Unit->ulPhysicalBlocks,
						sizeof(Unit->ulPhysicalBlocks),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_PHYBLKS));
		return	status;
	}

	//	Hardware type
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_HWTYPE,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWTYPE));
		return	status;
	}
	Unit->ucHWType = (UCHAR)tempUlong;

	//	Hardware version
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_HWVER,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWVER));
		return	status;
	}
	Unit->ucHWVersion = (UCHAR)tempUlong;

	//	User ID
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_USERID,
						REG_DWORD,
						&Unit->iUserID,
						sizeof(Unit->iUserID),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_USERID));
		return	status;
	}

	//	Password
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_PW,
						REG_BINARY,
						&Unit->iPassword,
						sizeof(Unit->iPassword),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_PW));
		return	status;
	}

	//	LURN options
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_LURNOPT,
						REG_DWORD,
						&Unit->LurnOptions,
						sizeof(Unit->LurnOptions),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LURNOPT));
		return	status;
	}

	//	Reconnection trial
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_RECONTRIAL,
						REG_DWORD,
						&Unit->ReconnTrial,
						sizeof(Unit->ReconnTrial),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RECONTRIAL));
		return	status;
	}
	//	Reconnection interval
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_RECONINTERV,
						REG_DWORD,
						&Unit->ReconnInterval,
						sizeof(Unit->ReconnInterval),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RECONINTERV));
		return	status;
	}
/*
		//	RAID: Sector information
		status = DrReadKeyValue(
							UnitDevKey,
							NDBUS_REG_RAID_SECTOR,
							REG_BINARY,
							&Unit->RAID_Info.SectorInfo,
							sizeof(Unit->RAID_Info.SectorInfo),
							NULL
						);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SECTOR));
			return	status;
		}
		//	RAID: offset flags
		status = DrReadKeyValue(
							UnitDevKey,
							NDBUS_REG_RAID_OFFSETFLAG,
							REG_DWORD,
							&Unit->RAID_Info.OffsetFlagInfo,
							sizeof(Unit->RAID_Info.OffsetFlagInfo),
							NULL
						);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_OFFSETFLAG));
			return	status;
		}
		//	Bitmap location
		status = DrReadKeyValue(
							UnitDevKey,
							NDBUS_REG_RAID_BMSTART,
							REG_BINARY,
							&Unit->RAID_Info.SectorBitmapStart,
							sizeof(Unit->RAID_Info.SectorBitmapStart),
							NULL
						);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_BMSTART));
			return	status;
		}

	//	Last written sector
	status = DrReadKeyValue(
						UnitDevKey,
						NDBUS_REG_RAID_LASTWRIT,
						REG_BINARY,
						&Unit->RAID_Info.SectorLastWrittenSector,
						sizeof(Unit->RAID_Info.SectorLastWrittenSector),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_LASTWRIT));
		return	status;
	}
*/

	return STATUS_SUCCESS;
}


//
//	Read a target and unit devices.
//

NTSTATUS
ReadTargetAndUnitFromRegistry(
		HANDLE						TargetKey,
		ULONG						AddTargetDataLen,
		PLANSCSI_ADD_TARGET_DATA	AddTargetData,
		PULONG						OutLength
) {
	NTSTATUS				status;
	ULONG					unitCnt;
	HANDLE					unitKey;
	PKEY_BASIC_INFORMATION	keyInfo;
	ULONG					outLength;
	ULONG					idxKey;
	ULONG					tempUlong;

	//
	//	Read unit device counter to check buffer size first.
	//
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_UNITCNT,
						REG_DWORD,
						&unitCnt,
						sizeof(unitCnt),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITCNT));
		return	status;
	}

	//
	//	Check buffer size.
	//
	outLength = FIELD_OFFSET(LANSCSI_ADD_TARGET_DATA, UnitDiskList) + sizeof(LSBUS_UNITDISK) * unitCnt;
	if(outLength > AddTargetDataLen) {
		if(OutLength)
			*OutLength = outLength;
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlZeroMemory(AddTargetData, outLength);
	AddTargetData->ulNumberOfUnitDiskList = unitCnt;
	AddTargetData->ulSize = outLength;

	//
	//	read Target information
	//

	//	Target ID
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_TARGETID,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETID));
		return	status;
	}
	AddTargetData->ucTargetId = (UCHAR)tempUlong;

	// Target type
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_TARGETTYPE,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETTYPE));
		return	status;
	}
	AddTargetData->ucTargetType = (UCHAR)tempUlong;;

	// Desired access
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_DESIREACC,
						REG_DWORD,
						&AddTargetData->DesiredAccess,
						sizeof(AddTargetData->DesiredAccess),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_DESIREACC));
		return	status;
	}

	// Target blocks
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_TARGETBLKS,
						REG_BINARY,
						&AddTargetData->ulTargetBlocks,
						sizeof(AddTargetData->ulTargetBlocks),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETBLKS));
		return	status;
	}

	// Content encryption method
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_CNTECR,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECR));
		return	status;
	}
	AddTargetData->CntEcrMethod = (UCHAR)tempUlong;

	// Content encryption key length
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_CNTECRKEYLEN,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECRKEYLEN));
		return	status;
	}
	AddTargetData->CntEcrKeyLength = (UCHAR)tempUlong;

	// Content encryption key
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_CNTECRKEY,
						REG_BINARY,
						&AddTargetData->CntEcrKey,
						sizeof(AddTargetData->CntEcrKey),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECRKEY));
		return	status;
	}

	// LUR flags
	status = DrReadKeyValue(
		TargetKey,
		NDBUS_REG_LURFLAGS,
		REG_DWORD,
		&AddTargetData->LurFlags,
		sizeof(AddTargetData->LurFlags),
		NULL
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LURFLAGS));
		return	status;
	}

	// LUR options
	status = DrReadKeyValue(
						TargetKey,
						NDBUS_REG_LUROPTIONS,
						REG_DWORD,
						&AddTargetData->LurOptions,
						sizeof(AddTargetData->LurOptions),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LUROPTIONS));
		return	status;
	}

	//	Sectors per bit
	status = DrReadKeyValue(
		TargetKey,
		NDBUS_REG_RAID_SPB,
		REG_DWORD,
		&AddTargetData->RAID_Info.SectorsPerBit,
		sizeof(AddTargetData->RAID_Info.SectorsPerBit),
		NULL
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SPB));
		return	status;
	}

	//	Spare disk count
	status = DrReadKeyValue(
		TargetKey,
		NDBUS_REG_RAID_SPARE,
		REG_DWORD,
		&AddTargetData->RAID_Info.nSpareDisk,
		sizeof(AddTargetData->RAID_Info.nSpareDisk),
		NULL
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SPARE));
		return	status;
	}	

	//
	//	read Unit device information.
	//
	keyInfo = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(PagedPool, 512, NDBUSREG_POOLTAG_KEYINFO);
	if(!keyInfo) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ExAllocatePoolWithTag(KEY_BASIC_INFORMATION) failed.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	for(idxKey = 0 ; idxKey < unitCnt; idxKey ++) {

		status = Reg_OpenUnit(&unitKey, idxKey, FALSE, TargetKey);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenUnit() failed. NTSTATUS:%08lx\n", status));
			ExFreePool(keyInfo);
			return status;
		}

		status = ReadUnitFromRegistry(unitKey, AddTargetData->UnitDiskList + idxKey);

		ZwClose(unitKey);

		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ReadLurnDescFromRegistry() failed. NTSTATUS:%08lx\n", status));
			ExFreePool(keyInfo);
			return status;
		}
	}

	ExFreePool(keyInfo);

	//
	//	Set return values
	//
	if(OutLength)
		*OutLength = outLength;

	return STATUS_SUCCESS;
}


//
//	read NDAS device instance information
//

NTSTATUS
ReadNDASDevInstFromRegistry(
		HANDLE							NdasDevInst,
		ULONG							PlugInLen,
		PBUSENUM_PLUGIN_HARDWARE_EX2	PlugIn,
		PULONG							OutLength
) {
	NTSTATUS	status;
	ULONG		outLength;
	ULONG		hwIDBufferLen;

	//
	//	Read unit device counter to check buffer size first.
	//
	status = DrReadKeyValue(
						NdasDevInst,
						NDBUS_REG_HWIDLEN,
						REG_DWORD,
						&hwIDBufferLen,
						sizeof(hwIDBufferLen),
						NULL
						);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWIDLEN));
		return	status;
	}

	//
	//	Check buffer size.
	//
	outLength = FIELD_OFFSET(BUSENUM_PLUGIN_HARDWARE_EX2, HardwareIDs) + hwIDBufferLen * sizeof(WCHAR);
	if(PlugInLen < outLength) {
		if(OutLength)
			*OutLength = outLength;
		return STATUS_BUFFER_TOO_SMALL;
	}
	PlugIn->Size = outLength;
	PlugIn->HardwareIDLen = hwIDBufferLen;


	//	Slot number
	status = DrReadKeyValue(
						NdasDevInst,
						NDBUS_REG_SLOTNO,
						REG_DWORD,
						&PlugIn->SlotNo,
						sizeof(PlugIn->SlotNo),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_SLOTNO));
		return	status;
	}

	//	Hardware IDs
	status = DrReadKeyValue(
						NdasDevInst,
						NDBUS_REG_HWIDS,
						REG_MULTI_SZ,
						&PlugIn->HardwareIDs,
						hwIDBufferLen * sizeof(WCHAR),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWIDS));
		return	status;
	}

	//	Device max blocks per request
	status = DrReadKeyValue(
						NdasDevInst,
						NDBUS_REG_DEVMAXBPR,
						REG_DWORD,
						&PlugIn->MaxRequestBlocks,
						sizeof(PlugIn->MaxRequestBlocks),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_DEVMAXBPR));
		return	status;
	}

	//	Plug in flags
	status = DrReadKeyValue(
						NdasDevInst,
						NDBUS_REG_FLAGS,
						REG_DWORD,
						&PlugIn->Flags,
						sizeof(PlugIn->Flags),
						NULL
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrReadKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_FLAGS));
		return	status;
	}


	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
//	Write registry
//


//
//	Write an NDAS unit device to the registry.
//

NTSTATUS
WriteUnitToRegistry(
		HANDLE				UnitReg,
		PLSBUS_UNITDISK		Unit
){
	NTSTATUS	status;
	ULONG		tempUlong;
	//
	//	read LUR Node information
	//

	//	Address
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_ADDR,
						REG_BINARY,
						&Unit->Address,
						sizeof(Unit->Address)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_ADDR));
		return	status;
	}

	//	NicAddress
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_NICADDR,
						REG_BINARY,
						&Unit->NICAddr,
						sizeof(Unit->NICAddr)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_NICADDR));
		return	status;
	}
	//	IsWan
	tempUlong = Unit->IsWANConnection;
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_ISWAN,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_ISWAN));
		return	status;
	}
	//	UnitNo
	tempUlong = Unit->ucUnitNumber;
	status = DrWriteKeyValue(
		UnitReg,
		NDBUS_REG_UNITNO,
		REG_DWORD,
		&tempUlong,
		sizeof(tempUlong)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITNO));
		return	status;
	}

	//	Unit blocks
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_UNITBLKS,
						REG_BINARY,
						&Unit->ulUnitBlocks,
						sizeof(Unit->ulUnitBlocks)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITBLKS));
		return	status;
	}
	//	Physical blocks
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_PHYBLKS,
						REG_BINARY,
						&Unit->ulPhysicalBlocks,
						sizeof(Unit->ulPhysicalBlocks)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_PHYBLKS));
		return	status;
	}
	//	Hardware type
	tempUlong = Unit->ucHWType;
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_HWTYPE,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWTYPE));
		return	status;
	}
	//	Hardware version
	tempUlong = Unit->ucHWVersion;
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_HWVER,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWVER));
		return	status;
	}
	//	User ID
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_USERID,
						REG_DWORD,
						&Unit->iUserID,
						sizeof(Unit->iUserID)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_USERID));
		return	status;
	}
	//	Password
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_PW,
						REG_BINARY,
						&Unit->iPassword,
						sizeof(Unit->iPassword)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_PW));
		return	status;
	}
	//	LURN options
	tempUlong = Unit->LurnOptions & (~LURNOPTION_MISSING);
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_LURNOPT,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LURNOPT));
		return	status;
	}
	//	Reconnection trials
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_RECONTRIAL,
						REG_DWORD,
						&Unit->ReconnTrial,
						sizeof(Unit->ReconnTrial)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RECONTRIAL));
		return	status;
	}
	//	Reconnection interval
	status = DrWriteKeyValue(
						UnitReg,
						NDBUS_REG_RECONINTERV,
						REG_DWORD,
						&Unit->ReconnInterval,
						sizeof(Unit->ReconnInterval)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RECONINTERV));
		return	status;
	}

	//	RAID: sector information
/*
		status = DrWriteKeyValue(
			UnitReg,
			NDBUS_REG_RAID_SECTOR,
			REG_BINARY,
			&Unit->RAID_Info.SectorInfo,
			sizeof(Unit->RAID_Info.SectorInfo)
			);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SECTOR));
			return	status;
		}
		//	RAID: Offset flags
		status = DrWriteKeyValue(
			UnitReg,
			NDBUS_REG_RAID_OFFSETFLAG,
			REG_DWORD,
			&Unit->RAID_Info.OffsetFlagInfo,
			sizeof(Unit->RAID_Info.OffsetFlagInfo)
			);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_OFFSETFLAG));
			return	status;
		}
		//	RAID: Bitmap location
		status = DrWriteKeyValue(
			UnitReg,
			NDBUS_REG_RAID_BMSTART,
			REG_BINARY,
			&Unit->RAID_Info.SectorBitmapStart,
			sizeof(Unit->RAID_Info.SectorBitmapStart)
			);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_BMSTART));
			return	status;
		}
		//	RAID: last written sector
		status = DrWriteKeyValue(
			UnitReg,
			NDBUS_REG_RAID_LASTWRIT,
			REG_BINARY,
			&Unit->RAID_Info.SectorLastWrittenSector,
			sizeof(Unit->RAID_Info.SectorLastWrittenSector)
			);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_LASTWRIT));
			return	status;
		}*/
	

	return STATUS_SUCCESS;
}


//
//	Write  AddTargetData to the registry.
//

NTSTATUS
WriteTargetToRegistry(
		HANDLE						NdasTargetKey,
		PLANSCSI_ADD_TARGET_DATA	AddTargetData
) {
	NTSTATUS				status;
	HANDLE					unitKey;
	PWSTR					nameBuffer;
	ULONG					idxKey;
	UNICODE_STRING			objectName;
	PLSBUS_UNITDISK			unit;
	ULONG					tempUlong;


	//
	//	Write LUR information.
	//

	//	Target ID
	tempUlong = AddTargetData->ucTargetId;
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_TARGETID,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETID));
		return	status;
	}
	//	Target type
	tempUlong = AddTargetData->ucTargetType;
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_TARGETTYPE,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
						);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETTYPE));
		return	status;
	}
	// AccessRight
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_DESIREACC,
						REG_DWORD,
						&AddTargetData->DesiredAccess,
						sizeof(AddTargetData->DesiredAccess)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_DESIREACC));
		return	status;
	}
	// Target blocks
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_TARGETBLKS,
						REG_BINARY,
						&AddTargetData->ulTargetBlocks,
						sizeof(AddTargetData->ulTargetBlocks)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_TARGETBLKS));
		return	status;
	}
	// Content encryption method
	tempUlong = AddTargetData->CntEcrMethod;
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_CNTECR,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECR));
		return	status;
	}
	// Content encryption method
	tempUlong = AddTargetData->CntEcrKeyLength;
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_CNTECRKEYLEN,
						REG_DWORD,
						&tempUlong,
						sizeof(tempUlong)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECRKEYLEN));
		return	status;
	}
	// Content encryption key
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_CNTECRKEY,
						REG_BINARY,
						&AddTargetData->CntEcrKey,
						sizeof(AddTargetData->CntEcrKey)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_CNTECRKEY));
		return	status;
	}
	//	LUR flags
	status = DrWriteKeyValue(
		NdasTargetKey,
		NDBUS_REG_LURFLAGS,
		REG_DWORD,
		&AddTargetData->LurFlags,
		sizeof(AddTargetData->LurFlags)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LURFLAGS));
		return	status;
	}
	//	LUR options
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_LUROPTIONS,
						REG_DWORD,
						&AddTargetData->LurOptions,
						sizeof(AddTargetData->LurOptions)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_LUROPTIONS));
		return	status;
	}
	//	RAID: sectors per bit
	status = DrWriteKeyValue(
		NdasTargetKey,
		NDBUS_REG_RAID_SPB,
		REG_DWORD,
		&AddTargetData->RAID_Info.SectorsPerBit,
		sizeof(AddTargetData->RAID_Info.SectorsPerBit)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SPB));
		return	status;
	}

	//	Spare disk count
	status = DrWriteKeyValue(
		NdasTargetKey,
		NDBUS_REG_RAID_SPARE,
		REG_DWORD,
		&AddTargetData->RAID_Info.nSpareDisk,
		sizeof(AddTargetData->RAID_Info.nSpareDisk)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_RAID_SPARE));
		return	status;
	}	


	//	NDAS unit count
	status = DrWriteKeyValue(
						NdasTargetKey,
						NDBUS_REG_UNITCNT,
						REG_DWORD,
						&AddTargetData->ulNumberOfUnitDiskList,
						sizeof(AddTargetData->ulNumberOfUnitDiskList)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_UNITCNT));
		return	status;
	}

	//
	//	Write NDAS units
	//
	nameBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, 512, NDBUSREG_POOLTAG_NAMEBUFF);
	if(!nameBuffer) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ExAllocatePoolWithTag() failed.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	objectName.Length = 0;
	objectName.MaximumLength = 512;
	objectName.Buffer = nameBuffer;

	for(idxKey = 0 ; idxKey < AddTargetData->ulNumberOfUnitDiskList; idxKey ++) {
		unit = AddTargetData->UnitDiskList + idxKey;

		status = Reg_OpenUnit(	&unitKey,
								idxKey,
								TRUE,
								NdasTargetKey);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenUnit() failed. NTSTATUS:%08lx\n", status));
			break;
		}

		status = WriteUnitToRegistry(unitKey, unit);

		ZwClose(unitKey);

		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("WriteUnitToRegistry() failed. NTSTATUS:%08lx\n", status));
			break;
		}

	}

	ExFreePool(nameBuffer);

	return status;
}


//
//	Write an NDAS device to the registry.
//	NOTE: It does not write any target information.
//

NTSTATUS
WriteNDASDevToRegistry(
	HANDLE							NdasDevInst,
	PBUSENUM_PLUGIN_HARDWARE_EX2	Plugin
){

	NTSTATUS					status;


	//	Slot number
	status = DrWriteKeyValue(
					NdasDevInst,
					NDBUS_REG_SLOTNO,
					REG_DWORD,
					&Plugin->SlotNo,
					sizeof(Plugin->SlotNo)
				);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_SLOTNO));
		goto cleanup;
	}

	//	Max request blocks
	status = DrWriteKeyValue(
					NdasDevInst,
					NDBUS_REG_DEVMAXBPR,
					REG_DWORD,
					&Plugin->MaxRequestBlocks,
					sizeof(Plugin->MaxRequestBlocks)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_DEVMAXBPR));
		goto cleanup;
	}

	//	Hardware ID length
	status = DrWriteKeyValue(
					NdasDevInst,
					NDBUS_REG_HWIDLEN,
					REG_DWORD,
					&Plugin->HardwareIDLen,
					sizeof(Plugin->HardwareIDLen)
					);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWIDLEN));
		goto cleanup;
	}

	//	HardwareIDs
	status = DrWriteKeyValue(
					NdasDevInst,
					NDBUS_REG_HWIDS,
					REG_MULTI_SZ,
					&Plugin->HardwareIDs,
					Plugin->HardwareIDLen * sizeof(WCHAR)
		);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_HWIDS));
		goto cleanup;
	}

	//	Flags
	status = DrWriteKeyValue(
					NdasDevInst,
					NDBUS_REG_FLAGS,
					REG_DWORD,
					&Plugin->Flags,
					sizeof(Plugin->Flags)
				);
	if(!NT_SUCCESS(status)) {
		Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("DrWriteKeyValue() failed. ValueKeyName:%ws\n", NDBUS_REG_FLAGS));
		goto cleanup;
	}


cleanup:
	return status;
}


//
//	Allocate AddTargetData and read the registry.
//

NTSTATUS
ReadTargetInstantly(
	HANDLE						NdasDevInst,
	ULONG						TargetId,
	PLANSCSI_ADD_TARGET_DATA	*AddTargetData
){
	NTSTATUS					status;
	HANDLE						targetKey;
	ULONG						outLength;
	PLANSCSI_ADD_TARGET_DATA	addTargetData;

	do {

		status =Reg_OpenTarget(&targetKey, TargetId, FALSE, NdasDevInst);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenTarget() failed. NTSTATUS:%08lx\n", status));
			break;
		}

		status = ReadTargetAndUnitFromRegistry(targetKey, 0, NULL, &outLength);
		if(status != STATUS_BUFFER_TOO_SMALL) {
			ZwClose(targetKey);
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ReadTargetAndUnitFromRegistry() failed. NTSTATUS:%08lx\n", status));
			break;
		}
		addTargetData = ExAllocatePoolWithTag(PagedPool, outLength, NDBUSREG_POOLTAG_ADDTARGET);
		if(addTargetData == NULL) {
			ZwClose(targetKey);
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ExAllocatePoolWithTag() failed. NTSTATUS:%08lx\n", status));
			break;
		}
		status = ReadTargetAndUnitFromRegistry(targetKey, outLength, addTargetData,  &outLength);
		if(!NT_SUCCESS(status)) {
			ExFreePool(addTargetData);
			ZwClose(targetKey);
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("ReadTargetAndUnitFromRegistry() failed. NTSTATUS:%08lx\n", status));
			break;
		}


		//
		//	Set return values
		//

		*AddTargetData = addTargetData;

	} while(FALSE);

	return status;
}


NTSTATUS
RewriteTargetInstantly(
	HANDLE						NdasDevInst,
	ULONG						TargetId,
	PLANSCSI_ADD_TARGET_DATA	AddTargetData
){
	NTSTATUS status;
	HANDLE	targetKey;

	do {

		//
		//	Clean the target registry to rewrite AddTargetData
		//
		status = Reg_OpenTarget(&targetKey, TargetId, FALSE, NdasDevInst);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("Reg_OpenTarget() failed. NTSTATUS:%08lx\n", status));
			break;
		}
		DrDeleteAllSubKeys(targetKey);


		//
		//	Rewrite AddTargetData
		//
		status = WriteTargetToRegistry(targetKey, AddTargetData);
		if(!NT_SUCCESS(status)) {
			Bus_KdPrint_Def(BUS_DBG_SS_ERROR, ("WriteTargetToRegistry() failed. NTSTATUS:%08lx\n", status));
			break;
		}

		ZwClose(targetKey);
	} while(FALSE);

	return status;
}

//////////////////////////////////////////////////////////////////////////
//
//	Plugin NDAS devices in the registry
//


//
//	Enumerate NDAS devices by reading registry.
//

NTSTATUS
EnumerateByRegistry(
		PFDO_DEVICE_DATA	FdoData,
		HANDLE				NdasDeviceReg,
		PTA_ADDRESS			AddedAddress,
		ULONG				PlugInTimeMask
	) {

	NTSTATUS					status;
	PKEY_BASIC_INFORMATION		keyInfo;
	ULONG						outLength;
	LONG						idxKey;
	OBJECT_ATTRIBUTES			objectAttributes;
	UNICODE_STRING				objectName;
	HANDLE						ndas);
		pdoData->HardwareIDs = NULL;
		IoDeleteDevice(pdo);

		ObDereferenceObject(pEvent);
		ObDereferenceObject(pAlarmEvent);

		return status;
	}



    //
    // Device Relation changes if a new pdo is created. So let
    // the PNP system know about that. This forces it to send bunch of pnp 
    // queries and cause the function driver to be loaded.
    //

    IoInvalidateDeviceRelations (FdoData->UnderlyingPDO, BusRelations);

	return status;
}


NTSTATUS
Bus_UnPlugDevice (
    PBUSENUM_UNPLUG_HARDWARE   UnPlug,
    PFDO_DEVICE_DATA            FdoData
    )
/*++
Routine Description:
    The application has told us a device has departed from the bus.
    
    We therefore need to flag the PDO as no longer present
    and then tell Plug and Play about it.
    
Arguments:

    Remove   - pointer to remove hardware structure.
    FdoData - contains the list to iterate over                    
                    
Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful

--*/
{
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;
    BOOLEAN             found = FALSE, plugOutAll;

    PAGED_CODE ();

    plugOutAll = (0 == UnPlug->SlotNo);
              
    //
    // Make sure the thread in which we execute cannot get
    // suspended in APC while we own the global resource.
    //

	KeEnterCriticalRegion();

    ExAcquireFastMutex (&FdoData->Mutex);

    if (plugOutAll) {
        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("Plugging out all the devices!\n"));
    } else {
        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("Plugging out %d\n", UnPlug->SlotNo));
    }

    if (FdoData->NumPDOs == 0) {
        //
        // We got a 2nd plugout...somebody in user space isn't playing nice!!!
        //
        Bus_KdPrint (FdoData, BUS_DBG_PNP_ERROR,
                      ("BAD BAD BAD...2 removes!!! Send only one!\n"));
        ExReleaseFastMutex (&FdoData->Mutex);
		

		KeLeaveCriticalRegion();
        
		return STATUS_NO_SUCH_DEVICE;
    }

    for (entry = FdoData->ListOfPDOs.Flink;
         entry != &FdoData->ListOfPDOs;
         entry = entry->Flink) {

        pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);

        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("found device %d\n", pdoData->SlotNo));

        if (plugOutAll || UnPlug->SlotNo == pdoData->SlotNo) {
            Bus_KdPrint (FdoData, BUS_DBG_PNP_INFO,
                          ("Plugged out %d\n", pdoData->SlotNo));

            pdoData->Present = FALSE;
            found = TRUE;
            if (!plugOutAll) {
                break;
            }
        }
    }

    ExReleaseFastMutex (&FdoData->Mutex);
	
	KeLeaveCriticalRegion();
    
    if (found) {
        IoInvalidateDeviceRelations (FdoData->UnderlyingPDO, BusRelations);
        return STATUS_SUCCESS;
    }

    Bus_KdPrint (FdoData, BUS_DBG_PNP_ERROR,
                  ("Device %d is not present\n", UnPlug->SlotNo));
                  
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
Bus_EjectDevice (
    PBUSENUM_EJECT_HARDWARE     Eject,
    PFDO_DEVICE_DATA            FdoData
    )
/*++
Routine Description:
    The user application has told us to eject the device from the bus.
    In a real situation the driver gets notified by an interrupt when the
    user presses the Eject button on the device.
    
Arguments:

    Eject   - pointer to Eject hardware structure.
    FdoData - contains the list to iterate over                    
                    
Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful
    
--*/
{
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;
    BOOLEAN             found = FALSE, ejectAll;

    PAGED_CODE ();

    ejectAll = (0 == Eject->SlotNo);
              
    //
    // Make sure the thread in which we execute cannot get
    // suspended in APC while we own the global resource.
    //
    KeEnterCriticalRegion();
    ExAcquireFastMutex (&FdoData->Mutex);

    if (ejectAll) {
        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("Ejecting all the pdos!\n"));
    } else {
        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("Ejecting %d\n", Eject->SlotNo));
    }

    if (FdoData->NumPDOs == 0) {
        //
        // Somebody in user space isn't playing nice!!!
        //
        Bus_KdPrint (FdoData, BUS_DBG_PNP_ERROR,
                      ("No devices to eject!\n"));
        ExReleaseFastMutex (&FdoData->Mutex);
        KeLeaveCriticalRegion();


        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Scan the list to find matching PDOs
    //
    for (entry = FdoData->ListOfPDOs.Flink;
         entry != &FdoData->ListOfPDOs;
         entry = entry->Flink) {

        pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);

        Bus_KdPrint (FdoData, BUS_DBG_PNP_NOISE,
                      ("found device %d\n", pdoData->SlotNo));

        if (ejectAll || Eject->SlotNo == pdoData->SlotNo) {
            Bus_KdPrint (FdoData, BUS_DBG_PNP_INFO,
                          ("Ejected %d\n", pdoData->SlotNo));
            found = TRUE;
            IoRequestDeviceEject(pdoData->Self);           
            if (!ejectAll) {
                break;
            }
        }
    }
    ExReleaseFastMutex (&FdoData->Mutex);
    KeLeaveCriticalRegion();

    
    if (found) {
        return STATUS_SUCCESS;
    }

    Bus_KdPrint (FdoData, BUS_DBG_PNP_ERROR,
                  ("Device %d is not present\n", Eject->SlotNo));
                  
    return STATUS_INVALID_PARAMETER;
}

#if DBG

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";           
        default:
            return "IRP_MN_?????";
    }
}

PCHAR 
DbgDeviceRelationString(
    IN DEVICE_RELATION_TYPE Type
    )
{  
    switch (Type)
    {
        case BusRelations:
            return "BusRelations";
        case EjectionRelations:
            return "EjectionRelations";
        case RemovalRelations:
            return "RemovalRelations";
        case TargetDeviceRelation:
            return "TargetDeviceRelation";
        default:
            return "UnKnown Relation";
    }
}

PCHAR 
DbgDeviceIDString(
    BUS_QUERY_ID_TYPE Type
    )
{
    switch (Type)
    {
        case BusQueryDeviceID:
            return "BusQueryDeviceID";
        case BusQueryHardwareIDs:
            return "BusQueryHardwareIDs";
        case BusQueryCompatibleIDs:
            return "BusQueryCompatibleIDs";
        case BusQueryInstanceID:
            return "BusQueryInstanceID";
        case BusQueryDeviceSerialNumber:
            return "BusQueryDeviceSerialNumber";
        default:
            return "UnKnown ID";
    }
}

#endif

