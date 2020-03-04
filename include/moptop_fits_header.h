/* moptop_fits_header.h
*/
#ifndef MOPTOP_FITS_HEADER_H
#define MOPTOP_FITS_HEADER_H

#include "ccd_general.h"
#include "ccd_fits_header.h"

/* external functions */
extern int Moptop_Fits_Header_Initialise(void);
extern int Moptop_Fits_Header_String_Add(char *keyword,char *value, char *comment);
extern int Moptop_Fits_Header_Integer_Add(char *keyword,int value, char *comment);
extern int Moptop_Fits_Header_Float_Add(char *keyword,double value, char *comment);
extern int Moptop_Fits_Header_Logical_Add(char *keyword,int value, char *comment);
extern int Moptop_Fits_Header_Delete(char *keyword);
extern int Moptop_Fits_Header_Clear(void);

#endif
