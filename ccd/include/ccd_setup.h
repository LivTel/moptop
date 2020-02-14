/* ccd_command.h
** $Header$
*/

#ifndef CCD_SETUP_H
#define CCD_SETUP_H

extern int CCD_Setup_Startup(void);
extern int CCD_Setup_Shutdown(void);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);

/*
** $Log$
*/

#endif
