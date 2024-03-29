/* ccd_setup.h */
#ifndef CCD_SETUP_H
#define CCD_SETUP_H

#include "ccd_command.h" /* for CCD_COMMAND_TIMESTAMP_MODE declaration */

/**
 * Is the specified binning value valid. Valid binning numbers are: 1,2,?. 
 * @param b The binning value to test.
 * @return TRUE if the binning is a valid number and FALSE if it not.
 */
#define CCD_SETUP_BINNING_IS_VALID(b) ((b==1)||(b==2))

/*  the following 3 lines are needed to support C++ compilers */
#ifdef __cplusplus
extern "C" {
#endif

extern void CCD_Setup_Set_Board(int board);
extern void CCD_Setup_Set_Camera_Setup(enum CCD_COMMAND_SETUP_FLAG setup_flag);
extern void CCD_Setup_Set_Timestamp_Mode(enum CCD_COMMAND_TIMESTAMP_MODE mode);
extern int CCD_Setup_Startup(void);
extern int CCD_Setup_Shutdown(void);
extern int CCD_Setup_Dimensions(int bin);
extern int CCD_Setup_Get_Binning(void);
extern int CCD_Setup_Get_Serial_Number(int *serial_number);
extern float CCD_Setup_Get_Pixel_Width(void);
extern float CCD_Setup_Get_Pixel_Height(void);
extern int CCD_Setup_Get_Sensor_Width(void);
extern int CCD_Setup_Get_Sensor_Height(void);
extern int CCD_Setup_Get_Image_Size_Bytes(void);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
