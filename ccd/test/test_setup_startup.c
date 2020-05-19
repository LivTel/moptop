/* test_setup_startup.c
** $Header$
*/
/**
 * Test the CCD library startup routine, and print out some retrieved data from the camera/Andor library.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_setup.h"
/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH        (256)
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;

static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
**          External functions 
** ------------------------------------------------------------------ */
/**
 * Main program.
 * <ul>
 * <li>We parse the arguments with Parse_Arguments.
 * <li>We setup the CCD library's logging with calls to CCD_General_Set_Log_Filter_Level, 
 *     CCD_General_Set_Log_Filter_Function, CCD_General_Log_Filter_Level_Absolute, 
 *     CCD_General_Set_Log_Handler_Function, CCD_General_Log_Handler_Stdout.
 * <li>We initialise the library, open a connection to the camera, and perform initial configuration using 
 *     CCD_Setup_Startup.
 * <li>We get a copy of the camera's serial number and firmware version using CCD_Setup_Get_Serial_Number, 
 *     CCD_Setup_Get_Firmware_Version.
 * <li>We print out data retrieved from the camera in CCD_Setup_Startup using  CCD_Setup_Get_Readout_Time, 
 *     CCD_Setup_Get_Bytes_Per_Pixel, CCD_Setup_Get_Pixel_Width, CCD_Setup_Get_Pixel_Height, 
 *     CCD_Setup_Get_Sensor_Width, CCD_Setup_Get_Sensor_Height, CCD_Setup_Get_Timestamp_Clock_Frequency, 
 *     CCD_Setup_Get_Image_Size_Bytes.
 * <li>We shutdown the connection to the library using CCD_Setup_Shutdown.
 * <li>
 * </ul>
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @see #Parse_Arguments
 * @see #Log_Level
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Level
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Filter_Level_Absolute
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Handler_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Error
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Handler_Stdout
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Startup
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Serial_Number
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Firmware_Version
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Readout_Time
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Bytes_Per_Pixel
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Width
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Height
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Width
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Height
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Timestamp_Clock_Frequency
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Get_Image_Size_Bytes
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Shutdown
 */
int main(int argc, char *argv[])
{
	char serial_number_string[64];
	char firmware_version_string[64];

	/* parse arguments */
	fprintf(stdout,"test_setup_startup : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_General_Set_Log_Filter_Level(Log_Level);
	CCD_General_Set_Log_Filter_Function(CCD_General_Log_Filter_Level_Absolute);
	CCD_General_Set_Log_Handler_Function(CCD_General_Log_Handler_Stdout);
	/* do startup */
	if(!CCD_Setup_Startup())
	{
		CCD_General_Error();
		return 2;
	}
	/* print out some retrieved data */
	if(!CCD_Setup_Get_Serial_Number(serial_number_string,64))
	{
		CCD_General_Error();
		return 3;
	}
	if(!CCD_Setup_Get_Firmware_Version(firmware_version_string,64))
	{
		CCD_General_Error();
		return 4;
	}
	fprintf(stdout,"Camera Serial Number: %s.\n",serial_number_string);
	fprintf(stdout,"Firmware Version: %s.\n",firmware_version_string);
	fprintf(stdout,"Readout Time: %d ms.\n",CCD_Setup_Get_Readout_Time());
	fprintf(stdout,"Bytes Per Pixel: %d.\n",CCD_Setup_Get_Bytes_Per_Pixel());
	fprintf(stdout,"Pixel Size: %.3f x %.3f micrometers.\n",CCD_Setup_Get_Pixel_Width(),
		CCD_Setup_Get_Pixel_Height());
	fprintf(stdout,"Sensor Size: %d x %d pixels.\n",CCD_Setup_Get_Sensor_Width(),CCD_Setup_Get_Sensor_Height());
	fprintf(stdout,"Timestamp Clock Frequency: %lld Hz.\n",CCD_Setup_Get_Timestamp_Clock_Frequency());
	fprintf(stdout,"Image Size: %d bytes.\n",CCD_Setup_Get_Image_Size_Bytes());
	/* do shutdown */
	if(!CCD_Setup_Shutdown())
	{
		CCD_General_Error();
		return 5;
	}
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Log_Level
 * @see #Help
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-help")==0))
		{
			Help();
			return FALSE;
		}
		else if((strcmp(argv[i],"-l")==0)||(strcmp(argv[i],"-log_level")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Log_Level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse log level %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-log_level requires a number 0..5.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}/* end for */
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Test Setup Startup:Help.\n");
	fprintf(stdout,"This program calls the Moptop CCD library's startup routine.\n");
	fprintf(stdout,"test_setup_startup  [-help][-l[og_level <0..5>].\n");
}
/*
** $Log$
*/
