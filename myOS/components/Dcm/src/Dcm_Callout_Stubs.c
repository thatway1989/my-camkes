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

/* Ecum Callout Stubs - generic version */

#include "Dcm.h"
#if defined(USE_MCU)
#include "Mcu.h"
#endif

#ifdef DCM_NOT_SERVICE_COMPONENT
Std_ReturnType Rte_Switch_DcmDiagnosticSessionControl_DcmDiagnosticSessionControl(Dcm_SesCtrlType session)
{
    (void)session;
    return E_OK;
}

Std_ReturnType Rte_Switch_DcmEcuReset_DcmEcuReset(uint8 resetMode)
{

    switch(resetMode) {
        case RTE_MODE_DcmEcuReset_NONE:
        case RTE_MODE_DcmEcuReset_HARD:
        case RTE_MODE_DcmEcuReset_KEYONOFF:
        case RTE_MODE_DcmEcuReset_SOFT:
        case RTE_MODE_DcmEcuReset_JUMPTOBOOTLOADER:
        case RTE_MODE_DcmEcuReset_JUMPTOSYSSUPPLIERBOOTLOADER:
            break;
        case RTE_MODE_DcmEcuReset_EXECUTE:
#if defined(USE_MCU) && ( MCU_PERFORM_RESET_API == STD_ON )
            Mcu_PerformReset();
#endif
            break;
        default:
            break;

    }
    return E_OK;
}

Std_ReturnType Rte_Switch_DcmControlDTCSetting_DcmControlDTCSetting(uint8 mode)
{
    (void)mode;
    return E_OK;
}
#endif

/* @req DCM543 */
Std_ReturnType Dcm_SetProgConditions(Dcm_ProgConditionsType *ProgConditions)
{
    (void)*ProgConditions;
    return E_OK;
}

/* @req DCM544 */
Dcm_EcuStartModeType Dcm_GetProgConditions(Dcm_ProgConditionsType *ProgConditions)
{
    (void)*ProgConditions;
    return DCM_COLD_START;
}

/* @req DCM547 */
void Dcm_Confirmation(Dcm_IdContextType idContext,PduIdType dcmRxPduId,Dcm_ConfirmationStatusType status)
{
    (void)idContext;
    (void)dcmRxPduId;
    (void)status;
}

