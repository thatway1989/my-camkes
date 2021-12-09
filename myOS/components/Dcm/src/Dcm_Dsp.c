/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 * 
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with  
 * the terms contained in the written license agreement between you and ArcCore, 
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as 
 * published by the Free Software Foundation and appearing in the file 
 * LICENSE.GPL included in the packaging of this file or here 
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/

/*
 *  General requirements
 */
//lint -w2 Static code analysis warning level
/* Disable MISRA 2004 rule 16.2, MISRA 2012 rule 17.2.
 * This because of recursive calls to readDidData.
 *  */
//lint -estring(974,*recursive*)

/* @req DCM273 */ 
/* @req DCM272 */ 
/* @req DCM696 */
/* @req DCM039 */
/* @req DCM269 */
/* @req DCM271 */
/* @req DCM275 */
/* !req DCM038 Paged buffer not supported */
/* !req DCM085 */
/* @req DCM531 A jump to bootloader is possible only with services DiagnosticSessionControl and LinkControl services */
/* @req DCM527 At first call of an operation using the Dcm_OpStatusType, OpStatus should be DCM_INITIAL */
/* !req DCM528 E_FORCE_RCRRP not supported return for all operations */
/* !req DCM530 E_PENDING not supported return for all operations */
/* @req DCM077 When calling DEM for OBD services, DCM shall use the following values for the parameter DTCOrigin: Service $0A uses DEM_DTC_ORIGIN_PERMANENT_MEMORY All other services use DEM_DTC_ORIGIN_PRIMARY_MEMORY */
#include <string.h>
#include "Dcm.h"
#include "Dcm_Internal.h"
#if defined(DCM_USE_SERVICE_CLEARDIAGNOSTICINFORMATION) || defined(DCM_USE_SERVICE_READDTCINFORMATION) || defined(DCM_USE_SERVICE_CONTROLDTCSETTING)\
    || defined(DCM_USE_SERVICE_REQUESTPOWERTRAINFREEZEFRAMEDATA) || defined(DCM_USE_SERVICE_CLEAREMISSIONRELATEDDIAGNOSTICDATA)\
    || defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDIAGNOSTICTROUBLECODES) || defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDTCSDETECTEDDURINGCURRENTORLASTCOMPLETEDDRIVINGCYCLE)\
    || defined(DCM_USE_SERVICE_REQUESTEMISSIONRELATEDDTCSWITHPERMANENTSTATUS)
#if defined(USE_DEM)
#include "Dem.h"/* @req DCM332 */
#else
#warning "Dcm: UDS services 0x14, 0x18 and/or 0x85 will not work without Dem."
#warning "Dcm: OBD services $02, $03, $04, $07 and/or $0A will not work without Dem."
#endif
#endif
//#include "MemMap.h"
#if defined(USE_MCU)
#include "Mcu.h"
#endif
#ifndef DCM_NOT_SERVICE_COMPONENT
#include "Rte_Dcm.h"
#endif

#if defined(USE_BSWM)
#include "BswM_DCM.h"
#endif

#if defined(DCM_USE_SERVICE_REQUESTTRANSFEREXIT) || defined(DCM_USE_SERVICE_TRANSFERDATA) || defined(DCM_USE_SERVICE_REQUESTDOWNLOAD) || defined(DCM_USE_SERVICE_REQUESTUPLOAD)
#define DCM_USE_UPLOAD_DOWNLOAD
#endif

/*
 * Macros
 */
#define ZERO_SUB_FUNCTION				0x00
#define DCM_FORMAT_LOW_MASK			0x0F
#define DCM_FORMAT_HIGH_MASK			0xF0
#define DCM_MEMORY_ADDRESS_MASK		0x00FFFFFF
#define DCM_DID_HIGH_MASK 				0xFF00			
#define DCM_DID_LOW_MASK				0xFF
#define DCM_PERODICDID_HIGH_MASK		0xF200
#define SID_AND_DDDID_LEN   0x4
#define SDI_AND_MS_LEN   0x4

#define SID_AND_SDI_LEN   0x6
#define SID_AND_PISDR_LEN   0x7

/* == Parser macros == */
/* General */
#define SID_INDEX 0
#define SID_LEN 1u
#define SF_INDEX 1
#define SF_LEN 1
#define DTC_LEN 3
#define FF_REC_NUM_LEN 1

#define HALF_BYTE 							4
#define OFFSET_ONE_BYTE						8
#define OFFSET_TWO_BYTES 					16
#define OFFSET_THREE_BYTES					24

/* Read/WriteMemeoryByAddress */
#define ALFID_INDEX 1
#define ALFID_LEN 1
#define ADDR_START_INDEX 2
/* DynamicallyDefineDataByIdentifier */
#define DDDDI_INDEX 2
#define DDDDI_LEN 2
#define DYNDEF_ALFID_INDEX 4
#define DYNDEF_ADDRESS_START_INDEX 5
/* InputOutputControlByIdentifier */
#define IOI_INDEX 1
#define IOI_LEN 2
#define IOCP_INDEX 3
#define IOCP_LEN 1
#define COR_INDEX 4
#define IS_VALID_IOCTRL_PARAM(_x) ((_x) <= DCM_SHORT_TERM_ADJUSTMENT)
#define TO_SIGNAL_BIT(_x) (uint8)(1u<<(7u-((_x)%8u)))

/*OBD RequestCurrentPowertrainDiagnosticData*/
#define PIDZERO								0
#define DATAZERO							0
#define INFOTYPE_ZERO						0
#define PID_LEN								1
#define RECORD_NUM_ZERO						0
#define SUPPRTED_PIDS_DATA_LEN				4
#define LEAST_BIT_MASK  					((uint8)0x01u)
#define OBD_DATA_LSB_MASK 					((uint32)0x000000FFu)
#define OBD_REQ_MESSAGE_LEN_ONE_MIN 		2
#define OBD_REQ_MESSAGE_LEN_MAX  			7
#define AVAIL_TO_SUPPORTED_PID_OFFSET_MIN  	0x01
#define AVAIL_TO_SUPPORTED_PID_OFFSET_MAX  	0x20
#define AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MIN  	0x01
#define AVAIL_TO_SUPPORTED_INFOTYPE_OFFSET_MAX  	0x20
#define MAX_REQUEST_PID_NUM 				6
#define LENGTH_OF_DTC  						2

/* CommunicationControl */
#define CC_CTP_INDEX 2
#define IS_SUPPORTED_0x28_SUBFUNCTION(_x) ((_x) < 4u)
#define UDS_0x28_NOF_COM_TYPES 3u
#define UDS_0x28_NOF_SUB_FUNCTIONS 4u
#define IS_VALID_0x28_COM_TYPE(_x) (((_x) > 0u) && ((_x) < 4u))
/*OBD RequestCurrentPowertrainDiagnosticData*/
#define FF_NUM_LEN							1
#define OBD_DTC_LEN							2
#define OBD_SERVICE_TWO 					((uint8)0x02u)
#define MAX_PID_FFNUM_NUM					3
#define OBD_REQ_MESSAGE_LEN_TWO_MIN			3
#define DATA_ELEMENT_INDEX_OF_PID_NOT_SUPPORTED  0
#define OBD_SERVICE_2_PID_AND_FRAME_SIZE 2u
#define OBD_SERVICE_2_PID_INDEX 0u
#define OBD_SERVICE_2_FRAME_INDEX 1u

