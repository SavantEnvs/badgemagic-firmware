#include "hw.h"
#ifndef MAYHEM_BLE_STUB_H
#define MAYHEM_BLE_STUB_H
/* GAP/GATT types and calls that src/ble/peripheral.c uses. Only the OTA write path runs on the host;
 * the stack calls are declarations whose bodies (shim.c) do nothing. */
#define GAP_CONNHANDLE_INIT 0xFFFE
#define GAP_ADTYPE_FLAGS 0x01
#define GAP_ADTYPE_16BIT_MORE 0x02
#define GAP_ADTYPE_LOCAL_NAME_COMPLETE 0x09
#define GAP_ADTYPE_POWER_LEVEL 0x0A
#define GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE 0x12
#define GAP_ADTYPE_FLAGS_GENERAL 0x02
#define GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED 0x04
#define LO_UINT16(x) ((x) & 0xFF)
#define HI_UINT16(x) (((x) >> 8) & 0xFF)

#define GAPROLE_ADVERT_ENABLED 0
#define GAPROLE_SCAN_RSP_DATA 1
#define GAPROLE_ADVERT_DATA 2
#define GAPROLE_MIN_CONN_INTERVAL 3
#define GAPROLE_MAX_CONN_INTERVAL 4
#define GAPROLE_STATE_ADV_MASK 0x03
#define GAPROLE_ADVERTISING 1
#define GAPROLE_CONNECTED 2
#define GAPROLE_WAITING 0
#define GAP_LINK_TERMINATED_EVENT 1
#define GAP_LINK_ESTABLISHED_EVENT 2
#define GGS_DEVICE_NAME_ATT 0
#define GATT_ALL_SERVICES 0xFFFFFFFF
#define TGAP_DISC_ADV_INT_MIN 0
#define TGAP_DISC_ADV_INT_MAX 1
#define GAPBOND_PERI_DEFAULT_PASSCODE 0
#define GAPBOND_PERI_PAIRING_MODE 1
#define GAPBOND_PERI_MITM_PROTECTION 2
#define GAPBOND_PERI_IO_CAPABILITIES 3
#define GAPBOND_PERI_BONDING_ENABLED 4
#define GAPBOND_PAIRING_MODE_NO_PAIRING 0
#define GAPBOND_IO_CAP_DISPLAY_ONLY 0

typedef uint8_t gapRole_States_t;
typedef struct { uint8_t event; uint8_t status; } tmos_event_hdr_t;
typedef struct { uint8_t opcode; } gapEvent_t;
typedef struct { uint8_t opcode; uint16_t connectionHandle; } gapLinkCmplEvent_t;
typedef struct { uint8_t opcode; uint16_t connectionHandle; uint16_t connInterval; uint16_t connLatency; uint16_t connTimeout; } gapEstLinkReqEvent_t;
typedef struct { uint8_t opcode; uint16_t connectionHandle; uint8_t reason; } gapTerminateLinkEvent_t;
typedef union {
	gapEvent_t gap;
	gapLinkCmplEvent_t linkCmpl;
} gapRoleEvent_t;
typedef struct {
	void (*pfnStateChange)(gapRole_States_t, gapRoleEvent_t *);
	void *pfnRssiRead;
	void (*pfnParamUpdate)(uint16_t, uint16_t, uint16_t, uint16_t);
} gapRolesCBs_t;
typedef struct { void *pfnScanRecv; void *pfnScanEnd; } gapRolesBroadcasterCBs_t;
typedef struct { void *pfnPasscodeCB; void *pfnPairStateCB; } gapBondCBs_t;

bStatus_t GAPRole_PeripheralInit(void);
bStatus_t GAPRole_SetParameter(uint16_t, uint16_t, void *);
bStatus_t GAPRole_TerminateLink(uint16_t);
bStatus_t GAPRole_PeripheralConnParamUpdateReq(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint8_t);
bStatus_t GAPRole_PeripheralStartDevice(uint8_t, gapBondCBs_t *, gapRolesCBs_t *);
bStatus_t GAPRole_BroadcasterSetCB(gapRolesBroadcasterCBs_t *);
bStatus_t GGS_SetParameter(uint16_t, uint16_t, void *);
bStatus_t GGS_AddService(uint32_t);
bStatus_t GATTServApp_AddService(uint32_t);
bStatus_t GAP_SetParamValue(uint16_t, uint16_t);
bStatus_t GAPBondMgr_SetParameter(uint16_t, uint16_t, void *);
#endif
