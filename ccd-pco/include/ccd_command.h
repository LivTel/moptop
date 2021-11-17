/* ccd_command.h */

#ifndef CCD_COMMAND_H
#define CCD_COMMAND_H

/* functions */
extern int CCD_Command_Initialise(void);
extern int CCD_Command_Finalise(void);
extern int CCD_Command_Open(void);
extern int CCD_Command_Close(void);
extern int CCD_Command_Get_Error_Number(void);
extern void CCD_Command_Error(void);
extern void CCD_Command_Error_String(char *error_string);

#endif