/*OBD RequestEmissionRelatedDiagnosticTroubleCodes service03 07*/
#define EMISSION_DTCS_HIGH_BYTE(dtc)		(((uint32)(dtc) >> 16) & 0xFFu)
#define EMISSION_DTCS_LOW_BYTE(dtc)			(((uint32)(dtc) >> 8) & 0xFFu)
#define OBD_RESPONSE_DTC_MAX_NUMS			126

/*OBD OnBoardMonitoringTestResultsSpecificMonitoredSystems service06*/
#define OBDMID_LEN							    1u
#define OBDMID_DATA_START_INDEX  			    1u
#define OBDM_TID_LEN						    1u
#define OBDM_UASID_LEN						    1u
#define OBDM_TESTRESULT_LEN					    6u
#define SUPPORTED_MAX_OBDMID_REQUEST_LEN   	    1u
#define SUPPORTED_OBDM_OUTPUT_LEN               (OBDM_TID_LEN + OBDM_UASID_LEN + OBDM_TESTRESULT_LEN)
#define SUPPORTED_OBDMIDS_DATA_LEN				4u
#define AVAIL_TO_SUPPORTED_OBDMID_OFFSET_MIN  	0x01
#define AVAIL_TO_SUPPORTED_OBDMID_OFFSET_MAX  	0x20
#define MAX_REQUEST_OBDMID_NUM 				    6u
#define IS_AVAILABILITY_OBDMID(_x)              ((0 == ((_x) % 0x20)) && ((_x) <= 0xE0))
#define OBDM_LSB_MASK				            0xFFu

/*OBD Requestvehicleinformation service09*/
#define OBD_TX_MAXLEN						0xFF
#define MAX_REQUEST_VEHINFO_NUM				6
#define OBD_SERVICE_FOUR 					0x04
#define OBD_VIN_LENGTH						17

#define IS_AVAILABILITY_PID(_x) ( (0 == ((_x) % 0x20)) && ((_x) <= 0xE0))
#define IS_AVAILABILITY_INFO_TYPE(_x) IS_AVAILABILITY_PID(_x)

#define BYTES_TO_DTC(hb, mb, lb)	(((uint32)(hb) << 16) | ((uint32)(mb) << 8) | (uint32)(lb))
#define DTC_HIGH_BYTE(dtc)			(((uint32)(dtc) >> 16) & 0xFFu)
#define DTC_MID_BYTE(dtc)			(((uint32)(dtc) >> 8) & 0xFFu)
#define DTC_LOW_BYTE(dtc)			((uint32)(dtc) & 0xFFu)

/* UDS ReadDataByPeriodicIdentifier */
#define TO_PERIODIC_DID(_x) (DCM_PERODICDID_HIGH_MASK + (uint16)(_x))
/* Maximum length for periodic Dids */
#define MAX_PDID_DATA_SIZE 7
/* CAN */
#define MAX_TYPE2_PERIODIC_DID_LEN_CAN 7
#define MAX_TYPE1_PERIODIC_DID_LEN_CAN 5

/* Flexray */
/* IMPROVEMENT: Maximum length for flexray? */
#define MAX_TYPE2_PERIODIC_DID_LEN_FLEXRAY 0
#define MAX_TYPE1_PERIODIC_DID_LEN_FLEXRAY 0

/* Ip */
/* IMPROVEMENT: Maximum length for ip? */
#define MAX_TYPE2_PERIODIC_DID_LEN_IP 0
#define MAX_TYPE1_PERIODIC_DID_LEN_IP 0

#define TIMER_DECREMENT(timer) \
        if (timer >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) { \
            timer = timer - DCM_MAIN_FUNCTION_PERIOD_TIME_MS; \
        } \

/* UDS Linkcontrol */
#define LINKCONTROL_SUBFUNC_VERIFY_BAUDRATE_TRANS_WITH_FIXED_BAUDRATE    0x01
#define LINKCONTROL_SUBFUNC_VERIFY_BAUDRATE_TRANS_WITH_SPECIFIC_BAUDRATE 0x02
#define LINKCONTROL_SUBFUNC_TRANSITION_BAUDRATE 0x03


typedef enum {
    DCM_READ_MEMORY = 0,
    DCM_WRITE_MEMORY,
} DspMemoryServiceType;

typedef enum {
    DCM_DSP_RESET_NO_RESET,
    DCM_DSP_RESET_PENDING,
    DCM_DSP_RESET_WAIT_TX_CONF,
} DcmDspResetStateType;

typedef struct {
    DcmDspResetStateType resetPending;
    PduIdType resetPduId;
    PduInfoType *pduTxData;
    Dcm_EcuResetType resetType;
} DspUdsEcuResetDataType;

typedef enum {
    DCM_JTB_IDLE,
    DCM_JTB_WAIT_RESPONSE_PENDING_TX_CONFIRM,
    DCM_JTB_EXECUTE,
    DCM_JTB_RESAPP_FINAL_RESPONSE_TX_CONFIRM,
    DCM_JTB_RESAPP_WAIT_RESPONSE_PENDING_TX_CONFIRM,
    DCM_JTB_RESAPP_ASSEMBLE_FINAL_RESPONSE
}DspJumpToBootState;

typedef struct {
    boolean pendingSessionChange;
    PduIdType sessionPduId;
    Dcm_SesCtrlType session;
    DspJumpToBootState jumpToBootState;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
    uint16 P2;
    uint16 P2Star;
    Dcm_ProtocolType protocolId;
} DspUdsSessionControlDataType;

typedef struct {
    PduIdType confirmPduId;
    DspJumpToBootState jumpToBootState;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
} DspUdsLinkControlDataType;

typedef struct {
    ReadDidPendingStateType state;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
    uint16 txWritePos;
    uint16 nofReadDids;
    uint16 reqDidIndex;
    uint16 pendingDid;
    uint16 pendingDataLength;
    uint16 pendingSignalIndex;
    uint16 pendingDataStartPos;
} DspUdsReadDidPendingType;

typedef enum {
    DCM_GENERAL_IDLE,
    DCM_GENERAL_PENDING,
    DCM_GENERAL_FORCE_RCRRP_AWAITING_SEND,
    DCM_GENERAL_FORCE_RCRRP,
} GeneralPendingStateType;

typedef struct {
    GeneralPendingStateType state;
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
    uint8 pendingService;
} DspUdsGeneralPendingType;

typedef struct {
    boolean comControlPending;
    uint8 subFunction;
    uint8 subnet;
    uint8 comType;
    PduIdType confirmPdu;
    PduIdType rxPdu;
} DspUdsCommunicationControlDataType;

static DspUdsEcuResetDataType dspUdsEcuResetData;
static DspUdsSessionControlDataType dspUdsSessionControlData;
#if defined(DCM_USE_SERVICE_LINKCONTROL)
static DspUdsLinkControlDataType dspUdsLinkControlData;
#endif
static DspUdsReadDidPendingType dspUdsReadDidPending;
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
static DspUdsGeneralPendingType dspUdsWriteDidPending;
#endif
static DspUdsGeneralPendingType dspUdsRoutineControlPending;
static DspUdsGeneralPendingType dspUdsSecurityAccessPending;
#if defined(DCM_USE_UPLOAD_DOWNLOAD)
static DspUdsGeneralPendingType dspUdsUploadDownloadPending;
#endif
#ifdef DCM_USE_SERVICE_COMMUNICATIONCONTROL
static DspUdsCommunicationControlDataType communicationControlData;
#endif


