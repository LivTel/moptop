/* ccd_command.h
** $Header$
*/

#ifndef CCD_COMMAND_H
#define CCD_COMMAND_H
/**
 * Value enumeration for CCD_Command_Set_Electronic_Shuttering_Mode, this sets the electronic shuttering mode
 * to "Rolling" (as opposed to "Global"). 
 * @see #CCD_Command_Set_Electronic_Shuttering_Mode
 */
#define CCD_COMMAND_ELECTRONIC_SHUTTERING_MODE_ROLLING          ("Rolling")
/**
 * Value enumeration for CCD_Command_Set_Sensor_Readout_Mode, 
 * this sets the direction in which the sensor will be read out to "Outside In Simultaneous".
 * See possible values in Andor_SDK3_Manual.pdf, P39/P40.
 * @see #CCD_Command_Set_Sensor_Readout_Mode
 */
#define CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS ("Outside In Simultaneous")
/**
 * Value enumeration for CCD_Command_Set_Simple_Pre_Amp_Gain_Control, which selects sensitivity and dynamic range
 * options. This selects 16 bit (low noise and high well capacity).
 * @see #CCD_Command_Set_Simple_Pre_Amp_Gain_Control
 */
#define CCD_COMMAND_AMP_GAIN_16BIT_LOW                          ("16-bit (low noise & high well capacity)")
/**
 * Value enumeration for CCD_Command_Set_Simple_Pre_Amp_Gain_Control, which selects sensitivity and dynamic range
 * options. This selects 12-bit (low noise).
 * @see #CCD_Command_Set_Simple_Pre_Amp_Gain_Control
 */
#define CCD_COMMAND_AMP_GAIN_12BIT_LOW                          ("12-bit (low noise)")
/**
 * Value enumeration for CCD_Command_Set_Simple_Pre_Amp_Gain_Control, which selects sensitivity and dynamic range
 * options. This selects 12-bit (high well capacity).
 * @see #CCD_Command_Set_Simple_Pre_Amp_Gain_Control
 */
#define CCD_COMMAND_AMP_GAIN_12BIT_HIGH                         ("12-bit (high well capacity)")

/* function hash defines that use the underlying command routines */
/**
 * Configures the state of the sensor cooling, using CCD_Command_Set_Bool with feature name "SensorCooling".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Sensor_Cooling(b)  CCD_Command_Set_Bool("SensorCooling",b)
/**
 * Turns on/off the sending of frame meta-data, using CCD_Command_Set_Bool with feature name "MetadataEnable".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Metadata_Enable(b)  CCD_Command_Set_Bool("MetadataEnable",b)
/**
 * Turns on/off the sending of frame timestamp meta-data, using CCD_Command_Set_Bool with feature name 
 * "MetadataTimestamp".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Metadata_Timestamp(b)  CCD_Command_Set_Bool("MetadataTimestamp",b)
/**
 * Turns on/off the spurious noise filter, using CCD_Command_Set_Bool with feature name "SpuriousNoiseFilter".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Spurious_Noise_Filter(b)  CCD_Command_Set_Bool("SpuriousNoiseFilter",b)
/**
 * Turns on/off the static blemish correction, using CCD_Command_Set_Bool with feature name "StaticBlemishCorrection".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Static_Blemish_Correction(b)  CCD_Command_Set_Bool("StaticBlemishCorrection",b)
/**
 * Turns on/off the rolling shutter global clear readout mode, 
 * using CCD_Command_Set_Bool with feature name "RollingShutterGlobalClear".
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Rolling_Shutter_Global_Clear(b)  CCD_Command_Set_Bool("RollingShutterGlobalClear",b)
/**
 * Configure which on-sensor electronic shuttering mode is used. 
 * using CCD_Command_Set_Enum_String with feature name "ElectronicShutteringMode".
 * @see #CCD_COMMAND_ELECTRONIC_SHUTTERING_MODE_ROLLING
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Electronic_Shuttering_Mode(s) CCD_Command_Set_Enum_String("ElectronicShutteringMode",s)
/**
 * Configure the direction in which the sensor will be read out,
 * using CCD_Command_Set_Enum_String with feature name "SensorReadoutMode".
 * See possible values in Andor_SDK3_Manual.pdf, P39/P40.
 * @see #CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Sensor_Readout_Mode(s) CCD_Command_Set_Enum_String("SensorReadoutMode",s)
/**
 * Configure selection of the sensitivity and dynamic range options.
 * using CCD_Command_Set_Enum_String with feature name "SimplePreAmpGainControl".
 * See possible values in Andor_SDK3_Manual.pdf, P40.
 * @see #CCD_COMMAND_AMP_GAIN_16BIT_LOW
 * @see #CCD_COMMAND_AMP_GAIN_12BIT_LOW
 * @see #CCD_COMMAND_AMP_GAIN_12BIT_HIGH
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Simple_Pre_Amp_Gain_Control(s) CCD_Command_Set_Enum_String("SimplePreAmpGainControl",s)



/* functions */
extern int CCD_Command_Initialise(void);
extern int CCD_Command_Finalise(void);
extern int CCD_Command_Open(void);
extern int CCD_Command_Close(void);
extern int CCD_Command_Command(char *feature_name_string);
extern int CCD_Command_Flush(void);
extern int CCD_Command_Get_Bool(char *feature_name_string,int *value);
extern int CCD_Command_Get_Enum_Index(char *feature_name_string,int *value);
extern int CCD_Command_Get_Enum_String_By_Index(char *feature_name_string,int index,
						char *value_string,int value_length);
extern int CCD_Command_Get_Float(char *feature_name_string,double *value);
extern int CCD_Command_Get_Float_Min(char *feature_name_string,double *value);
extern int CCD_Command_Get_Float_Max(char *feature_name_string,double *value);
extern int CCD_Command_Get_Int(char *feature_name_string,int *value);
extern int CCD_Command_Get_String(char *feature_name_string,char *value,int value_length);
extern int CCD_Command_Is_Readonly(char *feature_name_string,int *is_readonly);
extern int CCD_Command_Set_Bool(char *feature_name_string,int value);
extern int CCD_Command_Set_Enum_String(char *feature_name_string,char *enum_value_string);
extern int CCD_Command_Set_Float(char *feature_name_string,double value);
extern int CCD_Command_Get_Error_Number(void);
extern void CCD_Command_Error(void);
extern void CCD_Command_Error_String(char *error_string);
/*
** $Log$
*/

#endif
