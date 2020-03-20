/* test_temperature_control.c
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
#include <unistd.h>
#include "log_udp.h"
#include "ccd_command.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH        (256)
/**
 * How many times round the loop to wait for the temeprature to stabilise, before timing out.
 */
#define STABLE_TIMEOUT_COUNT (180)
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * Should the program loop until the temperature is stable? Initialised to FALSE.
 */
static int Wait_Until_Stable = FALSE;


static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);
static int Test_Temperature_Control(void);

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
 * <li>We call Test_Temperature_Control to print out the TemperatureControl enumeration values.
 * <li>We call CCD_Temperature_Get to print out the current sensor temperature.
 * <li>We call CCD_Temperature_Get_Temperature_Status_String to print out the current sensor temperature status string.
 * <li>If Wait_Until_Stable is TRUE:
 *     <ul>
 *     <li>We enter a loop until CCD_Temperature_Is_Stabilised returns TRUE, or we time out 
 *         (after STABLE_TIMEOUT_COUNT round the loop).
 *     <li>We call CCD_Temperature_Get to print out the current sensor temperature.
 *     <li>We call CCD_Temperature_Get_Temperature_Status_String to print out the 
 *         current sensor temperature status string.
 *     <li>We sleep for a second.
 *     </ul>
 * <li>We shutdown the connection to the library using CCD_Setup_Shutdown.
 * <li>
 * </ul>
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @see #STABLE_TIMEOUT_COUNT
 * @see #Test_Temperature_Control
 * @see #Parse_Arguments
 * @see #Log_Level
 * @see #Wait_Until_Stable
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Level
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Filter_Level_Absolute
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Handler_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Error
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Handler_Stdout
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Startup
 * @see ../cdocs/ccd_setup.html#CCD_Setup_Shutdown
 * @see ../cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 * @see ../cdocs/ccd_temperature.html#CCD_Temperature_Is_Stabilised
 */
int main(int argc, char *argv[])
{
	char temperature_status_string[64];
	double current_temperature;
	int index;

	/* parse arguments */
	fprintf(stdout,"test_temperature_control : Parsing Arguments.\n");
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
	/* Print out the list of TemperatureControl enumeration values. */
	if(!Test_Temperature_Control())
	{
		CCD_Setup_Shutdown();
		return 3;
	}
	/* print out current sensor temperature */
	if(!CCD_Temperature_Get(&current_temperature))
	{
		CCD_General_Error();
		CCD_Setup_Shutdown();
		return 4;
	}
	fprintf(stdout,"Current temperature: %.3f C.\n",current_temperature);
	/* get the current status */
	if(!CCD_Temperature_Get_Temperature_Status_String(temperature_status_string,64))
	{
		CCD_General_Error();
		CCD_Setup_Shutdown();
		return 5;
	}
	fprintf(stdout,"Current temperature status: %s.\n",temperature_status_string);
	if(Wait_Until_Stable)
	{
		index = 0;
		while((!CCD_Temperature_Is_Stabilised()) && (index < STABLE_TIMEOUT_COUNT))
		{
			fprintf(stdout,"Temperature not stable after %d attempts.\n",index);
			/* print out current sensor temperature */
			if(!CCD_Temperature_Get(&current_temperature))
			{
				CCD_General_Error();
				CCD_Setup_Shutdown();
				return 6;
			}
			fprintf(stdout,"Current temperature: %.3f C.\n",current_temperature);
			/* get the current status */
			if(!CCD_Temperature_Get_Temperature_Status_String(temperature_status_string,64))
			{
				CCD_General_Error();
				CCD_Setup_Shutdown();
				return 7;
			}
			fprintf(stdout,"Current temperature status: %s.\n",temperature_status_string);
			sleep(1);
			index ++;
		}/* end while not stabilised */
		if(CCD_Temperature_Is_Stabilised())
		{
			fprintf(stdout,"Temperature now stable after %d loops.\n",index);
		}
		else 
		{
			fprintf(stdout,"Temperature not stable, timed out after %d loops.\n",index);
		}
	} /* end if wait until stable */
	/* do shutdown */
	if(!CCD_Setup_Shutdown())
	{
		CCD_General_Error();
		return 8;
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
 * @see #Wait_Until_Stable 
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
		else if((strcmp(argv[i],"-wait_until_stable")==0))
		{
			Wait_Until_Stable = TRUE;
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
	fprintf(stdout,"Test Temperature Control:Help.\n");
	fprintf(stdout,"This program tests control/status retrieval of the sensor temperature.\n");
	fprintf(stdout,"test_temperature_control [-help][-l[og_level <0..5>][-wait_until_stable].\n");
}

/**
 * Print out the list of TemperatureControl enumeration values.
 * @see ../cdocs/ccd_command.html#CCD_Command_Get_Temperature_Control_Count
 * @see ../cdocs/ccd_command.html#CCD_Command_Get_Enum_String_By_Index
 */
static int Test_Temperature_Control(void)
{
	char temperature_control_value_enum_string[64];
	int count,index;

	/* get the number of temperature control enumerations. */
	if(!CCD_Command_Get_Temperature_Control_Count(&count))
	{
		CCD_General_Error();
		return FALSE;
	}
	fprintf(stdout,"There are %d TemperatureControl enumeration values:.\n",count);
	for(index = 0; index < count; index ++)
	{
		if(!CCD_Command_Get_Enum_String_By_Index("TemperatureControl",index,
							 temperature_control_value_enum_string,64))
		{
			CCD_General_Error();
			return FALSE;
		}
		fprintf(stdout,"TemperatureControl enumeration value string %d  = %s.\n",index,
			temperature_control_value_enum_string);
	}/* end for on temperature control enumeration count */
	return TRUE;
}

/*
** $Log$
*/
