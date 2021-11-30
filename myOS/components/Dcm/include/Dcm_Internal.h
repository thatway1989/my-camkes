/*************************************************************************
    > File Name: Dcm_Internal.h
    > SPDX-License-Identifier: BSD-2-Clause
    > Author: XXX
    > Created Time: 2021年11月17日 星期三 11时25分17秒
 ************************************************************************/

#define DCM_DET_REPORTERROR(_api,_err)                              \
        //(void)Det_ReportError(DCM_MODULE_ID, 0, _api, _err);    \
		printf("Dcm:error!");

#define VALIDATE(_exp,_api,_err ) \
        if(!(_exp)) { \
          DCM_DET_REPORTERROR(_api, _err); \
          return E_NOT_OK; \
        }

#define VALIDATE_RV(_exp,_api,_err,_rv ) \
        if(!(_exp)) { \
          DCM_DET_REPORTERROR(_api, _err); \
          return _rv; \
        }

#define VALIDATE_NO_RV(_exp,_api,_err ) \
        if(!(_exp) ){ \
          DCM_DET_REPORTERROR(_api, _err); \
          return; \
        }