typedef enum {
    DELAY_TIMER_DEACTIVE,
    DELAY_TIMER_ON_BOOT_ACTIVE,
    DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE
}DelayTimerActivation;

typedef struct {
    uint8                           secAcssAttempts; //Counter for number of false attempts
    uint32                          timerSecAcssAttempt; //Timer after exceededNumberOfAttempts
    DelayTimerActivation            startDelayTimer; //Flag to indicate Delay timer is active
}DspUdsSecurityAccessChkParam;

typedef struct {
    boolean 						reqInProgress;
    Dcm_SecLevelType				reqSecLevel;
#if (DCM_SECURITY_EOL_INDEX != 0)
    DspUdsSecurityAccessChkParam    secFalseAttemptChk[DCM_SECURITY_EOL_INDEX];
    uint8                           currSecLevIdx; //Current index for secFalseAttemptChk
#endif
    const Dcm_DspSecurityRowType	*reqSecLevelRef;
} DspUdsSecurityAccessDataType;

static DspUdsSecurityAccessDataType dspUdsSecurityAccesData;

typedef enum{
    DCM_MEMORY_UNUSED,
    DCM_MEMORY_READ,
    DCM_MEMORY_WRITE,
    DCM_MEMORY_FAILED	
}Dcm_DspMemoryStateType;

Dcm_DspMemoryStateType dspMemoryState;

typedef enum{
    DCM_DDD_SOURCE_DEFAULT,
    DCM_DDD_SOURCE_DID,
    DCM_DDD_SOURCE_ADDRESS
}Dcm_DspDDDSourceKindType;

typedef enum {
    PDID_ADDED = 0,
    PDID_UPDATED,
    PDID_BUFFER_FULL
}PdidEntryStatusType;

#if (DCM_PERIODIC_DID_SYNCH_SAMPLING == STD_ON)
typedef enum {
    PDID_NOT_SAMPLED = 0,
    PDID_SAMPLED_OK,
    PDID_SAMPLED_FAILED
}PdidSampleStatus;
#endif

typedef struct{
    uint32 PDidTxCounter;
    uint32 PDidTxPeriod;
    PduIdType PDidRxPduID;
    uint8 PeriodicDid;
#if (DCM_PERIODIC_DID_SYNCH_SAMPLING == STD_ON)
    uint8 PdidData[MAX_PDID_DATA_SIZE];
    uint8 PdidLength;
    PdidSampleStatus PdidSampleStatus;
#endif
}Dcm_pDidType;/* a type to save  the periodic DID and cycle */

typedef struct{
    Dcm_pDidType dspPDid[DCM_LIMITNUMBER_PERIODDATA];	/*a buffer to save the periodic DID and cycle   */
    uint8 PDidNofUsed;										/* note the number of periodic DID is used */
    uint8 nextStartIndex;
}Dsp_pDidRefType;

#if defined(DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER)
static Dsp_pDidRefType dspPDidRef;
#endif

typedef struct{
    uint8   formatOrPosition;						/*note the formate of address and size*/
    uint8	memoryIdentifier;
    uint32 SourceAddressOrDid;								/*note the memory address */
    uint16 Size;										/*note the memory size */
    Dcm_DspDDDSourceKindType DDDTpyeID;
}Dcm_DspDDDSourceType;

typedef struct{
    uint16 DynamicallyDid;
    Dcm_DspDDDSourceType DDDSource[DCM_MAX_DDDSOURCE_NUMBER];
}Dcm_DspDDDType;

#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static Dcm_DspDDDType dspDDD[DCM_MAX_DDD_NUMBER];
#endif

#if defined(DCM_USE_SERVICE_INPUTOUTPUTCONTROLBYIDENTIFIER) && defined(DCM_USE_CONTROL_DIDS)
typedef uint8 Dcm_DspIOControlVector[(DCM_MAX_IOCONTROL_DID_SIGNALS + 7) / 8];
typedef struct {
    uint16 did;
    boolean controlActive;
    Dcm_DspIOControlVector activeSignalBitfield;
}Dcm_DspIOControlStateType;
static Dcm_DspIOControlStateType IOControlStateList[DCM_NOF_IOCONTROL_DIDS];

typedef struct {
    const PduInfoType* pduRxData;
    PduInfoType* pduTxData;
    uint16 pendingSignalIndex;
    boolean pendingControl;
    GeneralPendingStateType state;
    Dcm_DspIOControlVector signalAffected;
    boolean controlActivated;
}DspUdsIOControlPendingType;
static DspUdsIOControlPendingType IOControlData;
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_CONTROLDTCSETTING)
typedef struct {
    Dem_DTCGroupType dtcGroup;
    Dem_DTCKindType dtcKind;
    boolean settingDisabled;
}Dcm_DspControlDTCSettingsType;
static Dcm_DspControlDTCSettingsType DspDTCSetting;
#endif

#if defined(DCM_USE_UPLOAD_DOWNLOAD)
typedef enum {
    DCM_NO_DATA_TRANSFER,
    DCM_UPLOAD,
    DCM_DOWNLOAD
}DcmDataTransferType;

typedef struct {
    uint32 nextAddress;
    uint8 blockSequenceCounter;
    DcmDataTransferType transferType;
    boolean firstBlockReceived;
    uint32 uplBytesLeft; /* Bytes left to be uploaded */
    uint32 uplMemBlockSize; /* block length is maxNumberOfBlockLength (ISO14229) minus SID and lengthFormatIdentifier */
}DcmTransferStatusType;

static DcmTransferStatusType TransferStatus;
#endif

static Dcm_ProgConditionsType GlobalProgConditions;

static GeneralPendingStateType ProgConditionStartupResponseState;

static boolean ProtocolStartRequested = FALSE;

/*
 * * static Function
 */
#ifndef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
#define LookupDDD(_x,  _y ) FALSE
#define readDDDData(_x, _y, _z, _v) DCM_E_GENERALREJECT
#else
static boolean LookupDDD(uint16 didNr, const Dcm_DspDDDType **DDid);
#endif

static void DspCancelPendingDid(uint16 didNr, uint16 signalIndex, ReadDidPendingStateType pendingState, PduInfoType *pduTxData );
//static void DspCancelPendingRoutine(const PduInfoType *pduRxData, PduInfoType *pduTxData);
//static void DspCancelPendingSecurityAccess(const PduInfoType *pduRxData, PduInfoType *pduTxData);
static Dcm_NegativeResponseCodeType DspUdsSecurityAccessCompareKeySubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel, boolean isCancel);
static Dcm_NegativeResponseCodeType DspUdsSecurityAccessGetSeedSubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel, boolean isCancel);

/**
 * Init function for Dsp
 * @param firstCall
 */
