/* filter_wheel_command.h */

#ifndef FILTER_WHEEL_COMMAND_H
#define FILTER_WHEEL_COMMAND_H
/**
 * The number of filters in the wheel.
 */
#define FILTER_WHEEL_COMMAND_FILTER_COUNT    (5)

extern int Filter_Wheel_Command_Open(char *device_name);
extern int Filter_Wheel_Command_Close(void);
extern int Filter_Wheel_Command_Move(int position);
extern int Filter_Wheel_Command_Get_Position(int *position);
extern int Filter_Wheel_Command_Get_Filter_Count(int *filter_count);
extern int Filter_Wheel_Command_Get_Error_Number(void);
extern void Filter_Wheel_Command_Error(void);
extern void Filter_Wheel_Command_Error_String(char *error_string);

#endif
