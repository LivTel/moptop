/* test_mov.c
** $Header$
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "pirot_command.h"
#include "pirot_general.h"
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
/**
 * The position to move the rotator to.
 */
static double Position;

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
 * @see #Position
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Filter_Level
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Filter_Function
 * @see ../cdocs/pirot_general.html#PIROT_Log_Filter_Level_Absolute
 * @see ../cdocs/pirot_general.html#PIROT_Set_Log_Handler_Function
 * @see ../cdocs/pirot_general.html#PIROT_Log_Handler_Stdout
 * @see ../cdocs/pirot_general.html#PIROT_Log
 * @see ../cdocs/pirot_general.html#PIROT_General_Error
 * @see ../cdocs/pirot_usb.html#PIROT_USB_Open
 * @see ../cdocs/pirot_usb.html#PIROT_USB_BAUD_RATE
 * @see ../cdocs/pirot_usb.html#PIROT_USB_Close
 * @see ../cdocs/pirot_command.html#PIROT_Command_MOV
 */
int main(int argc, char *argv[])
{
	double position;

	/* parse arguments */
	fprintf(stdout,"test_mov : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	PIROT_Set_Log_Filter_Level(Log_Level);
	PIROT_Set_Log_Filter_Function(PIROT_Log_Filter_Level_Absolute);
	PIROT_Set_Log_Handler_Function(PIROT_Log_Handler_Stdout);
        /* open device */
	PIROT_Log(LOG_VERBOSITY_TERSE,"test_mov : Connecting to controller.");
	if(!PIROT_USB_Open(Device_Name,PIROT_USB_BAUD_RATE))
	{
		PIROT_General_Error();
		return 2;
	}
	fprintf(stdout,"test_mov:Moving to position %.2f.\n",Position);
	if(!PIROT_Command_MOV(Position))
	{
		PIROT_General_Error();
		return 3;

	}
	fprintf(stdout,"test_mov:Closing connection.\n");
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
 * @see #Device_Name
 * @see #Log_Level
 * @see #Position
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
		else if((strcmp(argv[i],"-p")==0)||(strcmp(argv[i],"-position")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%lf",&Position);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse position %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-position requires a target position (0..36000.0).\n");
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
	fprintf(stdout,"Test Move:Help.\n");
	fprintf(stdout,"This program tries to move the Physik Instrumente rotator.\n");
	fprintf(stdout,"test_mov -d[evice_name] <USB device> -p[osition] <n> [-help]\n");
	fprintf(stdout,"\t[-l[og_level <0..5>].\n");
}
/*
** $Log$
*/
