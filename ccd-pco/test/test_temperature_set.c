/* test_temperature_set.c
** $Header$
*/
/**
 * Test the CCD library temperature control.
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
/* hash defines */
/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH        (256)

/* variables */
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * Which camera head to connect to. Defaults to 0.
 */
static int Camera_Board = 0;
/**
 * A boolean, if TRUE get the current head temperature.
 */
static int Get_Temperature = FALSE;
/**
 * A boolean, if TRUE set the camera head's temperature set-point.
 */
static int Set_Temperature = FALSE;
/**
 * The temperature set-pint to use, in degrees centigrade, if Set_Temperature is TRUE.
 */
static int Temperature_Setpoint = 0;

/* functions */
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
 * <li>We initialise the command library by calling CCD_Command_Initialise, and open a connection to the specified
 *     camera board by calling CCD_Command_Open with the specified Camera_Board as argument.
 * <li>If Get_Temperature is set, we call CCD_Command_Get_Temperature and CCD_Command_Get_Cooling_Setpoint_Temperature
 *     and print the results.
 * <li>If Set_Temperature is set, we call CCD_Command_Get_Cooling_Setpoint_Temperature, 
 *     CCD_Command_Description_Get_Default_Cooling_Setpoint , CCD_Command_Description_Get_Min_Cooling_Setpoint,
 *     CCD_Command_Description_Get_Max_Cooling_Setpoint, and print the results. We call 
 *     CCD_Command_Set_Cooling_Setpoint_Temperature with Temperature_Setpoint as an argument to set a new tmperature
 *     set-point, and then call CCD_Command_Get_Cooling_Setpoint_Temperature to see if we can read it back.
 * <li>We call CCD_Command_Close and CCD_Command_Finalise to close the open connection to the camera head.
 * <li>
 * </ul>
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @see #Parse_Arguments
 * @see #Log_Level
 * @see #Camera_Board
 * @see #Get_Temperature
 * @see #Set_Temperature
 * @see #Temperature_Setpoint
 * @see ../cdocs/ccd_command.html#CCD_Command_Initialise
 * @see ../cdocs/ccd_command.html#CCD_Command_Open
 * @see ../cdocs/ccd_command.html#CCD_Command_Set_Cooling_Setpoint_Temperature
 * @see ../cdocs/ccd_command.html#CCD_Command_Description_Get_Default_Cooling_Setpoint
 * @see ../cdocs/ccd_command.html#CCD_Command_Description_Get_Min_Cooling_Setpoint
 * @see ../cdocs/ccd_command.html#CCD_Command_Description_Get_Max_Cooling_Setpoint
 * @see ../cdocs/ccd_command.html#CCD_Command_Get_Cooling_Setpoint_Temperature
 * @see ../cdocs/ccd_command.html#CCD_Command_Get_Temperature
 * @see ../cdocs/ccd_command.html#CCD_Command_Close
 * @see ../cdocs/ccd_command.html#CCD_Command_Finalise
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Level
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Filter_Level_Absolute
 * @see ../cdocs/ccd_general.html#CCD_General_Set_Log_Handler_Function
 * @see ../cdocs/ccd_general.html#CCD_General_Error
 * @see ../cdocs/ccd_general.html#CCD_General_Log_Handler_Stdout
 */
