/* ccd_fits_header.h
*/
#ifndef CCD_FITS_HEADER_H
#define CCD_FITS_HEADER_H

/* for fitsfile declaration */
#include "fitsio.h"

extern int CCD_Fits_Header_Initialise(void);
extern int CCD_Fits_Header_Clear(void);
extern int CCD_Fits_Header_Delete(char *keyword);
extern int CCD_Fits_Header_Add_String(char *keyword,char *value,char *comment);
extern int CCD_Fits_Header_Add_Int(char *keyword,int value,char *comment);
extern int CCD_Fits_Header_Add_Long_Long_Int(char *keyword,long long int value,char *comment);
extern int CCD_Fits_Header_Add_Float(char *keyword,double value,char *comment);
extern int CCD_Fits_Header_Add_Logical(char *keyword,int value,char *comment);
extern int CCD_Fits_Header_Free(void);

extern int CCD_Fits_Header_Write_To_Fits(fitsfile *fits_fp);

extern int CCD_Fits_Header_Get_Error_Number(void);
extern void CCD_Fits_Header_Error(void);
extern void CCD_Fits_Header_Error_String(char *error_string);
#endif
