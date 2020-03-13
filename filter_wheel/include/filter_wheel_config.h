/* filter_wheel_config.h */

#ifndef FILTER_WHEEL_CONFIG_H
#define FILTER_WHEEL_CONFIG_H

#include "estar_config.h"

extern int Filter_Wheel_Config_Initialise(eSTAR_Config_Properties_t Config_Properties);
extern int Filter_Wheel_Config_Name_To_Position(char* filter_name,int *position);
extern int Filter_Wheel_Config_Position_To_Name(int position,char* filter_name);
extern int Filter_Wheel_Config_Get_Error_Number(void);
extern void Filter_Wheel_Config_Error(void);
extern void Filter_Wheel_Config_Error_String(char *error_string);
#endif
