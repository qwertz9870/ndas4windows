#include "stdafx.h"
#include "ndas/ndasdib.h"
#include "ndas/ndasmsg.h"
#include "scrc32.h"
#include "des.h"
#include "lurdesc.h"
#include "ndasdev.h"
#include "ndasunitdevfactory.h"
#include "autores.h"
#include "xdebug.h"
#include "eventlog.h"

static
NDAS_LOGICALDEVICE_TYPE
pGetNdasLogicalDeviceTypeFromDIBv2(CONST NDAS_DIB_V2* pDIBV2);

static
NDAS_UNITDEVICE_DISK_TYPE
pGetNdasUnitDiskTypeFromDIBv2(CONST NDAS_DIB_V2* pDIBV2);

//////////////////////////////////////////////////////////////////////////
//
// NDAS Unit Device Instance Creator
//
//////////////////////////////////////////////////////////////////////////

CNdasUnitDeviceCreator::CNdasUnitDeviceCreator(
	CNdasDevice& device, DWORD dwUnitNo) :
	m_pDevice(&device),
	m_dwUnitNo(dwUnitNo),
	m_devComm(device, dwUnitNo)
{
	::ZeroMemory(&m_unitDevInfo, sizeof(NDAS_UNITDEVICE_INFORMATION));
}

CNdasUnitDevice*
CNdasUnitDeviceCreator::CreateUnitDevice()
{
	CNdasUnitDevice* pUnitDevice = NULL;

	BOOL fSuccess = m_devComm.Initialize();
	if (!fSuccess) {
		DBGPRT_ERR_EX(
			_FT("Communication initialization to %s[%d] failed: "),
			m_pDevice->ToString(), 
			m_dwUnitNo);
		return NULL;
	}

	//
	// Discover unit device information
	//
	fSuccess = m_devComm.GetUnitDeviceInformation(&m_unitDevInfo);
	if (!fSuccess) {
		DBGPRT_ERR_EX(_FT("GetUnitDeviceInformation of %s[%d] failed: "), 
			m_pDevice->ToString(), 
			m_dwUnitNo);
		return NULL;
	}


	if (NDAS_UNITDEVICE_MEDIA_TYPE_BLOCK_DEVICE == m_unitDevInfo.MediaType) {

		pUnitDevice = CreateUnitDiskDevice();

	} else if (NDAS_UNITDEVICE_MEDIA_TYPE_COMPACT_BLOCK_DEVICE == m_unitDevInfo.MediaType) {

		// Not implemented yet
		// pUnitDevice = CreateCompactBlockDevice()

		NDAS_LOGICALDEVICE_GROUP ldGroup = {0};
		ldGroup.nUnitDevices = 1;
		ldGroup.Type = NDAS_LOGICALDEVICE_TYPE_FLASHCARD;
		ldGroup.UnitDevices[0].DeviceId = m_pDevice->GetDeviceId();
		ldGroup.UnitDevices[0].UnitNo = m_dwUnitNo;

		pUnitDevice = new CNdasUnitDevice(
			*m_pDevice,
			m_dwUnitNo,
			NDAS_UNITDEVICE_TYPE_COMPACT_BLOCK,
			CreateNdasUnitDeviceSubType(NDAS_UNITDEVICE_COMPACT_BLOCK_TYPE_FLASHCARD),
			m_unitDevInfo,
			ldGroup,
			0);

		if (NULL != pUnitDevice) pUnitDevice->AddRef();

	} else if (NDAS_UNITDEVICE_MEDIA_TYPE_CDROM_DEVICE == m_unitDevInfo.MediaType) {

		NDAS_LOGICALDEVICE_GROUP ldGroup = {0};
		ldGroup.nUnitDevices = 1;
		ldGroup.Type = NDAS_LOGICALDEVICE_TYPE_DVD;
		ldGroup.UnitDevices[0].DeviceId = m_pDevice->GetDeviceId();
		ldGroup.UnitDevices[0].UnitNo = m_dwUnitNo;

		pUnitDevice = new CNdasUnitDevice(
			*m_pDevice,
			m_dwUnitNo,
			NDAS_UNITDEVICE_TYPE_CDROM,
			CreateNdasUnitDeviceSubType(NDAS_UNITDEVICE_CDROM_TYPE_DVD),
			m_unitDevInfo,
			ldGroup,
			0);

		if (NULL != pUnitDevice) pUnitDevice->AddRef();

	} else if (NDAS_UNITDEVICE_MEDIA_TYPE_OPMEM_DEVICE == m_unitDevInfo.MediaType) {

		NDAS_LOGICALDEVICE_GROUP ldGroup = {0};
		ldGroup.nUnitDevices = 1;
		ldGroup.Type = NDAS_LOGICALDEVICE_TYPE_DVD;
		ldGroup.UnitDevices[0].DeviceId = m_pDevice->GetDeviceId();
		ldGroup.UnitDevices[0].UnitNo = m_dwUnitNo;

		pUnitDevice = new CNdasUnitDevice(
			*m_pDevice,
			m_dwUnitNo,
			NDAS_UNITDEVICE_TYPE_OPTICAL_MEMORY,
			CreateNdasUnitDeviceSubType(NDAS_UNITDEVICE_OPTICAL_MEMORY_TYPE_MO),
			m_unitDevInfo,
			ldGroup,
			0);

		if (NULL != pUnitDevice) pUnitDevice->AddRef();

	} else {

		_ASSERTE(FALSE);
		return NULL;

	}

	return pUnitDevice;
}

