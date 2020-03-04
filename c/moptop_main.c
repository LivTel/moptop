/* moptop_main.c
*/
/**
 * Moptop C server main program. This controls the CCD cameras. On one
 * of the two control computers, an instance of this server will also control the filter_wheel and rotator.
 * @author $Author$
 * @version $Revision$
 */
#include <signal.h> /* signal handling */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log_udp.h"

#include "command_server.h"

#include "ccd_fits_filename.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

#include "moptop_general.h"
#include "moptop_fits_header.h"
#include "moptop_server.h"

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id$";

/* internal routines */
static int Moptop_Initialise_Signal(void);
static int Moptop_Initialise_Logging(void);
static int Moptop_Startup_CCD(void);
static int Moptop_Shutdown_CCD(void);
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
** External functions 
** ------------------------------------------------------------------ */
/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 */
int main(int argc, char *argv[])
{
	int retval;

/* parse arguments */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP","Parsing Arguments.");
#endif
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* initialise signal handling */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Moptop_Initialise_Signal.");
#endif
	retval = Moptop_Initialise_Signal();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 4;
	}
	/* initialise/load configuration */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP","Moptop_Config_Load.");
#endif
	retval = Moptop_Config_Load(Moptop_General_Get_Config_Filename());
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 2;
	}
	/* set logging options */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Initialise_Logging.");
#endif
	retval = Moptop_Initialise_Logging();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 4;
	}
	/* initialise connection to the CCD */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Moptop_Startup_CCD.");
#endif
	retval = Moptop_Startup_CCD();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 3;
	}
	/* initialise command server */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Server_Initialise.");
#endif
	retval = Moptop_Server_Initialise();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		/* ensure CCD is warmed up */
		Moptop_Shutdown_CCD();
		return 4;
	}
	/* start server */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Server_Start.");
#endif
	retval = Moptop_Server_Start();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		/* ensure CCD is warmed up */
		Moptop_Shutdown_CCD();
		return 4;
	}
	/* shutdown */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Shutdown_CCD");
#endif
	retval = Moptop_Shutdown_CCD();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 2;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "moptop completed.");
#endif
	return 0;
}

/* -----------------------------------------------------------------------------
**      Internal routines
** ----------------------------------------------------------------------------- */
/**
 * Initialise signal handling. Switches off default "Broken pipe" error, so client
 * crashes do NOT kill the Moptop control system.
 * Don't use Logging here, this is called pre-logging.
 */
static int Moptop_Initialise_Signal(void)
{
	struct sigaction sig_action;

	/* old code
	signal(SIGPIPE, SIG_IGN);
	*/
	sig_action.sa_handler = SIG_IGN;
	sig_action.sa_flags = 0;
	sigemptyset(&sig_action.sa_mask);
	sigaction(SIGPIPE,&sig_action,NULL);
	return TRUE;
}

/**
 * Setup logging. Get directory name from config "logging.directory_name".
 * Get UDP logging config. Setup log handlers for Moptop software and subsystems.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log_Set_Directory
 * @see moptop_general.html#Moptop_General_Log_Set_Root
 * @see moptop_general.html#Moptop_General_Log_Set_Error_Root
 * @see moptop_general.html#Moptop_General_Log_Set_UDP
 * @see moptop_general.html#Moptop_General_Add_Log_Handler_Function
 * @see moptop_general.html#Moptop_General_Log_Handler_Log_Hourly_File
 * @see moptop_general.html#Moptop_General_Log_Handler_Log_UDP
 * @see moptop_general.html#Moptop_General_Call_Log_Handlers
 * @see moptop_general.html#Moptop_General_Set_Log_Filter_Function
 * @see moptop_general.html#Moptop_General_Log_Filter_Level_Absolute
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_Integer
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Set_Log_Handler_Function
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Function
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Log_Filter_Level_Absolute
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Handler_Function
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Filter_Function
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Log_Filter_Level_Absolute
 */
