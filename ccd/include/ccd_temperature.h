/* ccd_temperature.h */

#ifndef CCD_TEMPERATURE_H
#define CCD_TEMPERATURE_H

extern int CCD_Temperature_Get(double *sensor_temperature);
extern int CCD_Temperature_Get_Temperature_Status_String(char *status_string,int string_length);
extern int CCD_Temperature_Is_Stabilised(void);
extern int CCD_Temperature_Get_Error_Number(void);
extern void CCD_Temperature_Error(void);
extern void CCD_Temperature_Error_String(char *error_string);

#endif
