/* test_setup_startup.c
** $Header$
*/
/**
 * Test the CCD library startup routine.
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
/**
 * Which camera head to connect to. Defaults to 0.
 */
static int Camera_Board = 0;

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
 * <li>We set which camera to use by calling CCD_Setup_Set_Board.
 * <li>We initialise the library, open a connection to the camera, and perform initial configuration using 
 *     CCD_Setup_Startup.
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
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Set_Board
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Startup
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Shutdown
 */
int main(int argc, char *argv[])
{
	int serial_number;

	/* parse arguments */
	fprintf(stdout,"test_setup_startup : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_General_Set_Log_Filter_Level(Log_Level);
	CCD_General_Set_Log_Filter_Function(CCD_General_Log_Filter_Level_Absolute);
	CCD_General_Set_Log_Handler_Function(CCD_General_Log_Handler_Stdout);
	/* set camera */
	if(!CCD_Setup_Set_Board(Camera_Board))
	{
		CCD_General_Error();
		return 2;
	}		
	/* do startup */
	if(!CCD_Setup_Startup())
	{
		CCD_General_Error();
		return 2;
	}
	/* print out some retrieved data */
	if(!CCD_Setup_Get_Serial_Number(&serial_number))
	{
		CCD_General_Error();
		return 3;
	}
	fprintf(stdout,"Camera Serial Number: %d.\n",serial_number);
	fprintf(stdout,"Bytes Per Pixel: %d.\n",CCD_Setup_Get_Bytes_Per_Pixel());
	fprintf(stdout,"Sensor Size: %d x %d pixels.\n",CCD_Setup_Get_Sensor_Width(),CCD_Setup_Get_Sensor_Height());
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
 * @see #Camera_Board
 * @see #Log_Level
 * @see #Help
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-b")==0)||(strcmp(argv[i],"-board")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Camera_Board);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse camera board %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-board requires a camera board number.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0))
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
	fprintf(stdout,"test_setup_startup [-b[oard] <camera board>][-help][-l[og_level <0..5>].\n");
	fprintf(stdout,"\t-board selects which camera head to talk to - the default is camera 0.\n");
}
/*
** $Log$
*/
