/* ccd_setup.h */
#ifndef CCD_SETUP_H
#define CCD_SETUP_H
/**
 * Is the specified binning value valid. Valid binning numbers are: 1,2,?. 
 * TODO diddly
 * @param b The binning value to test.
 * @return TRUE if the binning is a valid number and FALSE if it not.
 */
#define CCD_SETUP_BINNING_IS_VALID(b) ((b==1)||(b==2))

extern int CCD_Setup_Startup(void);
extern int CCD_Setup_Shutdown(void);
extern int CCD_Setup_Dimensions(int bin);
extern int CCD_Setup_Get_Binning(void);
extern int CCD_Setup_Get_Serial_Number(char *serial_number_string,int string_length);
extern int CCD_Setup_Get_Firmware_Version(char *firmware_version_string,int string_length);
extern int CCD_Setup_Get_Readout_Time(void);
extern int CCD_Setup_Get_Bytes_Per_Pixel(void);
extern float CCD_Setup_Get_Pixel_Width(void);
extern float CCD_Setup_Get_Pixel_Height(void);
extern int CCD_Setup_Get_Sensor_Width(void);
extern int CCD_Setup_Get_Sensor_Height(void);
extern long long int CCD_Setup_Get_Timestamp_Clock_Frequency(void);
extern int CCD_Setup_Get_Image_Size_Bytes(void);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);

#endif