void DspInit(boolean firstCall)
{
    dspUdsSecurityAccesData.reqInProgress = FALSE;
    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
    dspUdsSessionControlData.pendingSessionChange = FALSE;
#if defined(DCM_USE_SERVICE_LINKCONTROL)
    dspUdsLinkControlData.jumpToBootState = DCM_JTB_IDLE;
#endif

#if defined(USE_DEM) && defined(DCM_USE_SERVICE_CONTROLDTCSETTING)
    if( firstCall ) {
        DspDTCSetting.settingDisabled = FALSE;
    }
#endif
    dspMemoryState = DCM_MEMORY_UNUSED;
    /* clear periodic send buffer */
#if defined(DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER)
    memset(&dspPDidRef,0,sizeof(dspPDidRef));
#endif
#ifdef DCM_USE_CONTROL_DIDS
    if(firstCall) {
        memset(IOControlStateList, 0, sizeof(IOControlStateList));
        IOControlData.state = DCM_GENERAL_IDLE;
    }
#endif
#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
    /* clear dynamically Did buffer */
    memset(&dspDDD[0],0,sizeof(dspDDD));
#endif

#if (DCM_SECURITY_EOL_INDEX != 0)
    uint8 temp = 0;
    if (firstCall) {
        //Reset the security access attempts
        do {
            dspUdsSecurityAccesData.secFalseAttemptChk[temp].secAcssAttempts = 0;
            if (Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[temp].DspSecurityDelayTimeOnBoot >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].timerSecAcssAttempt = Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[temp].DspSecurityDelayTimeOnBoot;
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].startDelayTimer = DELAY_TIMER_ON_BOOT_ACTIVE;
            }
            else {
                dspUdsSecurityAccesData.secFalseAttemptChk[temp].startDelayTimer = DELAY_TIMER_DEACTIVE;
            }
            temp++;
        } while (temp < DCM_SECURITY_EOL_INDEX);
        dspUdsSecurityAccesData.currSecLevIdx = 0;
    }
#else
    (void)firstCall;
#endif
#if defined(DCM_USE_UPLOAD_DOWNLOAD)
    TransferStatus.blockSequenceCounter = 0;
    TransferStatus.firstBlockReceived = FALSE;
    TransferStatus.transferType = DCM_NO_DATA_TRANSFER;
    dspUdsUploadDownloadPending.state = DCM_GENERAL_IDLE;
#endif
    dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
#ifdef DCM_USE_SERVICE_COMMUNICATIONCONTROL
    communicationControlData.comControlPending = FALSE;
    if (firstCall) {
        memset(DspChannelComMode, DCM_ENABLE_RX_TX_NORM_NM, sizeof(DspChannelComMode));
    }
#endif
}

/**
 * Function called first main function after init to allow Dsp to request
 * protocol start request
 */
void DspCheckProtocolStartRequests(void)
{
    /* @req DCM536 */
    if(DCM_WARM_START == Dcm_GetProgConditions(&GlobalProgConditions)) {
        /* Jump from bootloader */
#if 0
        /* !req DCM768 */
#if defined(USE_BSWM)
        if( progConditions.ApplUpdated ) {
            BswM_Dcm_ApplicationUpdated();
        }
#endif
#endif
        GlobalProgConditions.ApplUpdated = FALSE;
        if( (SID_DIAGNOSTIC_SESSION_CONTROL == GlobalProgConditions.Sid) || (SID_ECU_RESET == GlobalProgConditions.Sid)) {
            uint8 session = (SID_DIAGNOSTIC_SESSION_CONTROL == GlobalProgConditions.Sid) ? GlobalProgConditions.SubFncId : DCM_DEFAULT_SESSION;
            if( E_OK == DcmRequestStartProtocol(DCM_REQ_DSP, session,
                    GlobalProgConditions.ProtocolId, GlobalProgConditions.TesterSourceAdd, GlobalProgConditions.ResponseRequired) ) {
                ProtocolStartRequested = TRUE;
            } else {
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
            }
        }
    }
}

/**
 * Protocol start notification
 * @param started
 */
void DcmDspProtocolStartNotification(boolean started)
{
    if( ProtocolStartRequested ) {
        if(started) {
            if( GlobalProgConditions.ResponseRequired ) {
                /* Wait until full communication has been indicated, then send a response to service */
                ProgConditionStartupResponseState = DCM_GENERAL_PENDING;
            }
        } else {
            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
        }
    } else {
        /* Have not requested a start.. */
        DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
    }
}

/**
 * Get the rte mode corresponding to a reset type
 * @param resetType
 * @param rteMode
 * @return E_OK: rte mode found, E_NOT_OK: rte mode not found
 */
static Std_ReturnType getResetRteMode(Dcm_EcuResetType resetType, uint8 *rteMode)
{
    Std_ReturnType ret = E_OK;
    switch(resetType) {
        case DCM_HARD_RESET:
            *rteMode = RTE_MODE_DcmEcuReset_HARD;
            break;
        case DCM_KEY_OFF_ON_RESET:
            *rteMode = RTE_MODE_DcmEcuReset_KEYONOFF;
            break;
        case DCM_SOFT_RESET:
            *rteMode = RTE_MODE_DcmEcuReset_SOFT;
            break;
        default:
            ret = E_NOT_OK;
            break;
    }
    return ret;
}

/**
 * Main function for executing ECU reset
 */
void DspResetMainFunction(void)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 rteMode;
    if( (DCM_DSP_RESET_PENDING == dspUdsEcuResetData.resetPending) && (E_OK == getResetRteMode(dspUdsEcuResetData.resetType, &rteMode)) ) {
        /* IMPROVEMENT: Should be a call to SchM */
        result = Rte_Switch_DcmEcuReset_DcmEcuReset(rteMode);

        switch( result ) {
            case E_OK:
                dspUdsEcuResetData.resetPending = DCM_DSP_RESET_WAIT_TX_CONF;
                // Create positive response
                dspUdsEcuResetData.pduTxData->SduDataPtr[1] = dspUdsEcuResetData.resetType;
                dspUdsEcuResetData.pduTxData->SduLength = 2;
                DsdDspProcessingDone(DCM_E_POSITIVERESPONSE);
                break;
            case E_PENDING:
                dspUdsEcuResetData.resetPending = DCM_DSP_RESET_PENDING;
                break;
            case E_NOT_OK:
            default:
                dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
                DsdDspProcessingDone(DCM_E_CONDITIONSNOTCORRECT);
                break;
        }
    }
}

/**
 * Main function for reading DIDs
 */
void DspReadDidMainFunction(void) {
    if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
        DspUdsReadDataByIdentifier(dspUdsReadDidPending.pduRxData, dspUdsReadDidPending.pduTxData);
    }
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
    if( DCM_GENERAL_PENDING == dspUdsWriteDidPending.state ) {
        DspUdsWriteDataByIdentifier(dspUdsWriteDidPending.pduRxData, dspUdsWriteDidPending.pduTxData);
    }
#endif
}

/**
 * Main function for routine control
 */
void DspRoutineControlMainFunction(void) {
    if( DCM_GENERAL_FORCE_RCRRP_AWAITING_SEND == dspUdsRoutineControlPending.state){
        /* !req DCM528 */
        /* !req DCM529 Should wait until transmit has been confirmed */
        dspUdsRoutineControlPending.state = DCM_GENERAL_FORCE_RCRRP; // Do not try again until next main loop
    }
    else if( (DCM_GENERAL_PENDING == dspUdsRoutineControlPending.state) || (DCM_GENERAL_FORCE_RCRRP == dspUdsRoutineControlPending.state)) {
        //DspUdsRoutineControl(dspUdsRoutineControlPending.pduRxData, dspUdsRoutineControlPending.pduTxData);
    }
}

