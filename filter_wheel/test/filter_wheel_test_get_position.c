/* filter_wheel_test_get_position.c
** $Header$
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "filter_wheel_command.h"
#include "filter_wheel_general.h"

/**
 * Test program to try and get the current position of the Starlight Express filter wheel.
 */
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
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Set_Log_Filter_Level
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Set_Log_Filter_Function
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Log_Filter_Level_Absolute
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Set_Log_Handler_Function
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Log_Handler_Stdout
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_Log
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Error
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Open
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Close
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 */
int main(int argc, char *argv[])
{
	int current_position;

	/* parse arguments */
	fprintf(stdout,"test_filter_wheel_get_position : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Filter_Wheel_Set_Log_Filter_Level(Log_Level);
	Filter_Wheel_Set_Log_Filter_Function(Filter_Wheel_Log_Filter_Level_Absolute);
	Filter_Wheel_Set_Log_Handler_Function(Filter_Wheel_Log_Handler_Stdout);
        /* open device */
	Filter_Wheel_Log(LOG_VERBOSITY_TERSE,"test_filter_wheel_get_position : Connecting to controller.");
	if(!Filter_Wheel_Command_Open(Device_Name))
	{
		Filter_Wheel_General_Error();
		return 2;
	}
	fprintf(stdout,"test_filter_wheel_get_position:Get the current position of the filter wheel.\n");
	if(!Filter_Wheel_Command_Get_Position(&current_position))
	{
		Filter_Wheel_General_Error();
		return 3;

	}
	fprintf(stdout,"test_filter_wheel_get_position:The current position of the filter wheel is %d.\n",
		current_position);
	fprintf(stdout,"test_filter_wheel_get_position:Closing connection.\n");
	Filter_Wheel_Command_Close();
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Device_Name
 * @see #Log_Level
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-d")==0)||(strcmp(argv[i],"-device_name")==0))
		{
			if((i+1)<argc)
			{
				strncpy(Device_Name,argv[i+1],STRING_LENGTH-1);
				Device_Name[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:device_name requires a USB device name.\n");
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
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Test Filter Wheel Get Position:Help.\n");
	fprintf(stdout,"This program tries to get the current position of the filter wheel.\n");
	fprintf(stdout,"filter_wheel_test_get_position -d[evice_name] <USB device> [-help]\n");
	fprintf(stdout,"\t[-l[og_level <0..5>].\n");
}
/*
** $Log$
*/
