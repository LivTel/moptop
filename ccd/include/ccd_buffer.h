/* ccd_buffer.h */

#ifndef CCD_BUFFER_H
#define CCD_BUFFER_H

extern int CCD_Buffer_Initialise(void);
extern int CCD_Buffer_Free(void);
extern int CCD_Buffer_Queue_Images(int image_count);
extern int CCD_Buffer_Get_Error_Number(void);
extern void CCD_Buffer_Error(void);
extern void CCD_Buffer_Error_String(char *error_string);

#endif
