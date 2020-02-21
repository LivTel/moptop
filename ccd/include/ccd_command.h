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
/**
 * Value enumeration for CCD_Command_Set_Pixel_Encoding, which selects the format of the data stream.
 * This selects 12 bit.
 * @see #CCD_Command_Set_Pixel_Encoding
 */
#define CCD_COMMAND_PIXEL_ENCODING_12_BIT                       ("Mono12")
/**
 * Value enumeration for CCD_Command_Set_Pixel_Encoding, which selects the format of the data stream.
 * This selects 12 bit packed.
 * @see #CCD_Command_Set_Pixel_Encoding
 */
#define CCD_COMMAND_PIXEL_ENCODING_12_BIT_PACKED                ("Mono12Packed")
/**
 * Value enumeration for CCD_Command_Set_Pixel_Encoding, which selects the format of the data stream.
 * This selects 16 bit.
 * @see #CCD_Command_Set_Pixel_Encoding
 */
#define CCD_COMMAND_PIXEL_ENCODING_16_BIT                       ("Mono16")
/**
 * Value enumeration for CCD_Command_Set_Pixel_Readout_Rate, which selects the rate of pixel readout from the sensor.
 * This value selects 100MHz.
 * @see #CCD_Command_Set_Pixel_Readout_Rate
 */
#define CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ                  ("100 MHz")
/**
 * Value enumeration for CCD_Command_Set_Pixel_Readout_Rate, which selects the rate of pixel readout from the sensor.
 * This value selects 270MHz.
 * @see #CCD_Command_Set_Pixel_Readout_Rate
 */
#define CCD_COMMAND_PIXEL_READOUT_RATE_270_MHZ                  ("270 MHz")
/**
 * Value enumeration for CCD_Command_Set_Cycle_Mode, which configures whether the camera will acquire a fixed length 
 * sequence or a continuous sequence.
 * This value selects a continuous sequence.
 * @see #CCD_Command_Set_Cycle_Mode
 */
#define CCD_COMMAND_CYCLE_MODE_CONTINUOUS                        ("Continuous")
/**
 * Value enumeration for CCD_Command_Set_AOI_Binning, which configures the readout binning.
 * This value selects 1x1 binning.
 * @see #CCD_Command_Set_AOI_Binning
 */
#define CCD_COMMAND_AOI_BINNING_1                                ("1x1")
/**
 * Value enumeration for CCD_Command_Set_AOI_Binning, which configures the readout binning.
 * This value selects 2x2 binning.
 * @see #CCD_Command_Set_AOI_Binning
 */
#define CCD_COMMAND_AOI_BINNING_2                                ("2x2")
/**
 * Value enumeration for CCD_Command_Set_AOI_Binning, which configures the readout binning.
 * This value selects 3x3 binning.
 * @see #CCD_Command_Set_AOI_Binning
 */
#define CCD_COMMAND_AOI_BINNING_3                                ("3x3")
/**
 * Value enumeration for CCD_Command_Set_AOI_Binning, which configures the readout binning.
 * This value selects 4x4 binning.
 */
#define CCD_COMMAND_AOI_BINNING_4                                ("4x4")
/**
 * Value enumeration for CCD_Command_Set_AOI_Binning, which configures the readout binning.
 * This value selects 8x8 binning.
 * @see #CCD_Command_Set_AOI_Binning
 */
#define CCD_COMMAND_AOI_BINNING_8                                ("8x8")
/**
 * Value enumeration for CCD_Command_Set_Trigger_Mode, which configures the camera trigger mode.
 * This value selects software triggering.
 * @see #CCD_Command_Set_Trigger_Mode
 */
#define CCD_COMMAND_TRIGGER_MODE_SOFTWARE                        ("Software")
/**
 * Value enumeration for CCD_Command_Set_Trigger_Mode, which configures the camera trigger mode.
 * This value selects external/hardware/edge triggering.
 * @see #CCD_Command_Set_Trigger_Mode
 */
#define CCD_COMMAND_TRIGGER_MODE_EXTERNAL                        ("External")


