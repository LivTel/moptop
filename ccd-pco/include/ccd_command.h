/* ccd_command.h */

#ifndef CCD_COMMAND_H
#define CCD_COMMAND_H

/*  the following 3 lines are needed to support C++ compilers */
#ifdef __cplusplus
extern "C" {
#endif

/* functions */
extern int CCD_Command_Initialise(void);
extern int CCD_Command_Finalise(void);
extern int CCD_Command_Open(int board);
extern int CCD_Command_Close(void);

	extern int CCD_Command_Set_Camera_To_Current_Time(void);
	extern int CCD_Command_Get_Temperature(int *valid_sensor_temp,double *sensor_temp,int *camera_temp,
					       int *valid_psu_temp,int *psu_temp);
	extern int CCD_Command_Get_Image_Size_Bytes(int *image_size);
	
extern int CCD_Command_Get_Error_Number(void);
extern void CCD_Command_Error(void);
extern void CCD_Command_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
