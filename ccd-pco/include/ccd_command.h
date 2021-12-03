/* ccd_command.h */

#ifndef CCD_COMMAND_H
#define CCD_COMMAND_H

/* enumerations */
/**
 * Trigger mode, used to deremine how exposures are started by the camera head.
 * <ul>
 * <li><b>CCD_COMMAND_TRIGGER_MODE_INTERNAL</b> The camera head internally triggers (software/auto sequence).
 * <li><b>CCD_COMMAND_TRIGGER_MODE_EXTERNAL</b> The exposures are externally triggered.
 * </ul>
 * There is also an 0x3 option (external exposure control) which we don't use.
 */
enum CCD_COMMAND_TRIGGER_MODE
{
	CCD_COMMAND_TRIGGER_MODE_INTERNAL = 0x0,
	CCD_COMMAND_TRIGGER_MODE_EXTERNAL = 0x2
};
	
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
	extern int CCD_Command_Grabber_Post_Arm(void);
	extern int CCD_Command_Set_Camera_To_Current_Time(void);
	extern int CCD_Command_Set_Recording_State(int rec_state);
	extern int CCD_Command_Reset_Settings(void);
	extern int CCD_Command_Set_Timestamp_Mode(int mode);
	extern int CCD_Command_Set_Timebase(int delay_timebase,int exposure_timebase);
	extern int CCD_Command_Set_Delay_Exposure_Time(int delay_time,int exposure_time);
	extern int CCD_Command_Set_ADC_Operation(int num_adcs);
	extern int CCD_Command_Set_Bit_Alignment(int bit_alignment);
	extern int CCD_Command_Set_Noise_Filter_Mode(int mode);
	extern int CCD_Command_Set_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE mode);
	extern int CCD_Command_Set_Binning(int bin_x,int bin_y);
	extern int CCD_Command_Grabber_Acquire_Image_Async_Wait(void *image_buffer);
	extern int CCD_Command_Get_Temperature(int *valid_sensor_temp,double *sensor_temp,int *camera_temp,
					       int *valid_psu_temp,int *psu_temp);
	extern int CCD_Command_Description_Get_Num_ADCs(int *adc_count);
	extern int CCD_Command_Get_Exposure_Time_Min(double *minimum_exposure_length_s);
	extern int CCD_Command_Get_Exposure_Time_Max(double *maximum_exposure_length_s);
	extern int CCD_Command_Get_Actual_Size(int *image_width,int *image_height);
	extern int CCD_Command_Get_Image_Size_Bytes(int *image_size);
	extern int CCD_Command_Get_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE *mode);
	extern int CCD_Command_Get_Delay_Exposure_Time(int *delay_time,int *exposure_time);
	extern int CCD_Command_Get_Image_Number_From_Metadata(void *image_buffer,size_t image_buffer_length,
							      int *image_number);
	extern int CCD_Command_Get_Timestamp_From_Metadata(void *image_buffer,size_t image_buffer_length,
							   struct timespec *camera_timestamp);
extern int CCD_Command_Get_Error_Number(void);
extern void CCD_Command_Error(void);
extern void CCD_Command_Error_String(char *error_string);

#ifdef __cplusplus
}
#endif

#endif