CNdasUnitDiskDevice*
CNdasUnitDeviceCreator::CreateUnitDiskDevice()
{
	//
	// Read DIB
	//
	// Read DIB should success, even if the unit disk does not contain
	// the DIB. If it fails, there should be some communication error.
	//

	BOOL fSuccess = FALSE;
	PNDAS_DIB_V2 pDIBv2 = NULL;

	//
	// ReadDIB is to allocate pDIBv2
	//
	fSuccess = ReadDIB(&pDIBv2);

	// attach to auto heap 
	AutoProcessHeap autoDIB = (LPVOID) pDIBv2;

	if (!fSuccess) {
		DBGPRT_ERR_EX(_FT("Reading or creating DIBv2 failed: "));
		DPError(_FT("Creating unit disk device instance failed.\n"));
		return NULL;
	}

	NDAS_UNITDEVICE_DISK_TYPE diskType = pGetNdasUnitDiskTypeFromDIBv2(pDIBv2);
	if (NDAS_UNITDEVICE_DISK_TYPE_UNKNOWN == diskType) {
		//
		// Error! Invalid media type
		//
		DBGPRT_ERR_EX(_FT("Invalid media type %d(0x%08X)"), 
			pDIBv2->iMediaType,
			pDIBv2->iMediaType);

		//
		// we should create generic unknown unit disk device
		//
	}

	// return NULL;
	LPVOID pAddTargetInfo = NULL;

	if (NMT_RAID1 == pDIBv2->iMediaType) {
		//
		// do not allocate with "new INFO_RAID1"
		// destructor will delete with "HeapFree"
		//

		pAddTargetInfo = ::HeapAlloc(
			::GetProcessHeap(), 
			HEAP_ZERO_MEMORY, 
			sizeof(INFO_RAID1));

		PINFO_RAID1 pIR = reinterpret_cast<PINFO_RAID1>(pAddTargetInfo);

		pIR->SectorsPerBit = pDIBv2->iSectorsPerBit;
		//			pIR->OffsetFlagInfo = (UINT32)(&((PNDAS_DIB_V2)NULL)->FlagDirty);
		pIR->SectorBitmapStart = m_unitDevInfo.SectorCount - 0x0f00;
		pIR->SectorInfo = m_unitDevInfo.SectorCount - 0x0002;
		pIR->SectorLastWrittenInfo = m_unitDevInfo.SectorCount - 0x1000;
	} else if (NMT_RAID4 == pDIBv2->iMediaType) {
		//
		// do not allocate with "new INFO_RAID4"
		// destructor will delete with "HeapFree"
		//

		pAddTargetInfo = ::HeapAlloc(
			::GetProcessHeap(), 
			HEAP_ZERO_MEMORY, 
			sizeof(INFO_RAID4));

		PINFO_RAID4 pIR = reinterpret_cast<PINFO_RAID4>(pAddTargetInfo);

		pIR->SectorsPerBit = pDIBv2->iSectorsPerBit;
		//			pIR->OffsetFlagInfo = (UINT32)(&((PNDAS_DIB_V2)NULL)->FlagDirty);
		pIR->SectorBitmapStart = m_unitDevInfo.SectorCount - 0x0f00;
		pIR->SectorInfo = m_unitDevInfo.SectorCount - 0x0002;
		pIR->SectorLastWrittenInfo = m_unitDevInfo.SectorCount - 0x1000;
	}

	NDAS_UNITDEVICE_ID udid;
	udid.DeviceId;
	udid.UnitNo;

	ULONG ulUserBlocks = pDIBv2->sizeUserSpace;
	DWORD ldSequence = pDIBv2->iSequence;
	NDAS_LOGICALDEVICE_GROUP ldGroup = {0};
	ldGroup.Type = pGetNdasLogicalDeviceTypeFromDIBv2(pDIBv2);
	ldGroup.nUnitDevices = pDIBv2->nDiskCount;
	for(DWORD i = 0; i < ldGroup.nUnitDevices; i++)
	{
		::CopyMemory(
			&ldGroup.UnitDevices[i].DeviceId, 
			pDIBv2->UnitDisks[i].MACAddr, 
			sizeof(NDAS_DEVICE_ID));
		ldGroup.UnitDevices[i].UnitNo = pDIBv2->UnitDisks[i].UnitNumber;
	}

	//
	// Read CONTENT_ENCRYPT
	//
	// Read CONTENT_ENCRYPT should success, even if the unit disk does not contain
	// the CONTENT_ENCRYPT. If it fails, there should be some communication error.
	//

	NDAS_CONTENT_ENCRYPT_BLOCK ceb = {0};
	NDAS_CONTENT_ENCRYPT ce = {0};

	fSuccess = ReadContentEncryptBlock(&ceb);

	if (!fSuccess) {
		DBGPRT_ERR_EX(_FT("Reading or ContentEncrypt failed: "));
		DPError(_FT("Creating unit disk device instance failed.\n"));
		(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_CEB_READ_FAILURE);
		return NULL;
	}

	UINT uiRet = ::NdasEncVerifyContentEncryptBlock(&ceb);

	if (NDASENC_ERROR_CEB_INVALID_SIGNATURE == uiRet) {
		// No Content Encryption
		// Safe to ignore
		ce.Method = NDAS_CONTENT_ENCRYPT_METHOD_NONE;
	} else if (NDASENC_ERROR_CEB_INVALID_CRC == uiRet) {
		// No Content Encryption
		// Safe to ignore
		ce.Method = NDAS_CONTENT_ENCRYPT_METHOD_NONE;
	} else if (NDASENC_ERROR_CEB_UNSUPPORTED_REVISION == uiRet) {
		// Error !
		(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_CEB_UNSUPPORTED_REVISION, uiRet);
		return NULL;
	} else if (NDASENC_ERROR_CEB_INVALID_KEY_LENGTH == uiRet) {
		// No Content Encryption
		(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_CEB_INVALID_KEY_LENGTH, uiRet);
		ce.Method = NDAS_CONTENT_ENCRYPT_METHOD_NONE;
	} else if (ERROR_SUCCESS != uiRet) {
		// No Content Encryption
		ce.Method = NDAS_CONTENT_ENCRYPT_METHOD_NONE;
	} else {

		//
		// We consider the case that ENCRYPTION is not NONE.
		//

		if (NDAS_CONTENT_ENCRYPT_METHOD_NONE != ceb.Method) {

			BYTE SysKey[16] = {0};
			DWORD cbSysKey = sizeof(SysKey);

			uiRet = ::NdasEncGetSysKey(cbSysKey, SysKey, &cbSysKey);

			if (ERROR_SUCCESS != uiRet) {
				(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_GET_SYS_KEY, uiRet);
				return NULL;
			}

			uiRet = ::NdasEncVerifyFingerprintCEB(SysKey, cbSysKey, &ceb);

			if (ERROR_SUCCESS != uiRet) {
				(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_SYS_KEY_MISMATCH, uiRet);
				return NULL;
			}

			//
			// Create Content Encrypt
			//
			ce.Method = ceb.Method;
			ce.KeyLength = ceb.KeyLength;
			ce.Key;

			uiRet = ::NdasEncCreateContentEncryptKey(
				SysKey, 
				cbSysKey, 
				ceb.Key, 
				ceb.KeyLength, 
				ce.Key,
				sizeof(ce.Key));

			if (ERROR_SUCCESS != uiRet) {
				(VOID) ::NdasLogEventError2(EVT_NDASSVC_ERROR_KEY_GENERATION_FAILURE, uiRet);
				return NULL;
			}

		}

	}

	CNdasUnitDiskDevice* pUnitDiskDevice = 
		new CNdasUnitDiskDevice(
			*m_pDevice, 
			m_dwUnitNo,
			diskType,
			m_unitDevInfo,
			ldGroup,
			ldSequence,
			ulUserBlocks,
			pAddTargetInfo,
			ce,
			pDIBv2);

	if (NULL != pUnitDiskDevice) {
		//
		// We should detach pDIBv2 from being freeed here
		// pUnitDiskDevice will take care of it at dctor
		//
		autoDIB.Detach();
		pUnitDiskDevice->AddRef();
	}

	return pUnitDiskDevice;
}