/**
 * Main function for security access
 */
void DspSecurityAccessMainFunction(void) {
    if( DCM_GENERAL_PENDING == dspUdsSecurityAccessPending.state ) {
         DspUdsSecurityAccess(dspUdsSecurityAccessPending.pduRxData, dspUdsSecurityAccessPending.pduTxData);
    }
}

/**
 * Main function for sending response to service as a result to jump from boot
 */
void DspStartupServiceResponseMainFunction(void)
{
    if(DCM_GENERAL_PENDING == ProgConditionStartupResponseState) {
        Std_ReturnType reqRet = DslDspResponseOnStartupRequest(GlobalProgConditions.Sid, GlobalProgConditions.SubFncId, GlobalProgConditions.ProtocolId, (uint16)GlobalProgConditions.TesterSourceAdd);
        if(E_PENDING != reqRet) {
            if(E_OK != reqRet) {
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_RESPONSE);
            }
            ProgConditionStartupResponseState = DCM_GENERAL_IDLE;
        }
    }
}

void DspPreDsdMain(void) {
    /* Should be called before DsdMain so that an internal request
     * may be processed directly */
#if defined(DCM_USE_SERVICE_READDATABYPERIODICIDENTIFIER)
    DspPeriodicDIDMainFunction();
#endif
#if  defined(DCM_USE_SERVICE_RESPONSEONEVENT) && (DCM_ROE_INTERNAL_DIDS == STD_ON)
    DCM_ROE_PollDataIdentifiers();
#endif
    /* Should be done before DsdMain so that we can fulfill
     * DCM719 (mode switch DcmEcuReset to EXECUTE next main function) */
#if (DCM_USE_JUMP_TO_BOOT == STD_ON) || defined(DCM_USE_SERVICE_LINKCONTROL)
    DspJumpToBootMainFunction();
#endif

    DspStartupServiceResponseMainFunction();

}
void DspMain(void)
{
#if (DCM_USE_SPLIT_TASK_CONCEPT == STD_ON)
    DsdSetDspProcessingActive(TRUE);
#endif
    DspResetMainFunction();
#if defined(DCM_USE_SERVICE_READMEMORYBYADDRESS) || defined(DCM_USE_SERVICE_WRITEMEMORYBYADDRESS)
    DspMemoryMainFunction();
#endif
    DspReadDidMainFunction();
    DspRoutineControlMainFunction();
#ifdef DCM_USE_UPLOAD_DOWNLOAD
    DspUploadDownloadMainFunction();
#endif
    DspSecurityAccessMainFunction();
#if defined(DCM_USE_SERVICE_INPUTOUTPUTCONTROLBYIDENTIFIER) && defined(DCM_USE_CONTROL_DIDS)
    DspIOControlMainFunction();
#endif
#if (DCM_USE_SPLIT_TASK_CONCEPT == STD_ON)
    DsdSetDspProcessingActive(FALSE);
#endif
}

void DspTimerMain(void)
{
#if (DCM_SECURITY_EOL_INDEX != 0)

    for (uint8 i = 0; i < DCM_SECURITY_EOL_INDEX; i++)
    {
        //Check if a wait is required before accepting a request
        switch (dspUdsSecurityAccesData.secFalseAttemptChk[i].startDelayTimer) {

            case DELAY_TIMER_ON_BOOT_ACTIVE:
            case DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE:
                TIMER_DECREMENT(dspUdsSecurityAccesData.secFalseAttemptChk[i].timerSecAcssAttempt);
                if (dspUdsSecurityAccesData.secFalseAttemptChk[i].timerSecAcssAttempt < DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
                    dspUdsSecurityAccesData.secFalseAttemptChk[i].startDelayTimer = DELAY_TIMER_DEACTIVE;
                }
                break;

            case DELAY_TIMER_DEACTIVE:
            default:
                break;
        }
    }
#endif
}

void DspCancelPendingRequests(void)
{
    if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
        /* DidRead was in pending state, cancel it */
        DspCancelPendingDid(dspUdsReadDidPending.pendingDid, dspUdsReadDidPending.pendingSignalIndex ,dspUdsReadDidPending.state, dspUdsReadDidPending.pduTxData);
    }
    dspMemoryState = DCM_MEMORY_UNUSED;
    dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
    dspUdsReadDidPending.state = DCM_READ_DID_IDLE;
#ifdef DCM_USE_SERVICE_WRITEDATABYIDENTIFIER
    dspUdsWriteDidPending.state = DCM_GENERAL_IDLE;
#endif
    if( DCM_GENERAL_IDLE != dspUdsRoutineControlPending.state ) {
        //DspCancelPendingRoutine(dspUdsRoutineControlPending.pduRxData, dspUdsRoutineControlPending.pduTxData);
    }
    dspUdsRoutineControlPending.state = DCM_GENERAL_IDLE;
    if( DCM_GENERAL_IDLE != dspUdsSecurityAccessPending.state ) {
        //DspCancelPendingSecurityAccess(dspUdsSecurityAccessPending.pduRxData, dspUdsSecurityAccessPending.pduTxData);
    }
    dspUdsSecurityAccessPending.state = DCM_GENERAL_IDLE;
#if defined(DCM_USE_UPLOAD_DOWNLOAD)
    if( DCM_GENERAL_IDLE != dspUdsUploadDownloadPending.state ) {
        DspCancelPendingUploadDownload(dspUdsUploadDownloadPending.pendingService);
    }
    dspUdsUploadDownloadPending.state = DCM_GENERAL_IDLE;
#endif

#if defined(DCM_USE_SERVICE_INPUTOUTPUTCONTROLBYIDENTIFIER) && defined(DCM_USE_CONTROL_DIDS)
    if( DCM_GENERAL_PENDING == IOControlData.state ) {
        DspCancelPendingIOControlByDataIdentifier(IOControlData.pduRxData, IOControlData.pduTxData);
    }
    IOControlData.state = DCM_GENERAL_IDLE;
#endif
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
    dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
#endif
#if defined(DCM_USE_SERVICE_LINKCONTROL)
    dspUdsLinkControlData.jumpToBootState  = DCM_JTB_IDLE;
#endif
}

boolean DspCheckSessionLevel(Dcm_DspSessionRowType const* const* sessionLevelRefTable)
{
    Std_ReturnType returnStatus;
    boolean levelFound = FALSE;
    Dcm_SesCtrlType currentSession;

    returnStatus = DslGetSesCtrlType(&currentSession);
    if (returnStatus == E_OK) {
        if( (*sessionLevelRefTable)->Arc_EOL ) {
            /* No session reference configured, no check should be done. */
            levelFound = TRUE;
        } else {
            while ( ((*sessionLevelRefTable)->DspSessionLevel != DCM_ALL_SESSION_LEVEL) && ((*sessionLevelRefTable)->DspSessionLevel != currentSession) && (!(*sessionLevelRefTable)->Arc_EOL) ) {
                sessionLevelRefTable++;
            }

            if (!(*sessionLevelRefTable)->Arc_EOL) {
                levelFound = TRUE;
            }
        }
    }

    return levelFound;
}