/* function hash defines that use the underlying command routines */
/**
 * Configures the state of the sensor cooling, using CCD_Command_Set_Bool with feature name "SensorCooling".
 * @param b An integer representing a boolean, if TRUE turn on sensor cooling.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Sensor_Cooling(b)  CCD_Command_Set_Bool("SensorCooling",b)
/**
 * Turns on/off the sending of frame meta-data, using CCD_Command_Set_Bool with feature name "MetadataEnable".
 * @param b An integer representing a boolean, if TRUE turn on the sending of frame meta-data.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Metadata_Enable(b)  CCD_Command_Set_Bool("MetadataEnable",b)
/**
 * Turns on/off the sending of frame timestamp meta-data, using CCD_Command_Set_Bool with feature name 
 * "MetadataTimestamp".
 * @param b An integer representing a boolean, if TRUE turn on the sending of frame timestamp meta-data.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Metadata_Timestamp(b)  CCD_Command_Set_Bool("MetadataTimestamp",b)
/**
 * Turns on/off the spurious noise filter, using CCD_Command_Set_Bool with feature name "SpuriousNoiseFilter".
 * @param b An integer representing a boolean, if TRUE turn on the spurious noise filter.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Spurious_Noise_Filter(b)  CCD_Command_Set_Bool("SpuriousNoiseFilter",b)
/**
 * Turns on/off the static blemish correction, using CCD_Command_Set_Bool with feature name "StaticBlemishCorrection".
 * @param b An integer representing a boolean, if TRUE turn on the static blemish correction.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Static_Blemish_Correction(b)  CCD_Command_Set_Bool("StaticBlemishCorrection",b)
/**
 * Turns on/off the rolling shutter global clear readout mode, 
 * using CCD_Command_Set_Bool with feature name "RollingShutterGlobalClear".
 * @param b An integer representing a boolean, if TRUE turn on the rolling shutter global clear readout mode.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Bool
 */