BOOL
CNdasUnitDeviceCreator::ReadContentEncryptBlock(
	PNDAS_CONTENT_ENCRYPT_BLOCK pCEB)
{
	BOOL fSuccess = m_devComm.ReadDiskBlock(
		reinterpret_cast<PBYTE>(pCEB),
		NDAS_BLOCK_LOCATION_ENCRYPT);

	if(!fSuccess) {
		DBGPRT_ERR_EX(_FT("Unable to read CEB: "));
		return FALSE;
	}

	return TRUE;
}

BOOL
CNdasUnitDeviceCreator::ReadDIB(PNDAS_DIB_V2* ppDIBv2)
{
	//
	// ppDIBv2 will be set only if this function succeed.
	//

	PNDAS_DIB_V2 pDIBv2 = reinterpret_cast<PNDAS_DIB_V2>(
		::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, 512));

	if (NULL == pDIBv2) {
		//
		// Out of memory!
		//
		return FALSE;
	}

	BOOL fSuccess = m_devComm.ReadDiskBlock(
		reinterpret_cast<PBYTE>(pDIBv2),
		NDAS_BLOCK_LOCATION_DIB_V2);

	//
	// Regardless of the existence,
	// Disk Block should be read.
	// Failure means communication error or disk error
	//
	if (!fSuccess) {
		::HeapFree(::GetProcessHeap(), 0, pDIBv2);
		DPError(_FT("ReadDiskBlock failed.\n"));
		return FALSE;
	}

	//
	// check signature
	//

	if(NDAS_DIB_V2_SIGNATURE != pDIBv2->Signature ||
		pDIBv2->crc32 != crc32_calc((unsigned char *)pDIBv2, 
		sizeof(pDIBv2->bytes_248)) ||
		pDIBv2->crc32_unitdisks != crc32_calc((unsigned char *)pDIBv2->UnitDisks, 
		sizeof(pDIBv2->UnitDisks))) {
		//
		// Read DIBv1
		//
		
		fSuccess = ReadDIBv1AndConvert(pDIBv2);

		if (!fSuccess) {
			::HeapFree(::GetProcessHeap(), 0, pDIBv2);
			return FALSE;
		}

		*ppDIBv2 = pDIBv2;
		return TRUE;
	}

	//
	// check version
	//
	if(IS_HIGHER_VERSION_V2(*pDIBv2))
	{
		::HeapFree(::GetProcessHeap(), 0, pDIBv2);
		DPError(_FT("Unsupported version V2 failed"));
		return FALSE;
	}

	//
	// TODO: Lower version process (future code) ???
	//
	if(0)
	{
		DPError(_FT("lower version V2 detected"));
	}

	//
	// read additional locations if needed
	//
	if (pDIBv2->nDiskCount > NDAS_MAX_UNITS_IN_V2) {

		UINT32 nTrailSectorCount = 
			GET_TRAIL_SECTOR_COUNT_V2(pDIBv2->nDiskCount);

		SIZE_T dwBytes = sizeof(NDAS_DIB_V2) + 512 * nTrailSectorCount;

		LPVOID ptr = ::HeapReAlloc(
				::GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				pDIBv2, 
				dwBytes);

		if (NULL == ptr) {
			DBGPRT_ERR_EX(_FT("Memory allocation failed!!!:"));
			_ASSERTE(FALSE);
			return FALSE;
		}

		pDIBv2 = reinterpret_cast<PNDAS_DIB_V2>(ptr);

		for(DWORD i = 0; i < nTrailSectorCount; i++) {

			fSuccess = m_devComm.ReadDiskBlock(
				reinterpret_cast<PBYTE>(pDIBv2) + sizeof(NDAS_DIB_V2) + 512 * i,
				NDAS_BLOCK_LOCATION_ADD_BIND + i);

			if(!fSuccess) {
				DPError(_FT("Reading additional block failed %d"), 
					NDAS_BLOCK_LOCATION_ADD_BIND + i);
				::HeapFree(::GetProcessHeap, 0, pDIBv2);
				return FALSE;
			}
		}
	}

	// Virtual DVD check. Not supported ATM.

	*ppDIBv2 = pDIBv2;

	return TRUE;
}