boolean DspCheckSecurityLevel(Dcm_DspSecurityRowType const* const* securityLevelRefTable)
{
    Std_ReturnType returnStatus;
    boolean levelFound = FALSE;
    Dcm_SecLevelType currentSecurityLevel;

    returnStatus = DslGetSecurityLevel(&currentSecurityLevel);
    if (returnStatus == E_OK) {
        if( (*securityLevelRefTable)->Arc_EOL ) {
            /* No security level reference configured, no check should be done. */
            levelFound = TRUE;
        } else {
            while ( ((*securityLevelRefTable)->DspSecurityLevel != currentSecurityLevel) && (!(*securityLevelRefTable)->Arc_EOL) ) {
                securityLevelRefTable++;
            }
            if (!(*securityLevelRefTable)->Arc_EOL) {
                levelFound = TRUE;
            }
        }
    }

    return levelFound;
}

/**
 * Checks if a session is supported
 * @param session
 * @return TRUE: Session supported, FALSE: Session not supported
 */
boolean DspDslCheckSessionSupported(uint8 session) {
    const Dcm_DspSessionRowType *sessionRow = Dcm_ConfigPtr->Dsp->DspSession->DspSessionRow;
    while ((sessionRow->DspSessionLevel != session) && (!sessionRow->Arc_EOL) ) {
        sessionRow++;
    }
    return (FALSE == sessionRow->Arc_EOL)? TRUE: FALSE;
}

/**
**		This Function for check the pointer of Dynamically Did Sourced by Did buffer using a didNr
**/
#ifdef DCM_USE_SERVICE_DYNAMICALLYDEFINEDATAIDENTIFIER
static boolean LookupDDD(uint16 didNr,  const Dcm_DspDDDType **DDid )	
{
    uint8 i;
    boolean ret = FALSE;
    const Dcm_DspDDDType* DDidptr = &dspDDD[0];
    

    if (didNr < DCM_PERODICDID_HIGH_MASK) {
        return ret;
    }


    for(i = 0;((i < DCM_MAX_DDD_NUMBER) && (ret == FALSE)); i++)
    {
        if(DDidptr->DynamicallyDid == didNr)
        {
            ret = TRUE;
        
        }
        else
        {
            DDidptr++;
        }
    }
    if(ret == TRUE)
    {
        *DDid = DDidptr;
    }

    return ret;
}
#endif


static void DspCancelPendingDid(uint16 didNr, uint16 signalIndex, ReadDidPendingStateType pendingState, PduInfoType *pduTxData )
{
    const Dcm_DspDidType *didPtr = NULL_PTR;
    if( lookupNonDynamicDid(didNr, &didPtr) ) {
        if( signalIndex < didPtr->DspNofSignals ) {
            const Dcm_DspDataType *dataPtr = didPtr->DspSignalRef[signalIndex].DspSignalDataRef;
            if( DCM_READ_DID_PENDING_COND_CHECK == pendingState ) {
                if( dataPtr->DspDataConditionCheckReadFnc != NULL_PTR ) {
                    (void)dataPtr->DspDataConditionCheckReadFnc (DCM_CANCEL, pduTxData->SduDataPtr);
                }
            } else if( DCM_READ_DID_PENDING_READ_DATA == pendingState ) {
                if( dataPtr->DspDataReadDataFnc.AsynchDataReadFnc != NULL_PTR ) {
                    if( DATA_PORT_ASYNCH == dataPtr->DspDataUsePort ) {
                        (void)dataPtr->DspDataReadDataFnc.AsynchDataReadFnc(DCM_CANCEL, pduTxData->SduDataPtr);
                    }
                }
            } else {
                /* Not in a pending state */
                DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
            }
        } else {
            /* Invalid signal index */
            DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_UNEXPECTED_EXECUTION);
        }
    }
}


void DspUdsReadDataByIdentifier(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
    /* @req DCM253 */
    /* !req DCM561 */
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    uint16 nrOfDids;
    uint16 didNr;
    const Dcm_DspDidType *didPtr = NULL_PTR;
    Dcm_DspDDDType *DDidPtr = NULL_PTR;
    uint16 txPos = 1u;
    uint16 i;
    uint16 Length = 0u;
    boolean noRequestedDidSupported = TRUE;
    ReadDidPendingStateType pendingState = DCM_READ_DID_IDLE;
    uint16 nofDidsRead = 0u;
    uint16 reqDidStartIndex = 0u;
    uint16 nofDidsReadInPendingReq = 0u;
    uint16 pendingDid = 0u;
    uint16 pendingDataLen = 0u;
    uint16 pendingSigIndex = 0u;
    uint16 pendingDataStartPos = 0u;

    if ( (((pduRxData->SduLength - 1) % 2) == 0) && ( 0 != (pduRxData->SduLength - 1))) {
        nrOfDids = (pduRxData->SduLength - 1) / 2;
        if( DCM_READ_DID_IDLE != dspUdsReadDidPending.state ) {
            pendingState = dspUdsReadDidPending.state;
            txPos = dspUdsReadDidPending.txWritePos;
            nofDidsReadInPendingReq = dspUdsReadDidPending.nofReadDids;
            reqDidStartIndex = dspUdsReadDidPending.reqDidIndex;
            pendingDataLen = dspUdsReadDidPending.pendingDataLength;
            pendingSigIndex = dspUdsReadDidPending.pendingSignalIndex;
            pendingDataStartPos = dspUdsReadDidPending.pendingDataStartPos;
        }
        /* IMPROVEMENT: Check security level and session for all dids before trying to read data */
        for (i = reqDidStartIndex; (i < nrOfDids) && (responseCode == DCM_E_POSITIVERESPONSE); i++) {
            didNr = (uint16)((uint16)pduRxData->SduDataPtr[1 + (i * 2)] << 8) + pduRxData->SduDataPtr[2 + (i * 2)];
            if (lookupNonDynamicDid(didNr, &didPtr)) {	/* @req DCM438 */
                noRequestedDidSupported = FALSE;
                /* IMPROVEMENT: Check if the did has data or did ref. If not NRC here? */
                /* !req DCM481 */
                /* !req DCM482 */
                /* !req DCM483 */
                //responseCode = readDidData(didPtr, pduTxData, &txPos, &pendingState, &pendingDid, &pendingSigIndex, &pendingDataLen, &nofDidsRead, nofDidsReadInPendingReq, &pendingDataStartPos);
                if( DCM_E_RESPONSEPENDING == responseCode ) {
                    dspUdsReadDidPending.reqDidIndex = i;
                } else {
                    /* No pending response */
                    nofDidsReadInPendingReq = 0u;
                    nofDidsRead = 0u;
                }
            } else if(LookupDDD(didNr,(const Dcm_DspDDDType **)&DDidPtr) == TRUE) {
                noRequestedDidSupported = FALSE;
                /* @req DCM651 */
                /* @req DCM652 */
                /* @req DCM653 */
                pduTxData->SduDataPtr[txPos++] = (uint8)((DDidPtr->DynamicallyDid>>8) & 0xFF);
                pduTxData->SduDataPtr[txPos++] = (uint8)(DDidPtr->DynamicallyDid & 0xFF);
                responseCode = readDDDData(DDidPtr, &(pduTxData->SduDataPtr[txPos]), pduTxData->SduLength - txPos, &Length);
                txPos += Length;
            } else {
                /* DID not found. */
            }
        }
    } else {
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }
    if( (responseCode != DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT) && noRequestedDidSupported ) {
        /* @req DCM438 */
        /* None of the Dids in the request found. */
        responseCode = DCM_E_REQUESTOUTOFRANGE;
    }
    if (DCM_E_POSITIVERESPONSE == responseCode) {
        pduTxData->SduLength = txPos;
    }

    if( DCM_E_RESPONSEPENDING == responseCode) {
        dspUdsReadDidPending.state = pendingState;
        dspUdsReadDidPending.pduRxData = (PduInfoType*)pduRxData;
        dspUdsReadDidPending.pduTxData = pduTxData;
        dspUdsReadDidPending.nofReadDids = nofDidsRead;
        dspUdsReadDidPending.txWritePos = txPos;
        dspUdsReadDidPending.pendingDid = pendingDid;
        dspUdsReadDidPending.pendingDataLength = pendingDataLen;
        dspUdsReadDidPending.pendingSignalIndex = pendingSigIndex;
        dspUdsReadDidPending.pendingDataStartPos = pendingDataStartPos;
    } else {
        dspUdsReadDidPending.state = DCM_READ_DID_IDLE;
        dspUdsReadDidPending.nofReadDids = 0;
        DsdDspProcessingDone(responseCode);
    }
}

