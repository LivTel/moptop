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
	extern int CCD_Command_Arm_Camera(void);
	extern int CCD_Command_Set_Camera_To_Current_Time(void);
	extern int CCD_Command_Set_Recording_State(int rec_state);
	extern int CCD_Command_Reset_Settings(void);
	extern int CCD_Command_Set_Timestamp_Mode(int mode);
	extern int CCD_Command_Set_Timebase(int delay_timebase,int exposure_timebase);
	extern int CCD_Command_Set_Delay_Exposure_Time(int delay_time,int exposure_time);
	extern int CCD_Command_Set_ADC_Operation(int num_adcs);
	extern int CCD_Command_Set_Bit_Alignment(int bit_alignment);
	extern int CCD_Command_Set_Noise_Filter_Mode(int mode);
	extern int CCD_Command_Get_Temperature(int *valid_sensor_temp,double *sensor_temp,int *camera_temp,
					       int *valid_psu_temp,int *psu_temp);
	extern int CCD_Command_Description_Get_Num_ADCs(int *adc_count);
	extern int CCD_Command_Get_Image_Size_Bytes(int *image_size);
	
extern int CCD_Command_Get_Error_Number(void);
extern void CCD_Command_Error(void);
extern void CCD_Command_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