BOOL 
CNdasUnitDeviceCreator::ReadDIBv1AndConvert(PNDAS_DIB_V2 pDIBv2)
{
	BOOL fSuccess = FALSE;
	NDAS_DIB DIBv1 = {0};
	PNDAS_DIB pDIBv1 = &DIBv1;

	fSuccess = m_devComm.ReadDiskBlock(
		reinterpret_cast<PBYTE>(pDIBv1), 
		NDAS_BLOCK_LOCATION_DIB_V1);

	if (!fSuccess) {
		DBGPRT_ERR_EX(_FT("Reading DIBv1 block failed: "));
		return FALSE;
	}

	//
	// If there is no DIB in the disk,
	// create a pseudo DIBv2
	//
	if (NDAS_DIB_SIGNATURE != pDIBv1->Signature ||
		IS_NDAS_DIBV1_WRONG_VERSION(*pDIBv1)) 
	{
		//
		// Create a pseudo DIBv2
		//
		InitializeDIBv2(pDIBv2, m_devComm.GetDiskSectorCount());

		::CopyMemory(
			pDIBv2->UnitDisks[0].MACAddr, 
			m_pDevice->GetDeviceId().Node, 
			6);

		pDIBv2->UnitDisks[0].UnitNumber = m_dwUnitNo;

		return TRUE;
	}

	//
	// Convert V1 to V2
	//
	fSuccess = ConvertDIBv1toDIBv2(
		pDIBv1, 
		pDIBv2, 
		m_devComm.GetDiskSectorCount());

	if (!fSuccess) {

		//
		// Create a pseudo DIBv2 again!
		//
		InitializeDIBv2(pDIBv2, m_devComm.GetDiskSectorCount());

		::CopyMemory(
			pDIBv2->UnitDisks[0].MACAddr, 
			m_pDevice->GetDeviceId().Node, 
			6);

		pDIBv2->UnitDisks[0].UnitNumber = m_dwUnitNo;

		return TRUE;
	}

	return TRUE;
}