static Dcm_NegativeResponseCodeType DspUdsSecurityAccessGetSeedSubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel, boolean isCancel) {

    Dcm_NegativeResponseCodeType getSeedErrorCode;
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    uint8 cntSecRow = 0;
    Dcm_OpStatusType opStatus = (Dcm_OpStatusType)DCM_INITIAL;
    if(dspUdsSecurityAccessPending.state == DCM_GENERAL_PENDING){
        if( isCancel ) {
            opStatus = (Dcm_OpStatusType)DCM_CANCEL;
        } else {
            opStatus = (Dcm_OpStatusType)DCM_PENDING;
        }
    }

    // requestSeed message
    // Check if type exist in security table
    const Dcm_DspSecurityRowType *securityRow = &Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[0];
    while ((securityRow->DspSecurityLevel != requestedSecurityLevel) && (!securityRow->Arc_EOL)) {
        securityRow++;
        cntSecRow++; // Get the index of the security config
    }
    if (!securityRow->Arc_EOL) {

#if (DCM_SECURITY_EOL_INDEX != 0)

        //Check if a wait is required before accepting a request
        dspUdsSecurityAccesData.currSecLevIdx = cntSecRow;
        if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer != DELAY_TIMER_DEACTIVE) {
            responseCode = DCM_E_REQUIREDTIMEDELAYNOTEXPIRED;
        }
        else
#endif
        {
            // Check length
            if (pduRxData->SduLength == (2 + securityRow->DspSecurityADRSize)) {    /* @req DCM321 RequestSeed */
                Dcm_SecLevelType activeSecLevel;
                Std_ReturnType result;
                result = Dcm_GetSecurityLevel(&activeSecLevel);
                if (result == E_OK) {
                    if( (2 + securityRow->DspSecuritySeedSize) <= pduTxData->SduLength ) {
                        if (requestedSecurityLevel == activeSecLevel) {     /* @req DCM323 */
                            pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                            // If same level set the seed to zeroes
                            memset(&pduTxData->SduDataPtr[2], 0, securityRow->DspSecuritySeedSize);
                            pduTxData->SduLength = 2 + securityRow->DspSecuritySeedSize;
                        } else {
                            // New security level ask for seed
                            if ((securityRow->GetSeed.getSeedWithoutRecord != NULL_PTR) || (securityRow->GetSeed.getSeedWithRecord != NULL_PTR)) {
                                Std_ReturnType getSeedResult;
                                if(securityRow->DspSecurityADRSize > 0) {
                                    /* @req DCM324 RequestSeed */
                                    getSeedResult = securityRow->GetSeed.getSeedWithRecord(&pduRxData->SduDataPtr[2], opStatus, &pduTxData->SduDataPtr[2], &getSeedErrorCode);
                                } else {
                                    /* @req DCM324 RequestSeed */
                                    getSeedResult = securityRow->GetSeed.getSeedWithoutRecord(opStatus, &pduTxData->SduDataPtr[2], &getSeedErrorCode);
                                }
                                if (getSeedResult == E_PENDING){
                                    responseCode = DCM_E_RESPONSEPENDING;
                                }
                                else if (getSeedResult == E_OK) {
                                    // Everything ok add sub function to tx message and send it.
                                    pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                                    pduTxData->SduLength = 2 + securityRow->DspSecuritySeedSize;

                                    dspUdsSecurityAccesData.reqSecLevel = requestedSecurityLevel;
                                    dspUdsSecurityAccesData.reqSecLevelRef = securityRow;
                                    dspUdsSecurityAccesData.reqInProgress = TRUE;
                                }
                                else {
                                    // GetSeed returned not ok
                                    responseCode = getSeedErrorCode; /* @req DCM659 */
                                }
                            } else {
                                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                            }
                        }
                    } else {
                        /* Tx buffer not big enough */
                        responseCode = DCM_E_RESPONSETOOLONG;
                    }
                } else {
                    // NOTE: What to do?
                    responseCode = DCM_E_GENERALREJECT;
                }
            }
            else {
                // Length not ok
                responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
            }
        }
    }
    else {
        // Requested security level not configured
        responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
    }

    return responseCode;
}

