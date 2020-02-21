/* ccd_setup.c
** Moptop Andor Zyla CCD library
** $Header$
*/
/**
 * Routines to setup and configure the CCD.
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"
#include "ccd_setup.h"

/* hash defines */
/**
 * The maximum length of enumerated value strings in Setup_Data.
 * The longest SensorReadoutMode value is 25 characters long.
 * The longest SimplePreAmpGainControl value is 40 characters long.
 */
#define SETUP_ENUM_VALUE_STRING_LENGTH (64)

/* data types */
/**
 * Data type holding local data to ccd_command. This consists of the following:
 * <dl>
 * <dt>Sensor_Readout_Mode</dt> <dd>Enumerated string that configures the direction in which the 
 *                              sensor will be read out. The string has length SETUP_ENUM_VALUE_STRING_LENGTH.
 *                              See possible values in Andor_SDK3_Manual.pdf, P39/P40.</dd>
 * <dt>Pre_Amp_Gain_Control</dt> <dd>Enumerated value string that configures sensitivity and dynamic range using
 *                         the SimplePreAmpGainControl feature.The string has length SETUP_ENUM_VALUE_STRING_LENGTH.
 *                         See the Andor SDK manual Andor_SDK3_Manual.pdf P40 for possible values.</dd>
 * <dt>Pixel_Encoding</dt> <dd>Enumerated value string that configures the format of the data stream using the
 *                         PixelEncoding feature. The string has length SETUP_ENUM_VALUE_STRING_LENGTH.
 *                         See possible values in Andor_SDK3_Manual.pdf, P43.</dd>
 * <dt>Pixel_Readout_Rate</dt> <dd>Enumerated value string that configures the pixel readout rate using the
 *                             PixelReadoutRate feature. The string has length SETUP_ENUM_VALUE_STRING_LENGTH. 
 *                             See possible values in Andor_SDK3_Manual.pdf, P44.</dd>
 * <dt>Binning</dt> <dd>The readout binning, stored as an integer. Can be one of 1,2,3,4,8. 
 *                  See possible values for feature "AOIBinning" in Andor_SDK3_Manual.pdf, P46.</dd>
 * <dt>Full_AOI_Control</dt> <dd>Integer holding whether or not the camera supports arbitrary AOI selection.
 *                           Retrieved from the camera/Andor library during CCD_Setup_Startup.</dd>
 * <dt>Serial_Number</dt> <dd>A character string containing the serial number retrieved from the Andor camera head, 
 *                            of length SETUP_ENUM_VALUE_STRING_LENGTH. 
 *                            Retrieved from the camera/Andor library during CCD_Setup_Startup.</dd>
 * <dt>Firmware_Version</dt> <dd>A character string containing the firmware version retrieved from the 
 *                            Andor camera head, of length SETUP_ENUM_VALUE_STRING_LENGTH. 
 *                            Retrieved from the camera/Andor library during CCD_Setup_Startup.</dd>
 * <dt>Readout_Time</dt> <dd>An integer storing the length of time to readout the camera in milliseconds. 
 *                       This is retrieved from the camera during startup using CCD_Command_Get_Readout_Time, 
 *                       and converted from decimal seconds to milliseconds.</dd>
 * <dt>Bytes_Per_Pixel</dt> <dd>An integer storing the bytes per pixel retrieved from the camera during startup 
 *                          using CCD_Command_Get_Bytes_Per_Pixel, and converted to an integer.</dd>
 * <dt>Pixel_Width</dt> <dd>A double storing the pixel width in micrometers retrieved from the camera 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Pixel_Height</dt> <dd>A double storing the pixel height in micrometers retrieved from the camera 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Sensor_Width</dt> <dd>An integer storing the sensor width in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Sensor_Height</dt> <dd>An integer storing the sensor height in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Timestamp_Clock_Frequency</dt> <dd>An integer storing the timestamp clock frequency in Hertz 
 *                                    retrieved from the camera during CCD_Setup_Startup.</dd>
 * <dt>Image_Size_Bytes</dt> <dd>An integer storing the image size in bytes.</dd>
 * </dl>
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 */
struct Setup_Struct
{
	char Sensor_Readout_Mode[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Pre_Amp_Gain_Control[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Pixel_Encoding[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Pixel_Readout_Rate[SETUP_ENUM_VALUE_STRING_LENGTH];
	int Binning;
	int Full_AOI_Control;
	char Serial_Number[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Firmware_Version[SETUP_ENUM_VALUE_STRING_LENGTH];
	int Readout_Time;
	int Bytes_Per_Pixel;
	double Pixel_Width;
	double Pixel_Height;
	int Sensor_Width;
	int Sensor_Height;
	int Timestamp_Clock_Frequency;
	int Image_Size_Bytes;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Setup_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Sensor_Readout_Mode</dt> <dd>CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS</dd>
 * <dt>Pre_Amp_Gain_Control</dt> <dd>CCD_COMMAND_AMP_GAIN_16BIT_LOW</dd>
 * <dt>Pixel_Encoding</dt> <dd>CCD_COMMAND_PIXEL_ENCODING_16_BIT</dd>
 * <dt>Pixel_Readout_Rate</dt> <dd>CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ</dd>
 * <dt>Binning</dt> <dd>1</dd>
 * <dt>Full_AOI_Control</dt> <dd>-1</dd>
 * <dt>Serial_Number</dt> <dd>""</dd>
 * <dt>Firmware_Version</dt> <dd>""</dd>
 * <dt>Readout_Time</dt> <dd>-1</dd>
 * <dt>Bytes_Per_Pixel</dt> <dd>-1</dd>
 * <dt>Pixel_Width</dt> <dd>0.0</dd>
 * <dt>Pixel_Height</dt> <dd>0.0</dd>
 * <dt>Sensor_Width</dt> <dd>0</dd>
 * <dt>Sensor_Height</dt> <dd>0</dd>
 * <dt>Timestamp_Clock_Frequency</dt> <dd>0</dd>
 * <dt>Image_Size_Bytes</dt> <dd>0</dd>
 * </dl>
 * @see ccd_command.html#CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS
 * @see ccd_command.html#CCD_COMMAND_AMP_GAIN_16BIT_LOW
 * @see ccd_command.html#CCD_COMMAND_PIXEL_ENCODING_16_BIT
 * @see ccd_command.html#CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ
 */
static struct Setup_Struct Setup_Data = 
{
	CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS,
	CCD_COMMAND_AMP_GAIN_16BIT_LOW,
	CCD_COMMAND_PIXEL_ENCODING_16_BIT,
	CCD_COMMAND_PIXEL_READOUT_RATE_100_MHZ,
	1,-1,"","",-1,-1,0.0,0.0,0,0,0,0
};

/**
 * Variable holding error code of last operation performed.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Setup_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Do the initial setup for an Andor Zyla camera.
 * <ul>
 * <li>We initialise the libraries used using CCD_Command_Initialise.
 * <li>We open a connection to the CCD camera using CCD_Command_Open.
 * <li>We turn on sensor cooling using CCD_Command_Set_Sensor_Cooling.
 * <li>We turn on frame metadata using CCD_Command_Set_Metadata_Enable.
 * <li>We turn on frame timestamp metadata using CCD_Command_Set_Metadata_Timestamp.
 * <li>We turn off the spurious noise filter using CCD_Command_Set_Spurious_Noise_Filter.
 * <li>We turn off the static blemish correction using CCD_Command_Set_Static_Blemish_Correction.
 * <li>We turn on rolling shutter global clear readout mode using CCD_Command_Set_Rolling_Shutter_Global_Clear.
 * <li>We configure the direction in which the sensor will be read out, usingCCD_Command_Set_Sensor_Readout_Mode
 *     with the value stored in Setup_Data.Sensor_Readout_Mode.
 * <li>We configure the sensitivity and dynamic range using CCD_Command_Set_Simple_Pre_Amp_Gain_Control with the 
 *     value stored in Setup_Data.Pre_Amp_Gain_Control.
 * <li>We configure the format of the data stream using CCD_Command_Set_Pixel_Encoding with the 
 *     value stored in Setup_Data.Pixel_Encoding.
 * <li>We configure the pixel readout rate using CCD_Command_Set_Pixel_Readout_Rate with the 
 *     value stored in Setup_Data.Pixel_Readout_Rate.
 * <li>We configure the cycle mode to be Continuous using CCD_Command_Set_Cycle_Mode.
 * <li>We retrieve and store whether or not the camera supports arbitrary AOI selection by calling 
 *     CCD_Command_Get_Full_AOI_Control and storing the result in Setup_Data.Full_AOI_Control.
 * <li>We retrieve and store the camera's serial number using CCD_Command_Get_Serial_Number to store the returned string
 *     in Setup_Data.Serial_Number.
 * <li>We retrieve and store the camera's firmware version using CCD_Command_Get_Firmware_Version 
 *     to store the returned string in Setup_Data.Firmware_Version.
 * <li>We get the number of bytes per pixel from the camera/Andor library using CCD_Command_Get_Bytes_Per_Pixel,
 *     and store the returned value (as an integer) in Setup_Data.Bytes_Per_Pixel.
 * <li>We get the pixel width from the camera/Andor library using CCD_Command_Get_Pixel_Width,
 *     and store the returned value in Setup_Data.Pixel_Width.
 * <li>We get the pixel height from the camera/Andor library using CCD_Command_Get_Pixel_Height,
 *     and store the returned value in Setup_Data.Pixel_Height.
 * <li>We get the sensor width from the camera/Andor library using CCD_Command_Get_Sensor_Width,
 *     and store the returned value in Setup_Data.Sensor_Width.
 * <li>We get the sensor height from the camera/Andor library using CCD_Command_Get_Sensor_Height,
 *     and store the returned value in Setup_Data.Sensor_Height.
 * <li>We get the timestamp clock frequency from the camera/Andor library using 
 *     CCD_Command_Get_Timestamp_Clock_Frequency, and store the returned value in Setup_Data.Timestamp_Clock_Frequency.
 * <li>We get the image size in bytes from the camera/Andor library using CCD_Command_Get_Image_Size_Bytes,
 *     and store the returned value in Setup_Data.Image_Size_Bytes.
 * <ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Initialise
 * @see ccd_command.html#CCD_Command_Open
 * @see ccd_command.html#CCD_Command_Set_Sensor_Cooling
 * @see ccd_command.html#CCD_Command_Set_Metadata_Enable
 * @see ccd_command.html#CCD_Command_Set_Metadata_Timestamp
 * @see ccd_command.html#CCD_Command_Set_Spurious_Noise_Filter
 * @see ccd_command.html#CCD_Command_Set_Static_Blemish_Correction
 * @see ccd_command.html#CCD_Command_Set_Rolling_Shutter_Global_Clear
 * @see ccd_command.html#CCD_Command_Set_Sensor_Readout_Mode
 * @see ccd_command.html#CCD_Command_Set_Simple_Pre_Amp_Gain_Control
 * @see ccd_command.html#CCD_Command_Set_Pixel_Encoding
 * @see ccd_command.html#CCD_Command_Set_Pixel_Readout_Rate
 * @see ccd_command.html#CCD_Command_Set_Cycle_Mode
 * @see ccd_command.html#CCD_Command_Get_Full_AOI_Control
 * @see ccd_command.html#CCD_Command_Get_Serial_Number
 * @see ccd_command.html#CCD_Command_Get_Firmware_Version
 * @see ccd_command.html#CCD_Command_Get_Bytes_Per_Pixel
 * @see ccd_command.html#CCD_Command_Get_Pixel_Width
 * @see ccd_command.html#CCD_Command_Get_Pixel_Height
 * @see ccd_command.html#CCD_Command_Get_Sensor_Width
 * @see ccd_command.html#CCD_Command_Get_Sensor_Height
 * @see ccd_command.html#CCD_Command_Get_Timestamp_Clock_Frequency
 * @see ccd_command.html#CCD_Command_Get_Image_Size_Bytes
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Setup_Startup(void)
{
	double dvalue;

	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Started.");
#endif /* LOGGING */
	/* initialise the Andor libraries */
	if(!CCD_Command_Initialise())
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise failed.");
		return FALSE;
	}
	/* open a connection to the CCD camera */
	if(!CCD_Command_Open())
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Open failed.");
		return FALSE;
	}
	/* initial configuration of the camera */
	/* turn cooling on */
	if(!CCD_Command_Set_Sensor_Cooling(TRUE))
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Sensor_Cooling(TRUE) failed.");
		return FALSE;
	}
	/* turn on frame metadata */
	if(!CCD_Command_Set_Metadata_Enable(TRUE))
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Metadata_Enable(TRUE) failed.");
		return FALSE;
	}
	/* turn on frame timestamp metadata */
	if(!CCD_Command_Set_Metadata_Timestamp(TRUE))
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Metadata_Timestamp(TRUE) failed.");
		return FALSE;
	}
	/* turn off the spurious noise filter */
	if(!CCD_Command_Set_Spurious_Noise_Filter(FALSE))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Spurious_Noise_Filter(FALSE) failed.");
		return FALSE;
	}
	/* turn off the static blemish correction */
	if(!CCD_Command_Set_Static_Blemish_Correction(FALSE))
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Static_Blemish_Correction(FALSE) failed.");
		return FALSE;
	}
	/* turn on the rolling shutter global clear readout mode */
	if(!CCD_Command_Set_Rolling_Shutter_Global_Clear(TRUE))
	{
		Setup_Error_Number = 10;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Rolling_Shutter_Global_Clear(TRUE) failed.");
		return FALSE;
	}
	/* set the direction the sensor reads out in */
	if(!CCD_Command_Set_Sensor_Readout_Mode(Setup_Data.Sensor_Readout_Mode))
	{
		Setup_Error_Number = 11;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Sensor_Readout_Mode(%s) failed.",
			Setup_Data.Sensor_Readout_Mode);
		return FALSE;
	}
	/* configure sensitivity and dynamic range options. */
	if(!CCD_Command_Set_Simple_Pre_Amp_Gain_Control(Setup_Data.Pre_Amp_Gain_Control))
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Simple_Pre_Amp_Gain_Control(%s) failed.",
			Setup_Data.Pre_Amp_Gain_Control);
		return FALSE;
	}
	/* configure the format of the data stream. */
	if(!CCD_Command_Set_Pixel_Encoding(Setup_Data.Pixel_Encoding))
	{
		Setup_Error_Number = 13;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Pixel_Encoding(%s) failed.",
			Setup_Data.Pixel_Encoding);
		return FALSE;
	}
	/* configure the pixel readout rate */
	if(!CCD_Command_Set_Pixel_Readout_Rate(Setup_Data.Pixel_Readout_Rate))
	{
		Setup_Error_Number = 14;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Pixel_Readout_Rate(%s) failed.",
			Setup_Data.Pixel_Readout_Rate);
		return FALSE;
	}
	/* configure cycle mode to be continuous. */
	if(!CCD_Command_Set_Cycle_Mode(CCD_COMMAND_CYCLE_MODE_CONTINUOUS))
	{
		Setup_Error_Number = 15;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Cycle_Mode(%s) failed.",
			CCD_COMMAND_CYCLE_MODE_CONTINUOUS);
		return FALSE;
	}
	/* diddly AOIBinning not set here unlike cam_conf */
	/* diddly TriggerMode not set here unlike cam_conf */
	/* diddly ExposureTime not set here unlike cam_conf */
	/* retrieve and store whether or not the camera supports arbitrary AOI selection */
	if(!CCD_Command_Get_Full_AOI_Control(&(Setup_Data.Full_AOI_Control)))
	{
		Setup_Error_Number = 18;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Full_AOI_Control failed.");
		return FALSE;
	}
	/* get the camera serial number */
	if(!CCD_Command_Get_Serial_Number(Setup_Data.Serial_Number,SETUP_ENUM_VALUE_STRING_LENGTH))
	{
		Setup_Error_Number = 19;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Serial_Number failed.");
		return FALSE;
	}
	/* get the camera firmware version */
	if(!CCD_Command_Get_Firmware_Version(Setup_Data.Firmware_Version,SETUP_ENUM_VALUE_STRING_LENGTH))
	{
		Setup_Error_Number = 20;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Firmware_Version failed.");
		return FALSE;
	}
	/* diddly ExposureTime not retrieved here unlike cam_conf */
	/* get the actual length of time to readout the chip according to the Andor library/camera. 
	** Returned in decimal seconds and converted/stored in milliseconds. */
	if(!CCD_Command_Get_Readout_Time(&dvalue))
	{
		Setup_Error_Number = 21;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Readout_Time failed.");
		return FALSE;
	}
	Setup_Data.Readout_Time = (int)(dvalue*((double)CCD_GENERAL_ONE_SECOND_MS));
	/* get the bytes per pixel from the Andor library/camera. Convert from double to integer. */
	if(!CCD_Command_Get_Bytes_Per_Pixel(&dvalue))
	{
		Setup_Error_Number = 22;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Bytes_Per_Pixel failed.");
		return FALSE;
	}
	Setup_Data.Bytes_Per_Pixel = (int)dvalue;
	/* get the pixel width */
	if(!CCD_Command_Get_Pixel_Width(&(Setup_Data.Pixel_Width)))
	{
		Setup_Error_Number = 23;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Pixel_Width failed.");
		return FALSE;
	}
	/* get the pixel height */
	if(!CCD_Command_Get_Pixel_Height(&(Setup_Data.Pixel_Height)))
	{
		Setup_Error_Number = 24;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Pixel_Height failed.");
		return FALSE;
	}
	/* get the sensor width */
	if(!CCD_Command_Get_Sensor_Width(&(Setup_Data.Sensor_Width)))
	{
		Setup_Error_Number = 25;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Sensor_Width failed.");
		return FALSE;
	}
	/* get the sensor height */
	if(!CCD_Command_Get_Sensor_Height(&(Setup_Data.Sensor_Height)))
	{
		Setup_Error_Number = 26;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Sensor_Height failed.");
		return FALSE;
	}
	/* get the timestamp clock frequency */
	if(!CCD_Command_Get_Timestamp_Clock_Frequency(&(Setup_Data.Timestamp_Clock_Frequency)))
	{
		Setup_Error_Number = 27;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Timestamp_Clock_Frequency failed.");
		return FALSE;
	}
	/* get the current image size in bytes */
	if(!CCD_Command_Get_Image_Size_Bytes(&(Setup_Data.Image_Size_Bytes)))
	{
		Setup_Error_Number = 28;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Get_Image_Size_Bytes failed.");
		return FALSE;
	}
	/* diddly GetFloatMin ExposureTime not done here unlike cam_conf */
	/* diddly GetFloatMax ExposureTime not done here unlike cam_conf */
	if(!CCD_Command_Flush())
	{
		Setup_Error_Number = 30;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Flush failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Shutdown the connection to the CCD.
 * <ul>
 * <li>We close connection to the CCD camera using CCD_Command_Close.
 * <li>We finalise the libraries used using CCD_Command_Finalise.
 * <ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_command.html#CCD_Command_Finalise
 * @see ccd_command.html#CCD_Command_Close
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Setup_Shutdown(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Shutdown: Started.");
#endif /* LOGGING */
	/* close the open connection to the CCD camera */
	if(!CCD_Command_Close())
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"CCD_Setup_Shutdown: CCD_Command_Close failed.");
		return FALSE;
	}
	/* shutdown the Andor library */
	if(!CCD_Command_Finalise())
	{
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"CCD_Setup_Shutdown: CCD_Command_Finalise failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Shutdown: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Setup binning and other per exposure configuration.
 * <ul>
 * <li>We use CCD_SETUP_BINNING_IS_VALID to check the binning parameter is a supported binning.
 * <li>We store the binning in Setup_Data.Binning.
 * <li>We convert the binning number to a suitable string, one of CCD_COMMAND_AOI_BINNING_1, 
 *     CCD_COMMAND_AOI_BINNING_2, CCD_COMMAND_AOI_BINNING_3, CCD_COMMAND_AOI_BINNING_4, CCD_COMMAND_AOI_BINNING_8.
 * <li>We call CCD_Command_Set_AOI_Binning with the string to set the binning in the Andor library/camera.
 * <li>We call CCD_Command_Get_Image_Size_Bytes to retrieve the (potentially different) image size in bytes.
 * </ul>
 * @param bin The binning to apply to the readout. The following values are supported: 1,2,3,4,8. Only
 *        square binning is supported. See AOIBinning feature in the Andor_SDK3_Manual.pdf, P46.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_SETUP_BINNING_IS_VALID
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Set_AOI_Binning
 * @see ccd_command.html#CCD_Command_Get_Image_Size_Bytes
 * @see ccd_command.html#CCD_COMMAND_AOI_BINNING_1
 * @see ccd_command.html#CCD_COMMAND_AOI_BINNING_2
 * @see ccd_command.html#CCD_COMMAND_AOI_BINNING_3
 * @see ccd_command.html#CCD_COMMAND_AOI_BINNING_4
 * @see ccd_command.html#CCD_COMMAND_AOI_BINNING_8
 */
int CCD_Setup_Dimensions(int bin)
{
	char aoi_binning_string[SETUP_ENUM_VALUE_STRING_LENGTH];

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Dimensions: Started.");
#endif /* LOGGING */
	if(!CCD_SETUP_BINNING_IS_VALID(bin))
	{
		Setup_Error_Number = 16;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: Binning %d not valid.",bin);
		return FALSE;
	}
	/* save the binning for later retrieval */
	Setup_Data.Binning = bin;
	/* convert the binning to a string suitable for sending to the Andor API. */
	switch(Setup_Data.Binning)
	{
		case 1:
			strcpy(aoi_binning_string,CCD_COMMAND_AOI_BINNING_1);
			break;
		case 2:
			strcpy(aoi_binning_string,CCD_COMMAND_AOI_BINNING_2);
			break;
		case 3:
			strcpy(aoi_binning_string,CCD_COMMAND_AOI_BINNING_3);
			break;
		case 4:
			strcpy(aoi_binning_string,CCD_COMMAND_AOI_BINNING_4);
			break;
		case 8:
			strcpy(aoi_binning_string,CCD_COMMAND_AOI_BINNING_8);
			break;
	}
	/* Configure the Andor AOI binning */
	if(!CCD_Command_Set_AOI_Binning(aoi_binning_string))
	{
		Setup_Error_Number = 17;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Set_AOI_Binning(%d = %s) failed.",
			Setup_Data.Binning,aoi_binning_string);
		return FALSE;
	}
	/* update the image size in bytes, that changes with binning */
	if(!CCD_Command_Get_Image_Size_Bytes(&(Setup_Data.Image_Size_Bytes)))
	{
		Setup_Error_Number = 29;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Get_Image_Size_Bytes failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Dimensions: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Return the current used readout binning.
 * @return The readout binning, as an integer, Can be one of: 1,2,3,4,8.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Binning(void)
{
	return Setup_Data.Binning;
}

/**
 * Routine to retrieve the camera serial number string, saved from the camera in CCD_Setup_Startup to Setup_Data.
 * @param serial_number_string A string of length string_length bytes to copy the camera serial number string stored
 *        in Setup_Data.Serial_Number into. Ideally should be of length SETUP_ENUM_VALUE_STRING_LENGTH.
 * @param string_length The maximum length of characters that can be stored in serial_number_string.
 * @return The string returns TRUE on success and FALSE on failure (the serial_number_string is not long enough to store
 *         the serial number string).
 * @see #Setup_Data
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Get_Serial_Number(char *serial_number_string,int string_length)
{
	if(strlen(Setup_Data.Serial_Number) >= string_length)
	{
		Setup_Error_Number = 31;
		sprintf(Setup_Error_String,
			"CCD_Setup_Get_Serial_Number: serial_number_string length too short (%d vs %d).",
			strlen(Setup_Data.Serial_Number),string_length);
		return FALSE;
	}
	strcpy(serial_number_string,Setup_Data.Serial_Number);
	return TRUE;
}

/**
 * Routine to retrieve the camera firmware version string, saved from the camera in CCD_Setup_Startup to Setup_Data.
 * @param firmware_version_string A string of length string_length bytes to copy the camera firmware version string 
 *        stored in Setup_Data.Serial_Number into. Ideally should be of length SETUP_ENUM_VALUE_STRING_LENGTH.
 * @param string_length The maximum length of characters that can be stored in firmware_version_string.
 * @return The string returns TRUE on success and FALSE on failure 
 *         (the firmware_version_string is not long enough to store the firmware version string).
 * @see #Setup_Data
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Get_Firmware_Version(char *firmware_version_string,int string_length)
{
	if(strlen(Setup_Data.Firmware_Version) >= string_length)
	{
		Setup_Error_Number = 32;
		sprintf(Setup_Error_String,
			"CCD_Setup_Get_Serial_Number: firmware_version_string length too short (%d vs %d).",
			strlen(Setup_Data.Firmware_Version),string_length);
		return FALSE;
	}
	strcpy(firmware_version_string,Setup_Data.Firmware_Version);
	return TRUE;
}

/**
 * Return the readout time of the CCD, in milliseconds, as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The readout time of the CCD, in milliseconds.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Readout_Time(void)
{
	return Setup_Data.Readout_Time;
}

/**
 * Return the bytes per pixel returned during a readout, as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The number of bytes per pixel.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Bytes_Per_Pixel(void)
{
	return Setup_Data.Bytes_Per_Pixel;
}

/**
 * Return the physical width of a pixel, as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The physical width of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Width(void)
{
	return Setup_Data.Pixel_Width;
}

/**
 * Return the physical height of a pixel, as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The physical height of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Height(void)
{
	return Setup_Data.Pixel_Height;
}

/**
 * Return the number of pixels across the sensor (width), as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The sensor width in pixels.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Sensor_Width(void)
{
	return Setup_Data.Sensor_Width;
}

/**
 * Return the number of pixels across the sensor (height), as previously retrieved from the Andor library
 * and stored in Setup_Data.
 * @return The sensor height in pixels.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Sensor_Height(void)
{
	return Setup_Data.Sensor_Height;
}

/**
 * Return clock frequency of the internal clock used to generate frame timestamps, 
 * as previously retrieved from the Andor library and stored in Setup_Data.
 * @return The timestamp clock frequency in Hertz.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Timestamp_Clock_Frequency(void)
{
	return Setup_Data.Timestamp_Clock_Frequency;
}

/**
 * Return the size of images returned by the Andor library, 
 * as previously retrieved from the Andor library and stored in Setup_Data.
 * @return The image size in bytes.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Image_Size_Bytes(void)
{
	return Setup_Data.Image_Size_Bytes;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Setup_Error_Number
 */
int CCD_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Setup_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Setup:Error(%d) : %s\n",time_string,Setup_Error_Number,Setup_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Setup_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Setup:Error(%d) : %s\n",time_string,
		Setup_Error_Number,Setup_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