VOID 
CNdasUnitDeviceCreator::InitializeDIBv2(
	PNDAS_DIB_V2 pDIBv2, 
	UINT64 nDiskSectorCount)
{
	_ASSERTE(!::IsBadWritePtr(pDIBv2, sizeof(NDAS_DIB_V2)));

	::ZeroMemory(pDIBv2, sizeof(NDAS_DIB_V2));

	pDIBv2->Signature = NDAS_DIB_V2_SIGNATURE;
	pDIBv2->MajorVersion = NDAS_DIB_VERSION_MAJOR_V2;
	pDIBv2->MinorVersion = NDAS_DIB_VERSION_MINOR_V2;
	pDIBv2->sizeXArea = 2 * 1024 * 2;
	pDIBv2->sizeUserSpace = nDiskSectorCount - pDIBv2->sizeXArea;
	pDIBv2->iSectorsPerBit = 128;
	pDIBv2->sizeUserSpace -= pDIBv2->sizeUserSpace % pDIBv2->iSectorsPerBit;
	pDIBv2->iMediaType = NMT_SINGLE;
	pDIBv2->iSequence = 0;
	pDIBv2->nDiskCount = 1;
//	pDIBv2->FlagDirty = 0;

}

BOOL
CNdasUnitDeviceCreator::ConvertDIBv1toDIBv2(
	CONST NDAS_DIB* pDIBv1, 
	NDAS_DIB_V2* pDIBv2, 
	UINT64 nDiskSectorCount)
{
	_ASSERTE(!::IsBadReadPtr(pDIBv1, sizeof(NDAS_DIB)));
	_ASSERTE(!::IsBadWritePtr(pDIBv2, sizeof(NDAS_DIB_V2)));

	InitializeDIBv2(pDIBv2, nDiskSectorCount);
	// fit to old system
	pDIBv2->sizeUserSpace = nDiskSectorCount - pDIBv2->sizeXArea;
	pDIBv2->iSectorsPerBit = 0; // no backup information

	// single disk
	if(IS_NDAS_DIBV1_WRONG_VERSION(*pDIBv1) || // no DIB information
		NDAS_DIB_DISK_TYPE_SINGLE == pDIBv1->DiskType)
	{
		pDIBv2->iMediaType = NMT_SINGLE;
		pDIBv2->iSequence = 0;
		pDIBv2->nDiskCount = 1;
		ZeroMemory(&pDIBv2->UnitDisks[0], sizeof(pDIBv2->UnitDisks[0]));

		::CopyMemory(
			pDIBv2->UnitDisks[0].MACAddr, 
			m_pDevice->GetRemoteLpxAddress().Node, 
			sizeof(m_pDevice->GetRemoteLpxAddress().Node));
	}
	else
	{
		// pair(2) disks (mirror, aggregation)
		UNIT_DISK_LOCATION *pUnitDiskLocation0, *pUnitDiskLocation1;
		if(NDAS_DIB_DISK_TYPE_MIRROR_MASTER == pDIBv1->DiskType ||
			NDAS_DIB_DISK_TYPE_AGGREGATION_FIRST == pDIBv1->DiskType)
		{
			pUnitDiskLocation0 = &pDIBv2->UnitDisks[0];
			pUnitDiskLocation1 = &pDIBv2->UnitDisks[1];
		}
		else
		{
			pUnitDiskLocation0 = &pDIBv2->UnitDisks[1];
			pUnitDiskLocation1 = &pDIBv2->UnitDisks[0];
		}

		if(
			0x00 == pDIBv1->EtherAddress[0] &&
			0x00 == pDIBv1->EtherAddress[1] &&
			0x00 == pDIBv1->EtherAddress[2] &&
			0x00 == pDIBv1->EtherAddress[3] &&
			0x00 == pDIBv1->EtherAddress[4] &&
			0x00 == pDIBv1->EtherAddress[5]) 
		{
			// usually, there is no ether address information
			::CopyMemory(
				pUnitDiskLocation0->MACAddr, 
				m_pDevice->GetRemoteLpxAddress().Node, 
				sizeof(m_pDevice->GetRemoteLpxAddress().Node));
			pUnitDiskLocation0->UnitNumber = 0;
		}
		else
		{
			CopyMemory(&pUnitDiskLocation0->MACAddr, pDIBv1->EtherAddress, 
				sizeof(pUnitDiskLocation0->MACAddr));
			pUnitDiskLocation0->UnitNumber = pDIBv1->UnitNumber;
		}
		CopyMemory(pUnitDiskLocation1->MACAddr, pDIBv1->PeerAddress, 
			sizeof(pUnitDiskLocation1->MACAddr));
		pUnitDiskLocation1->UnitNumber = pDIBv1->UnitNumber;

		pDIBv2->nDiskCount = 2;

		switch(pDIBv1->DiskType)
		{
		case NDAS_DIB_DISK_TYPE_MIRROR_MASTER:
			pDIBv2->iMediaType = NMT_MIRROR;
			pDIBv2->iSequence = 0;
			break;
		case NDAS_DIB_DISK_TYPE_MIRROR_SLAVE:
			pDIBv2->iMediaType = NMT_MIRROR;
			pDIBv2->iSequence = 1;
			break;
		case NDAS_DIB_DISK_TYPE_AGGREGATION_FIRST:
			pDIBv2->iMediaType = NMT_AGGREGATE;
			pDIBv2->iSequence = 0;
			break;
		case NDAS_DIB_DISK_TYPE_AGGREGATION_SECOND:
			pDIBv2->iMediaType = NMT_AGGREGATE;
			pDIBv2->iSequence = 1;
			break;
		default:
			return FALSE;
		}
	}

	// write crc
	pDIBv2->crc32 = crc32_calc(
		(unsigned char *)pDIBv2,
		sizeof(pDIBv2->bytes_248));

	pDIBv2->crc32_unitdisks = crc32_calc(
		(unsigned char *)pDIBv2->UnitDisks,
		sizeof(pDIBv2->UnitDisks));

	return TRUE;
}