// CompareKey message
static Dcm_NegativeResponseCodeType DspUdsSecurityAccessCompareKeySubFnc (const PduInfoType *pduRxData, PduInfoType *pduTxData, Dcm_SecLevelType requestedSecurityLevel, boolean isCancel) {

    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;
    Dcm_OpStatusType opStatus = (Dcm_OpStatusType)DCM_INITIAL;
    if(dspUdsSecurityAccessPending.state == DCM_GENERAL_PENDING){
        if( isCancel ) {
            opStatus = (Dcm_OpStatusType)DCM_CANCEL;
        } else {
            opStatus = (Dcm_OpStatusType)DCM_PENDING;
        }
    }

    if (requestedSecurityLevel == dspUdsSecurityAccesData.reqSecLevel) {
        /* Check whether senkey message is sent according to a valid sequence */
        if (dspUdsSecurityAccesData.reqInProgress) {

#if (DCM_SECURITY_EOL_INDEX != 0)
            //Check if a wait is required before accepting a request
            if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer != DELAY_TIMER_DEACTIVE) {
                responseCode = DCM_E_REQUIREDTIMEDELAYNOTEXPIRED;
            } else

#endif
            {
                if (pduRxData->SduLength == (2 + dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityKeySize)) { /* @req DCM321 SendKey */
                    if ((dspUdsSecurityAccesData.reqSecLevelRef->CompareKey != NULL_PTR)) {
                        Std_ReturnType compareKeyResult;
                        compareKeyResult = dspUdsSecurityAccesData.reqSecLevelRef->CompareKey(&pduRxData->SduDataPtr[2], opStatus); /* @req DCM324 SendKey */
                        if( isCancel ) {
                            /* Ignore compareKeyResult on cancel  */
                            responseCode = DCM_E_POSITIVERESPONSE;
                        } else if (compareKeyResult == E_PENDING) {
                            responseCode = DCM_E_RESPONSEPENDING;
                        } else if (compareKeyResult == E_OK) {
                            /* Client should reiterate the process of getseed msg, if sendkey fails- ISO14229 */
                            dspUdsSecurityAccesData.reqInProgress = FALSE;
                          // Request accepted
                           // Kill timer
                           DslInternal_SetSecurityLevel(dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityLevel); /* @req DCM325 */
                           pduTxData->SduDataPtr[1] = pduRxData->SduDataPtr[1];
                           pduTxData->SduLength = 2;
                        } else {
                           /* Client should reiterate the process of getseed msg, if sendkey fails- ISO14229 */
                           dspUdsSecurityAccesData.reqInProgress = FALSE;
                           responseCode = DCM_E_INVALIDKEY; /* @req DCM660 */
                        }
                    } else {
                       responseCode = DCM_E_CONDITIONSNOTCORRECT;
                    }
                } else {
                    // Length not ok
                    responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
                }

#if (DCM_SECURITY_EOL_INDEX != 0)
                //Count the false access attempts -> Only send invalid keys events according to ISO 14229
                if (responseCode == DCM_E_INVALIDKEY) {
                    dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts += 1;

                    if (dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts >= dspUdsSecurityAccesData.reqSecLevelRef->DspSecurityNumAttDelay) {
                        //Enable delay timer
                        if (Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[dspUdsSecurityAccesData.currSecLevIdx].DspSecurityDelayTime >= DCM_MAIN_FUNCTION_PERIOD_TIME_MS) {
                            dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].startDelayTimer = DELAY_TIMER_ON_EXCEEDING_LIMIT_ACTIVE;
                            dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].timerSecAcssAttempt = Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[dspUdsSecurityAccesData.currSecLevIdx].DspSecurityDelayTime;
                        }
                        dspUdsSecurityAccesData.secFalseAttemptChk[dspUdsSecurityAccesData.currSecLevIdx].secAcssAttempts  = 0;
                        if (Dcm_ConfigPtr->Dsp->DspSecurity->DspSecurityRow[dspUdsSecurityAccesData.currSecLevIdx].DspSecurityDelayTime > 0) {
                            responseCode = DCM_E_EXCEEDNUMBEROFATTEMPTS; // Delay time is optional according to ISO spec and so is the NRC
                        }
                    }
                }
#endif
            }
        } else {
            // sendKey request without a preceding requestSeed
            responseCode = DCM_E_REQUESTSEQUENCEERROR;
        }
    } else {
        // sendKey request without a preceding requestSeed
#if defined(CFG_DCM_SECURITY_ACCESS_NRC_FIX)
        /* Should this really give subFunctionNotSupported? */
        responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
#else
        responseCode = DCM_E_REQUESTSEQUENCEERROR;
#endif
    }

    return responseCode;
}

void DspUdsSecurityAccess(const PduInfoType *pduRxData, PduInfoType *pduTxData)
{
    /* @req DCM252 */
    Dcm_NegativeResponseCodeType responseCode = DCM_E_POSITIVERESPONSE;

    if( pduRxData->SduLength >= 2u) {
        /* Check sub function range (0x01 to 0x42) */
        if ((pduRxData->SduDataPtr[1] >= 0x01) && (pduRxData->SduDataPtr[1] <= 0x42)) {
            boolean isRequestSeed = pduRxData->SduDataPtr[1] & 0x01u;
            Dcm_SecLevelType requestedSecurityLevel = (pduRxData->SduDataPtr[1]+1)/2;
            if (isRequestSeed) {
                responseCode = DspUdsSecurityAccessGetSeedSubFnc(pduRxData, pduTxData, requestedSecurityLevel,FALSE);
            } else {
                responseCode = DspUdsSecurityAccessCompareKeySubFnc(pduRxData, pduTxData, requestedSecurityLevel,FALSE);
            }
        } else {
            responseCode = DCM_E_SUBFUNCTIONNOTSUPPORTED;
        }
    }
    else {
        responseCode = DCM_E_INCORRECTMESSAGELENGTHORINVALIDFORMAT;
    }

    if( DCM_E_RESPONSEPENDING != responseCode ) {
        dspUdsSecurityAccessPending.state = DCM_GENERAL_IDLE;
        DsdDspProcessingDone(responseCode);
    } else {
        dspUdsSecurityAccessPending.state = DCM_GENERAL_PENDING;
        dspUdsSecurityAccessPending.pduRxData = pduRxData;
        dspUdsSecurityAccessPending.pduTxData = pduTxData;
    }
}


void DspDcmConfirmation(PduIdType confirmPduId, NotifResultType result)
{
    DslResetSessionTimeoutTimer(); /* @req DCM141 */
#if (DCM_USE_JUMP_TO_BOOT == STD_ON)
    if (confirmPduId == dspUdsSessionControlData.sessionPduId) {
        if( DCM_JTB_RESAPP_FINAL_RESPONSE_TX_CONFIRM == dspUdsSessionControlData.jumpToBootState ) {
            /* IMPROVEMENT: Add support for pending */
            /* @req 4.2.2/SWS_DCM_01179 */
            /* @req 4.2.2/SWS_DCM_01180 */
            /* @req 4.2.2/SWS_DCM_01181 */

            if( NTFRSLT_OK == result ) {
                if(E_OK == Dcm_SetProgConditions(&GlobalProgConditions)) {
                    (void)Rte_Switch_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_EXECUTE);
                }
                else {
                    /* Final response has already been sent. */
                    DCM_DET_REPORTERROR(DCM_GLOBAL_ID, DCM_E_INTEGRATION_ERROR);
                }
            }
        }
        dspUdsSessionControlData.jumpToBootState = DCM_JTB_IDLE;
    }
#endif

    if (confirmPduId == dspUdsEcuResetData.resetPduId) {
        if ( DCM_DSP_RESET_WAIT_TX_CONF == dspUdsEcuResetData.resetPending) {
            if( NTFRSLT_OK == result ) {
                /* IMPROVEMENT: Should be a call to SchM */
                (void)Rte_Switch_DcmEcuReset_DcmEcuReset(RTE_MODE_DcmEcuReset_EXECUTE);/* @req DCM594 */
            }
            dspUdsEcuResetData.resetPending = DCM_DSP_RESET_NO_RESET;
        }
    }

    if ( TRUE == dspUdsSessionControlData.pendingSessionChange ) {
        if (confirmPduId == dspUdsSessionControlData.sessionPduId) {
            if( NTFRSLT_OK == result ) {
                /* @req DCM311 */
                DslSetSesCtrlType(dspUdsSessionControlData.session);
            }
            dspUdsSessionControlData.pendingSessionChange = FALSE;
        }
    }
#ifdef DCM_USE_SERVICE_COMMUNICATIONCONTROL
    if(communicationControlData.comControlPending) {
        if( confirmPduId == communicationControlData.confirmPdu ) {
            if(NTFRSLT_OK == result) {
                (void)DspInternalCommunicationControl(communicationControlData.subFunction, communicationControlData.subnet, communicationControlData.comType, communicationControlData.rxPdu, TRUE);
            }
            communicationControlData.comControlPending = FALSE;
        }
    }
#endif
}