#define CCD_Command_Set_Rolling_Shutter_Global_Clear(b)  CCD_Command_Set_Bool("RollingShutterGlobalClear",b)
/**
 * Configure which on-sensor electronic shuttering mode is used. 
 * using CCD_Command_Set_Enum_String with feature name "ElectronicShutteringMode".
 * @param s A string representing the shuttering mode to use, one of: CCD_COMMAND_ELECTRONIC_SHUTTERING_MODE_ROLLING.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_ELECTRONIC_SHUTTERING_MODE_ROLLING
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Electronic_Shuttering_Mode(s) CCD_Command_Set_Enum_String("ElectronicShutteringMode",s)
/**
 * Configure the direction in which the sensor will be read out,
 * using CCD_Command_Set_Enum_String with feature name "SensorReadoutMode".
 * See possible values in Andor_SDK3_Manual.pdf, P39/P40.
 * @param s A string representing the direction in which the sensor will be read out, one of: 
 *        CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Sensor_Readout_Mode(s) CCD_Command_Set_Enum_String("SensorReadoutMode",s)
/**
 * Configure selection of the sensitivity and dynamic range options.
 * Uses CCD_Command_Set_Enum_String with feature name "SimplePreAmpGainControl".
 * See possible values in Andor_SDK3_Manual.pdf, P40.
 * @param s A string representing the values to configure, one of: CCD_COMMAND_AMP_GAIN_16BIT_LOW, 
 *        CCD_COMMAND_AMP_GAIN_12BIT_LOW, CCD_COMMAND_AMP_GAIN_12BIT_HIGH.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_AMP_GAIN_16BIT_LOW
 * @see #CCD_COMMAND_AMP_GAIN_12BIT_LOW
 * @see #CCD_COMMAND_AMP_GAIN_12BIT_HIGH
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Simple_Pre_Amp_Gain_Control(s) CCD_Command_Set_Enum_String("SimplePreAmpGainControl",s)
/**
 * Configure the format of the data stream.
 * Uses the CCD_Command_Set_Enum_String with feature name "PixelEncoding".
 * See possible values in Andor_SDK3_Manual.pdf, P43.
 * @param s A string representing the pixel format to use, one of: CCD_COMMAND_PIXEL_ENCODING_12_BIT / 
 *        CCD_COMMAND_PIXEL_ENCODING_12_BIT_PACKED / CCD_COMMAND_PIXEL_ENCODING_16_BIT.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_PIXEL_ENCODING_12_BIT
 * @see #CCD_COMMAND_PIXEL_ENCODING_12_BIT_PACKED
 * @see #CCD_COMMAND_PIXEL_ENCODING_16_BIT
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Pixel_Encoding(s) CCD_Command_Set_Enum_String("PixelEncoding",s)
/**
 * Configure the rate of pixel readout from the sensor.
 * Uses the CCD_Command_Set_Enum_String with feature name "PixelReadoutRate".
 * See possible values in Andor_SDK3_Manual.pdf, P44.
 * @param s A string representing the pixel rate to use, see 
 *        CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ / CCD_COMMAND_PIXEL_READOUT_RATE_270_MHZ.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ
 * @see #CCD_COMMAND_PIXEL_READOUT_RATE_270_MHZ
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Pixel_Readout_Rate(s) CCD_Command_Set_Enum_String("PixelReadoutRate",s)
/**
 * Configure whether the camera will acquire a fixed length sequence or a continuous sequence.
 * Uses the CCD_Command_Set_Enum_String with feature name "CycleMode".
 * See possible values in Andor_SDK3_Manual.pdf, P42.
 * @param s A string representing the cycle mode e.g. CCD_COMMAND_CYCLE_MODE_CONTINUOUS.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_CYCLE_MODE_CONTINUOUS
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Cycle_Mode(s) CCD_Command_Set_Enum_String("CycleMode",s)
/**
 * Set the readout binning for the Area Of Interest.
 * Uses the CCD_Command_Set_Enum_String with feature name "AOIBinning".
 * See possible values in Andor_SDK3_Manual.pdf, P46.
 * @param s A string representing the binning to use, one of: CCD_COMMAND_AOI_BINNING_1, CCD_COMMAND_AOI_BINNING_2,
 *          CCD_COMMAND_AOI_BINNING_3, CCD_COMMAND_AOI_BINNING_4, CCD_COMMAND_AOI_BINNING_8.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_AOI_BINNING_1
 * @see #CCD_COMMAND_AOI_BINNING_2
 * @see #CCD_COMMAND_AOI_BINNING_3
 * @see #CCD_COMMAND_AOI_BINNING_4
 * @see #CCD_COMMAND_AOI_BINNING_8
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_AOI_Binning(s) CCD_Command_Set_Enum_String("AOIBinning",s)
/**
 * Set the camera trigger mode (how exposures are started).
 * Uses the CCD_Command_Set_Enum_String with feature name "TriggerMode".
 * See possible values in Andor_SDK3_Manual.pdf, P52.
 * @param s A string representing camera trigger mode, one of: CCD_COMMAND_TRIGGER_MODE_SOFTWARE, 
 *          CCD_COMMAND_TRIGGER_MODE_EXTERNAL
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see #CCD_COMMAND_TRIGGER_MODE_EXTERNAL
 * @see #CCD_Command_Set_Enum_String
 */
#define CCD_Command_Set_Trigger_Mode(s) CCD_Command_Set_Enum_String("TriggerMode",s)
/**
 * Set the exposure length. The actual exposure length used by the Andor library may be different, based on
 * camera setup constraints.
 * Uses the CCD_Command_Set_Float with feature name "ExposureTime".
 * @param f The exposure length to request, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Set_Float
 */
#define CCD_Command_Set_Exposure_Time(f) CCD_Command_Set_Float("ExposureTime",f)
/**
 * Get the actual exposure length currently being used by the Andor library.
 * Uses the CCD_Command_Get_Float with feature name "ExposureTime".
 * @param f The address of a double to store the exposure length into, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float
 */
#define CCD_Command_Get_Exposure_Time(f) CCD_Command_Get_Float("ExposureTime",f)
/**
 * Get whether or not the camera supports arbitrary AOI (area of interest) selection.
 * Uses the CCD_Command_Get_Bool with feature name "FullAOIControl".
 * @param b The address of an integer to store the returned boolean value into.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Bool
 */
#define CCD_Command_Get_Full_AOI_Control(b) CCD_Command_Get_Bool("FullAOIControl",b)
/**
 * Get the serial number of the connected camera, as a string.
 * Uses the CCD_Command_Get_String with feature name "SerialNumber".
 * @param s A string long enough to store the returned serial number into.
 * @param s_length The length of string s in bytes.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_String
 */