int main(int argc, char *argv[])
{
	int valid_sensor_temp,camera_temp,valid_psu_temp,psu_temp;
	int min_setpoint,max_setpoint,default_setpoint,current_setpoint;
	double sensor_temp;

	/* parse arguments */
	fprintf(stdout,"test_temperature : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_General_Set_Log_Filter_Level(Log_Level);
	CCD_General_Set_Log_Filter_Function(CCD_General_Log_Filter_Level_Absolute);
	CCD_General_Set_Log_Handler_Function(CCD_General_Log_Handler_Stdout);
	/* initialise ccd library */
	if(!CCD_Command_Initialise())
	{
		CCD_General_Error();
		return 2;
	}
	/* Open conenction to camera */
	if(!CCD_Command_Open(Camera_Board))
	{
		CCD_General_Error();
		return 3;
	}
	if(Get_Temperature)
	{
		if(!CCD_Command_Get_Temperature(&valid_sensor_temp,&sensor_temp,&camera_temp,
						&valid_psu_temp,&psu_temp))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 4;
		}
		if(valid_sensor_temp)
			fprintf(stdout,"Sensor temperature = %.3f C.\n",sensor_temp);
		else
			fprintf(stdout,"Sensor temperature was NOT valid.\n");
		fprintf(stdout,"Camera temperature = %d C.\n",camera_temp);
		if(valid_psu_temp)
			fprintf(stdout,"PSU temperature = %d C.\n",psu_temp);
		else
			fprintf(stdout,"PSU temperature was NOT valid.\n");
		if(!CCD_Command_Get_Cooling_Setpoint_Temperature(&current_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 5;
		}
		fprintf(stdout,"Current temperature setpoint = %d C.\n",current_setpoint);
	}/* end if Get_Temperature */	
	if(Set_Temperature)
	{
		if(!CCD_Command_Get_Cooling_Setpoint_Temperature(&current_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 6;
		}
		fprintf(stdout,"Current temperature setpoint = %d C.\n",current_setpoint);
		if(!CCD_Command_Description_Get_Default_Cooling_Setpoint(&default_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 7;
		}
		if(!CCD_Command_Description_Get_Min_Cooling_Setpoint(&min_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 8;
		}
		if(!CCD_Command_Description_Get_Max_Cooling_Setpoint(&max_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 9;
		}
		fprintf(stdout,"Default setpoint = %d C. Min/Max %d/%d C\n",default_setpoint,min_setpoint,max_setpoint);
		fprintf(stdout,"Setting new temperature setpoint to %d C.\n",Temperature_Setpoint);
		if(!CCD_Command_Set_Cooling_Setpoint_Temperature(Temperature_Setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 10;
		}
		if(!CCD_Command_Get_Cooling_Setpoint_Temperature(&current_setpoint))
		{
			CCD_General_Error();
			CCD_Command_Close();
			return 6;
		}
		fprintf(stdout,"New Current temperature setpoint = %d C.\n",current_setpoint);
	}/* end if set_temperature */
	/* print out current sensor temperature */
	/* do shutdown */
	if(!CCD_Command_Close())
	{
		CCD_General_Error();
		return 6;
	}
	if(!CCD_Command_Finalise())
	{
		CCD_General_Error();
		return 7;
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
 * @see #Get_Temperature
 * @see #Set_Temperature
 * @see #Camera_Board
 * @see #Log_Level
 * @see #Help
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	Set_Temperature = FALSE;
	Get_Temperature = FALSE;
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
		else if((strcmp(argv[i],"-get")==0))
		{
			Get_Temperature = TRUE;
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
		else if((strcmp(argv[i],"-s")==0)||(strcmp(argv[i],"-set")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Temperature_Setpoint);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse temperature set-point %s.\n",argv[i+1]);
					return FALSE;
				}
				Set_Temperature = TRUE;
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-set requires a temperature set-point.\n");
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
	fprintf(stdout,"Test Temperature Control:Help.\n");
	fprintf(stdout,"This program tests setting and getting the camer asensor temperature.\n");
	fprintf(stdout,"test_temperature_set  [-b[oard] <camera board>][-help][-l[og_level <0..5>].\n");
	fprintf(stdout,"\t[-g[et]][-s[et] <temperature>.\n");
	fprintf(stdout,"\t-board selects which camera head to talk to - the default is camera 0.\n");
	fprintf(stdout,"\t-get retrieves the current temperature.\n");
	fprintf(stdout,"\t-set sets the camera head temperature set-point, in degrees centigrade.\n");
}