static
NDAS_LOGICALDEVICE_TYPE
pGetNdasLogicalDeviceTypeFromDIBv2(CONST NDAS_DIB_V2* pDIBV2)
{
	_ASSERTE(NULL != pDIBV2);
	switch (pDIBV2->iMediaType) {
	case NMT_SINGLE:	return NDAS_LOGICALDEVICE_TYPE_DISK_SINGLE;
	case NMT_MIRROR:	return NDAS_LOGICALDEVICE_TYPE_DISK_MIRRORED;
	case NMT_AGGREGATE: return NDAS_LOGICALDEVICE_TYPE_DISK_AGGREGATED;
	case NMT_RAID0:		return NDAS_LOGICALDEVICE_TYPE_DISK_RAID0;
	case NMT_RAID1:		return NDAS_LOGICALDEVICE_TYPE_DISK_RAID1;
	case NMT_RAID4:		return NDAS_LOGICALDEVICE_TYPE_DISK_RAID4;
	case NMT_VDVD:		return NDAS_LOGICALDEVICE_TYPE_VIRTUAL_DVD;
	default:			return NDAS_LOGICALDEVICE_TYPE_UNKNOWN;
	}
}

static
NDAS_UNITDEVICE_DISK_TYPE
pGetNdasUnitDiskTypeFromDIBv2(CONST NDAS_DIB_V2* pDIBV2)
{
	_ASSERTE(NULL != pDIBV2);
	switch (pDIBV2->iMediaType) {
	case NMT_SINGLE:	return NDAS_UNITDEVICE_DISK_TYPE_SINGLE;
	case NMT_MIRROR: 
		return (pDIBV2->iSequence == 0) ?
			NDAS_UNITDEVICE_DISK_TYPE_MIRROR_MASTER :
			NDAS_UNITDEVICE_DISK_TYPE_MIRROR_SLAVE;
	case NMT_AGGREGATE:	return NDAS_UNITDEVICE_DISK_TYPE_AGGREGATED;
	case NMT_RAID0:		return NDAS_UNITDEVICE_DISK_TYPE_RAID0;
	case NMT_RAID1:		return NDAS_UNITDEVICE_DISK_TYPE_RAID1;
	case NMT_RAID4:		return NDAS_UNITDEVICE_DISK_TYPE_RAID4;
	case NMT_VDVD:		return NDAS_UNITDEVICE_DISK_TYPE_VIRTUAL_DVD;
	default:			return NDAS_UNITDEVICE_DISK_TYPE_UNKNOWN;
	}
}