#define CCD_Command_Get_Serial_Number(s,s_length) CCD_Command_Get_String("SerialNumber",s,s_length)
/**
 * Get the firmware version of the connected camera, as a string.
 * Uses the CCD_Command_Get_String with feature name "FirmwareVersion".
 * @param s A string long enough to store the returned firmware version into.
 * @param s_length The length of string s in bytes.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_String
 */
#define CCD_Command_Get_Firmware_Version(s,s_length) CCD_Command_Get_String("FirmwareVersion",s,s_length)
/**
 * Get the actual length of time to readout the chip.
 * Uses the CCD_Command_Get_Float with feature name "ReadoutTime".
 * @param f The address of a double to store the readout time into, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float
 */
#define CCD_Command_Get_Readout_Time(f) CCD_Command_Get_Float("ReadoutTime",f)
/**
 * Get the number of bytes per pixel from the camera/Andor library.
 * Uses the CCD_Command_Get_Float with feature name "BytesPerPixel".
 * @param f The address of a double to store the readout time into, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float
 */
#define CCD_Command_Get_Bytes_Per_Pixel(f) CCD_Command_Get_Float("BytesPerPixel",f)
/**
 * Get the pixel width from the camera/Andor library.
 * Uses the CCD_Command_Get_Float with feature name "PixelWidth".
 * @param f The address of a double to store the pixel width, in micrometers.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float
 */
#define CCD_Command_Get_Pixel_Width(f) CCD_Command_Get_Float("PixelWidth",f)
/**
 * Get the pixel height from the camera/Andor library.
 * Uses the CCD_Command_Get_Float with feature name "PixelHeight".
 * @param f The address of a double to store the pixel height, in micrometers.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float
 */
#define CCD_Command_Get_Pixel_Height(f) CCD_Command_Get_Float("PixelHeight",f)
/**
 * Get the sensor width from the camera/Andor library.
 * Uses the CCD_Command_Get_Int with feature name "SensorWidth".
 * @param i The address of an integer to store the sensor width, in pixels.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Int
 */
#define CCD_Command_Get_Sensor_Width(i) CCD_Command_Get_Int("SensorWidth",i)
/**
 * Get the sensor height from the camera/Andor library.
 * Uses the CCD_Command_Get_Int with feature name "SensorHeight".
 * @param i The address of an integer to store the sensor height, in pixels.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Int
 */
#define CCD_Command_Get_Sensor_Height(i) CCD_Command_Get_Int("SensorHeight",i)
/**
 * Get the timestamp clock frequency from the camera/Andor library.
 * Uses the CCD_Command_Get_Int with feature name "TimestampClockFrequency".
 * @param i The address of an integer to store the timestamp clock frequency, in Hertz.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Int
 */
#define CCD_Command_Get_Timestamp_Clock_Frequency(i) CCD_Command_Get_Int("TimestampClockFrequency",i)
/**
 * Get the image size in bytes. This changes with AOI (area of interest) changes / binning etc...
 * Uses the CCD_Command_Get_Int with feature name "ImageSizeBytes".
 * @param i The address of an integer to store the image size in bytes.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Int
 */
#define CCD_Command_Get_Image_Size_Bytes(i) CCD_Command_Get_Int("ImageSizeBytes",i)
/**
 * Get the minimum exposure length allowed by the Andor library.
 * Uses the CCD_Command_Get_Float_Min with feature name "ExposureTime".
 * @param f The address of a double to store the minimum exposure length into, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float_Min
 */
#define CCD_Command_Get_Exposure_Time_Min(f) CCD_Command_Get_Float_Min("ExposureTime",f)
/**
 * Get the maximum exposure length allowed by the Andor library.
 * Uses the CCD_Command_Get_Float_Max with feature name "ExposureTime".
 * @param f The address of a double to store the maximum exposure length into, in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_Command_Get_Float_Max
 */
#define CCD_Command_Get_Exposure_Time_Max(f) CCD_Command_Get_Float_Max("ExposureTime",f)

/* functions */
extern int CCD_Command_Initialise(void);
extern int CCD_Command_Finalise(void);
extern int CCD_Command_Open(void);
extern int CCD_Command_Close(void);
extern int CCD_Command_Command(char *feature_name_string);
extern int CCD_Command_Flush(void);
extern int CCD_Command_Get_Bool(char *feature_name_string,int *value);
/* diddly todo extern int CCD_Command_Get_Enum_Count(char *feature_name_string,int *count);*/
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
