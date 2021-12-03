/* ccd_buffer.h */

#ifndef CCD_BUFFER_H
#define CCD_BUFFER_H

/*  the following 3 lines are needed to support C++ compilers */
#ifdef __cplusplus
extern "C" {
#endif

extern int CCD_Buffer_Initialise(void);
extern int CCD_Buffer_Free(void);
extern void *CCD_Buffer_Get_Image_Buffer(void);
extern int CCD_Buffer_Get_Error_Number(void);
extern void CCD_Buffer_Error(void);
extern void CCD_Buffer_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
