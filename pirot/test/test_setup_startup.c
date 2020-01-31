/* test_setup_startup.c
** $Header$
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "pirot_command.h"
#include "pirot_general.h"
#include "pirot_setup.h"
#include "pirot_usb.h"

/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH        (256)
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * The USB device to connect to.
 * @see #STRING_LENGTH
 */
static char Device_Name[STRING_LENGTH];

static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
**          External functions 
** ------------------------------------------------------------------ */

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @see #Parse_Arguments
 * @see #Device_Name
 * @see #Log_Level
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Filter_Level
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Filter_Function
 * @see ../cdocs/pirot_general.html#PIROT_Log_Filter_Level_Absolute
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Handler_Function
 * @see ../cdocs/pirot_general.html#PIROT_Log_Handler_Stdout
 * @see ../cdocs/pirot_general.html#PIROT_Log
 * @see ../cdocs/pirot_general.html#PIROT_General_Error
 * @see ../cdocs/pirot_setup.html#PIROT_Setup_Rotator
 * @see ../cdocs/pirot_usb.html#PIROT_USB_Open
 * @see ../cdocs/pirot_usb.html#PIROT_USB_BAUD_RATE
 * @see ../cdocs/pirot_usb.html#PIROT_USB_Close
 */
int main(int argc, char *argv[])
{
	double position;

	/* parse arguments */
	fprintf(stdout,"test_setup_startup : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	PIROT_Set_Log_Filter_Level(Log_Level);
	PIROT_Set_Log_Filter_Function(PIROT_Log_Filter_Level_Absolute);
	PIROT_Set_Log_Handler_Function(PIROT_Log_Handler_Stdout);
        /* open device */
	PIROT_Log(LOG_VERBOSITY_TERSE,"test_setup_startup : Connecting to controller.");
	if(!PIROT_USB_Open(Device_Name,PIROT_USB_BAUD_RATE))
	{
		PIROT_General_Error();
		return 2;
	}
	/* call the setup rotator routine */
	if(!PIROT_Setup_Rotator())
	{
		PIROT_General_Error();
		return 3;
	}
	/* close the device */
	fprintf(stdout,"test_setup_startup:Closing connection.\n");
	PIROT_USB_Close();
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see ../cdocs/pirot_general.html#PIROT_General_Error
 * @see ../cdocs/pirot_setup.html#PIROT_Setup_Rotator_Run_Velocity
 * @see ../cdocs/pirot_setup.html#PIROT_Setup_Trigger_Step_Angle
 * @see #Device_Name
 * @see #Log_Level
 * @see #Position
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;
	double run_velocity,trigger_step_angle;

	for(i=1;i<argc;i++)
	{
	        if((strcmp(argv[i],"-a")==0)||(strcmp(argv[i],"-trigger_step_angle")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%lf",&trigger_step_angle);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse trigger step angle %s.\n",argv[i+1]);
					return FALSE;
				}
				if(!PIROT_Setup_Trigger_Step_Angle(trigger_step_angle))
				{
					PIROT_General_Error();
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-run_velocity requires a velocity in degrees/s (0..360.0).\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-d")==0)||(strcmp(argv[i],"-device_name")==0))
		{
			if((i+1)<argc)
			{
			  strncpy(Device_Name,argv[i+1],STRING_LENGTH-1);
				Device_Name[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:device_name requires a USB device.\n");
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
		else if((strcmp(argv[i],"-v")==0)||(strcmp(argv[i],"-run_velocity")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%lf",&run_velocity);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse run velocity %s.\n",argv[i+1]);
					return FALSE;
				}
				if(!PIROT_Setup_Rotator_Run_Velocity(run_velocity))
				{
					PIROT_General_Error();
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-run_velocity requires a velocity in degrees/s (0..360.0).\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Test Setup Startup:Help.\n");
	fprintf(stdout,"This program tries to configure and initialise the Physik Instrumente rotator.\n");
	fprintf(stdout,"test_setup_startup -d[evice_name] <USB device> [-run_velocity|-v <n.nn>] [-trigger_step_angle|-a <n.nn>] [-help]\n");
	fprintf(stdout,"\t[-l[og_level <0..5>].\n");
	fprintf(stdout,"The run velocity is in degrees/s, 0..360 deg/s, with a default of 45 deg/s.\n");
	fprintf(stdout,"The trigger step angle is in degrees, with a default of 22.5 (16 triggers per rotation). Other sensible values could be 11.25 (32 triggers per rotation) or 45.0 (8 triggers per revolution.\n");
}
/*
** $Log$
*/
