/* ccd_exposure.h */

#ifndef CCD_EXPOSURE_H
#define CCD_EXPOSURE_H

/*  the following 3 lines are needed to support C++ compilers */
#ifdef __cplusplus
extern "C" {
#endif

extern int CCD_Exposure_Set_Trigger_Mode(int external);
extern int CCD_Exposure_Trigger_Mode_Is_Software(void);
extern int CCD_Exposure_Trigger_Mode_Is_External(void);
extern int CCD_Exposure_Length_Set(int exposure_length_ms);
extern int CCD_Exposure_Length_Get(int *exposure_length_ms);
extern int CCD_Exposure_Get_Error_Number(void);
extern void CCD_Exposure_Error(void);
extern void CCD_Exposure_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
