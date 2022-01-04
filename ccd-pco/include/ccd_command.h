/* ccd_command.h */

#ifndef CCD_COMMAND_H
#define CCD_COMMAND_H

/* enumerations */
/**
 * Trigger mode, used to determine how exposures are started by the camera head.
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

/**
 * Timebase, used to decide what units are used when specifying exposure/delay lengths.
 * <ul>
 * <li><b>CCD_COMMAND_TIMEBASE_NS</b> We are using nanoseconds to specify exposure/delay lengths.
 * <li><b>CCD_COMMAND_TIMEBASE_US</b> We are using microseconds to specify exposure/delay lengths.
 * <li><b>CCD_COMMAND_TIMEBASE_MS</b> We are using milliseconds to specify exposure/delay lengths.
 * </ul>
 */
enum CCD_COMMAND_TIMEBASE
{
	CCD_COMMAND_TIMEBASE_NS = 0x0,
	CCD_COMMAND_TIMEBASE_US = 0x1,
	CCD_COMMAND_TIMEBASE_MS = 0x2
};

/**
 * Timestamp mode, used to determine what timestamp data is included in the read-out.
 * <ul>
 * <li><b>CCD_COMMAND_TIMESTAMP_MODE_OFF</b> No timestamp data in the image.
 * <li><b>CCD_COMMAND_TIMESTAMP_MODE_BINARY</b> A BCD encoded timestamp is in the first 14 pixels of the image.
 * <li><b>CCD_COMMAND_TIMESTAMP_MODE_BINARY_ASCII</b> A BCD encoded timestamp is in the first 14 pixels of the image,
 *                                                  and an ASCII representation as well.
 * <li><b>CCD_COMMAND_TIMESTAMP_MODE_ASCII</b> An ASCII representation of the timestamp is 
 *                                            in the top corner of the image.
 * </ul>
 */
enum CCD_COMMAND_TIMESTAMP_MODE
{
	CCD_COMMAND_TIMESTAMP_MODE_OFF          = 0x0000,
	CCD_COMMAND_TIMESTAMP_MODE_BINARY       = 0x0001,
	CCD_COMMAND_TIMESTAMP_MODE_BINARY_ASCII = 0x0002,
	CCD_COMMAND_TIMESTAMP_MODE_ASCII        = 0x0003
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
	extern int CCD_Command_Set_Timestamp_Mode(enum CCD_COMMAND_TIMESTAMP_MODE mode);
	extern int CCD_Command_Set_Timebase(enum CCD_COMMAND_TIMEBASE delay_timebase,
					    enum CCD_COMMAND_TIMEBASE exposure_timebase);
	extern int CCD_Command_Set_Delay_Exposure_Time(int delay_time,int exposure_time);
	extern int CCD_Command_Set_ADC_Operation(int num_adcs);
	extern int CCD_Command_Set_Bit_Alignment(int bit_alignment);
	extern int CCD_Command_Set_Noise_Filter_Mode(int mode);
	extern int CCD_Command_Set_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE mode);
	extern int CCD_Command_Set_Binning(int bin_x,int bin_y);
	extern int CCD_Command_Set_ROI(int start_x,int start_y,int end_x,int end_y);
	extern int CCD_Command_Set_Cooling_Setpoint_Temperature(int temperature);
	extern int CCD_Command_Grabber_Acquire_Image_Async_Wait(void *image_buffer);
	extern int CCD_Command_Grabber_Acquire_Image_Async_Wait_Timeout(void *image_buffer,int timeout_ms);
	extern int CCD_Command_Get_Temperature(int *valid_sensor_temp,double *sensor_temp,int *camera_temp,
					       int *valid_psu_temp,int *psu_temp);
	extern int CCD_Command_Description_Get_Num_ADCs(int *adc_count);
	extern int CCD_Command_Description_Get_Exposure_Time_Min(double *minimum_exposure_length_s);
	extern int CCD_Command_Description_Get_Exposure_Time_Max(double *maximum_exposure_length_s);
	extern int CCD_Command_Description_Get_Max_Horizontal_Size(int *max_hor_size);
	extern int CCD_Command_Description_Get_Max_Vertical_Size(int *max_ver_size);
	extern int CCD_Command_Description_Get_Default_Cooling_Setpoint(int *temperature);
	extern int CCD_Command_Description_Get_Min_Cooling_Setpoint(int *temperature);
	extern int CCD_Command_Description_Get_Max_Cooling_Setpoint(int *temperature);
	extern int CCD_Command_Description_Get_Sensor_Type(int *sensor_type,int *sensor_subtype);
	extern int CCD_Command_Get_Camera_Type(int *camera_type,int *serial_number);
	extern int CCD_Command_Get_ROI(int *start_x,int *start_y,int *end_x,int *end_y);
	extern int CCD_Command_Get_Actual_Size(int *image_width,int *image_height);
	extern int CCD_Command_Get_Image_Size_Bytes(int *image_size);
	extern int CCD_Command_Get_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE *mode);
	extern int CCD_Command_Get_Delay_Exposure_Time(int *delay_time,int *exposure_time);
	extern int CCD_Command_Get_Cooling_Setpoint_Temperature(int *temperature);
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