static int Moptop_Initialise_Logging(void)
{
	char *log_directory = NULL;
	char *filename_root = NULL;
	char *hostname = NULL;
	int retval,port_number,active;

	/* don't log yet - not fully setup yet */
	/* log directory */
	if(!Moptop_Config_Get_String("logging.directory_name",&log_directory))
	{
		Moptop_General_Error_Number = 17;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get logging directory.");
		return FALSE;
	}
	if(!Moptop_General_Log_Set_Directory(log_directory))
	{
		if(log_directory != NULL)
			free(log_directory);
		return FALSE;
	}
	if(log_directory != NULL)
		free(log_directory);
	/* log filename root */
	if(!Moptop_Config_Get_String("logging.root.log",&filename_root))
	{
		Moptop_General_Error_Number = 19;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get log root filename.");
		return FALSE;
	}
	if(!Moptop_General_Log_Set_Root(filename_root))
	{
		if(filename_root != NULL)
			free(filename_root);
		return FALSE;
	}
	if(filename_root != NULL)
		free(filename_root);
	/* error filename root */
	if(!Moptop_Config_Get_String("logging.root.error",&filename_root))
	{
		Moptop_General_Error_Number = 23;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get error root filename.");
		return FALSE;
	}
	if(!Moptop_General_Log_Set_Error_Root(filename_root))
	{
		if(filename_root != NULL)
			free(filename_root);
		return FALSE;
	}
	if(filename_root != NULL)
		free(filename_root);
	/* setup log_udp */
	if(!Moptop_Config_Get_Boolean("logging.udp.active",&active))
	{
		Moptop_General_Error_Number = 20;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get log_udp active.");
		return FALSE;
	}
	if(!Moptop_Config_Get_Integer("logging.udp.port_number",&port_number))
	{
		Moptop_General_Error_Number = 21;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get log_udp port_number.");
		return FALSE;
	}
	if(!Moptop_Config_Get_String("logging.udp.hostname",&hostname))
	{
		Moptop_General_Error_Number = 22;
		sprintf(Moptop_General_Error_String,"Moptop_Initialise_Logging:"
			"Failed to get log_udp hostname.");
		return FALSE;
	}
	if(!Moptop_General_Log_Set_UDP(active,hostname,port_number))
	{
		if(hostname != NULL)
			free(hostname);
		return FALSE;
	}
	if(hostname != NULL)
		free(hostname);
	/* Moptop */
	Moptop_General_Add_Log_Handler_Function(Moptop_General_Log_Handler_Log_Hourly_File);
	Moptop_General_Add_Log_Handler_Function(Moptop_General_Log_Handler_Log_UDP);
	Moptop_General_Set_Log_Filter_Function(Moptop_General_Log_Filter_Level_Absolute);
	/* CCD */
	CCD_General_Set_Log_Handler_Function(Moptop_General_Call_Log_Handlers_CCD);
	CCD_General_Set_Log_Filter_Function(CCD_General_Log_Filter_Level_Absolute);
	/* filter wheel */
	/* diddly */
	/* rotator */
	/* diddly */

	/* setup command server logging */
	Command_Server_Set_Log_Handler_Function(Moptop_General_Call_Log_Handlers);
	Command_Server_Set_Log_Filter_Function(Command_Server_Log_Filter_Level_Absolute);
	return TRUE;
}

/**
 * Initialise the CCD connection, initialise the CCD and set the temperature.
 * <ul>
 * <li>
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_Integer
 * @see moptop_config.html#Moptop_Config_Get_Double
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_Initialise
  */
static int Moptop_Startup_CCD(void)
{
	return TRUE;
}

/**
 * Shutdown the CCD conenction.
 * <ul>
 * <li>
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 */
static int Moptop_Shutdown_CCD(void)
{
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Moptop:Help.\n");
	fprintf(stdout,"moptop [-co[nfig_filename] <filename>]\n");
	fprintf(stdout,"\t[-moptop_log_level|-ll <level>\n");
	fprintf(stdout,"\t[-ccd_log_level|-ccdll <level>\n");
	fprintf(stdout,"\t[-command_server_log_level|-csll <level>\n");
	fprintf(stdout,"\t<level> is an integer from 1..5.\n");
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see moptop_general.html#Moptop_General_Set_Config_Filename
 * @see moptop_general.html#Moptop_General_Set_Log_Filter_Level
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Level
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Filter_Level
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,log_level;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-moptop_log_level")==0)||(strcmp(argv[i],"-ll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Moptop_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-ccd_log_level")==0)||(strcmp(argv[i],"-ccdll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				CCD_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-command_server_log_level")==0)||(strcmp(argv[i],"-csll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Command_Server_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-config_filename")==0)||(strcmp(argv[i],"-co")==0))
		{
			if((i+1)<argc)
			{
				if(!Moptop_General_Set_Config_Filename(argv[i+1]))
				{
					fprintf(stderr,"Parse_Arguments:"
						"Moptop_General_Set_Config_Filename failed.\n");
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:config filename required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}

