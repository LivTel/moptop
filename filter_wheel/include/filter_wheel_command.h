/* filter_wheel_command.h
** $Header$
*/

#ifndef FILTER_WHEEL_COMMAND_H
#define FILTER_WHEEL_COMMAND_H
extern int Filter_Wheel_Command_Open(char *device_name);
extern int Filter_Wheel_Command_Close(void);
extern int Filter_Wheel_Command_Move(int position);
extern int Filter_Wheel_Command_Get_Position(int *position);
extern void Filter_Wheel_Command_Error(void);
extern void Filter_Wheel_Command_Error_String(char *error_string);
/*
** $Log$
*/

#endif
