/* moptop_main.c */
/**
 * Moptop C server main program. This controls the CCD cameras. On one
 * of the two control computers, an instance of this server will also control the filter_wheel and rotator.
 * @author $Author$
 */
#include <signal.h> /* signal handling */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log_udp.h"

#include "command_server.h"

#include "ccd_buffer.h"
#include "ccd_fits_filename.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

#include "filter_wheel_command.h"
#include "filter_wheel_general.h"

#include "pirot_general.h"
#include "pirot_setup.h"
#include "pirot_usb.h"

#include "moptop_config.h"
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
static int Moptop_Initialise_Mechanisms(void);
static void Moptop_Shutdown_Mechanisms(void);
static int Moptop_Startup_CCD(void);
static int Moptop_Shutdown_CCD(void);
static int Moptop_Startup_Rotator(void);
static int Moptop_Shutdown_Rotator(void);
static int Moptop_Startup_Filter_Wheel(void);
static int Moptop_Shutdown_Filter_Wheel(void);
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
** External functions 
** ------------------------------------------------------------------ */
/**
 * Main program.
 * <ul>
 * <li>We parse the command line arguments using Parse_Arguments.
 * <li>We setup signal handling (so the server doesn't crash when a client does) using Moptop_Initialise_Signal.
 * <li>We load the configuration file using Moptop_Config_Load, using the config file returned by 
 *     Moptop_General_Get_Config_Filename.
 * <li>We initialise the logging using Moptop_Initialise_Logging.
 * <li>We initialise the mechanisms (CCD/rotator, filter wheel) using Moptop_Initialise_Mechanisms.
 * <li>We intialise the server using Moptop_Server_Initialise.
 * <li>We start the server to handle incoming commands with Moptop_Server_Start. This routine finishes
 *     when the server/progam is told to terminate.
 * <li>We shutdown the connection to the mechanisms using Moptop_Shutdown_Mechanisms.
 * </ul>
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 * @see #Parse_Arguments
 * @see #Moptop_Initialise_Signal
 * @see #Moptop_Config_Load
 * @see #Moptop_Initialise_Logging
 * @see #Moptop_Initialise_Mechanisms
 * @see #Moptop_Server_Initialise
 * @see #Moptop_Server_Start
 * @see #Moptop_Shutdown_Mechanisms
 * @see moptop_general.html#Moptop_General_Get_Config_Filename
 * @see moptop_general.html#Moptop_General_Error
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
	/* initialise mechanisms */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Moptop_Initialise_Mechanisms.");
#endif
	retval = Moptop_Initialise_Mechanisms();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 3;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Server_Initialise.");
#endif
	retval = Moptop_Server_Initialise();
	if(retval == FALSE)
	{
		Moptop_General_Error("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		/* shutdown mechanisms */
		Moptop_Shutdown_Mechanisms();
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
		/* shutdown mechanisms */
		Moptop_Shutdown_Mechanisms();
		return 4;
	}
	/* shutdown */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Moptop_Shutdown_Mechanisms");
#endif
	Moptop_Shutdown_Mechanisms();
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
 * @return The routine returns TRUE on success and FALSE on failure. Moptop_General_Error_Number / 
 *         Moptop_General_Error_String are set on failure.
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log_Set_Directory
 * @see moptop_general.html#Moptop_General_Log_Set_Root
 * @see moptop_general.html#Moptop_General_Log_Set_Error_Root
 * @see moptop_general.html#Moptop_General_Log_Set_UDP
 * @see moptop_general.html#Moptop_General_Add_Log_Handler_Function
 * @see moptop_general.html#Moptop_General_Log_Handler_Log_Hourly_File
 * @see moptop_general.html#Moptop_General_Log_Handler_Log_UDP
 * @see moptop_general.html#Moptop_General_Call_Log_Handlers
 * @see moptop_general.html#Moptop_General_Call_Log_Handlers_CCD
 * @see moptop_general.html#Moptop_General_Call_Log_Handlers_Filter_Wheel
 * @see moptop_general.html#Moptop_General_Call_Log_Handlers_Rotator
 * @see moptop_general.html#Moptop_General_Set_Log_Filter_Function
 * @see moptop_general.html#Moptop_General_Log_Filter_Level_Absolute
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_Integer
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Set_Log_Handler_Function
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Set_Log_Filter_Function
 * @see ../ccd/cdocs/ccd_general.html#CCD_General_Log_Filter_Level_Absolute
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Handler_Function
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Function
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Log_Filter_Level_Absolute
 * @see ../pirot/cdocs/pirot_general.html#PIROT_Set_Log_Handler_Function
 * @see ../pirot/cdocs/pirot_general.html#PIROT_Set_Log_Filter_Function
 * @see ../pirot/cdocs/pirot_general.html#PIROT_Log_Filter_Level_Absolute
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
	Filter_Wheel_General_Set_Log_Handler_Function(Moptop_General_Call_Log_Handlers_Filter_Wheel);
	Filter_Wheel_General_Set_Log_Filter_Function(Filter_Wheel_General_Log_Filter_Level_Absolute);
	/* rotator */
	PIROT_Set_Log_Handler_Function(Moptop_General_Call_Log_Handlers_Rotator);
	PIROT_Set_Log_Filter_Function(PIROT_Log_Filter_Level_Absolute);
	/* setup command server logging */
	Command_Server_Set_Log_Handler_Function(Moptop_General_Call_Log_Handlers);
	Command_Server_Set_Log_Filter_Function(Command_Server_Log_Filter_Level_Absolute);
	return TRUE;
}

/**
 * Initialise the moptop mechanisms. Calls Moptop_Startup_CCD,Moptop_Startup_Rotator,Moptop_Startup_Filter_Wheel.
 * @return The routine returns TRUE on success and FALSE on failure. Moptop_General_Error_Number / 
 *         Moptop_General_Error_String are set on failure.
 * @see #Moptop_Startup_CCD
 * @see #Moptop_Startup_Rotator
 * @see #Moptop_Startup_Filter_Wheel
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 */
static int Moptop_Initialise_Mechanisms(void)
{
	int retval;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* initialise connection to the CCD */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Startup_CCD.");
#endif
	retval = Moptop_Startup_CCD();
	if(retval == FALSE)
	{
		return FALSE;
	}
	/* initialise connection to the PI rotator */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Startup_Rotator.");
#endif
	retval = Moptop_Startup_Rotator();
	if(retval == FALSE)
	{
		return FALSE;
	}
	/* initialise connection to the filter wheel */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Startup_Filter_Wheel.");
#endif
	retval = Moptop_Startup_Filter_Wheel();
	if(retval == FALSE)
	{
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown the moptop mechanisms. Calls Moptop_Shutdown_CCD,Moptop_Shutdown_Rotator,Moptop_Shutdown_Filter_Wheel.
 * @see #Moptop_Shutdown_CCD
 * @see #Moptop_Shutdown_Rotator
 * @see #Moptop_Shutdown_Filter_Wheel
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 */
static void Moptop_Shutdown_Mechanisms(void)
{
	int retval;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* shutdown ccd */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Shutdown_CCD.");
#endif
	retval = Moptop_Shutdown_CCD();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Moptop_General_Error("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
	}
	/* shutdown rotator */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Shutdown_Rotator.");
#endif
	retval = Moptop_Shutdown_Rotator();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Moptop_General_Error("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
	}
	/* shutdown filter wheel */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Moptop_Shutdown_Filter_Wheel.");
#endif
	retval = Moptop_Shutdown_Filter_Wheel();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Moptop_General_Error("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
}


/**
 * If the CCD is enabled for initialisation, initialise the CCD.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "ccd.enable" to see whether the CCD is enabled for initialisation.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Call CCD_Setup_Startup to initialise the CCD.
 * <li>Call CCD_Buffer_Initialise to allocate space for the image buffers.
 * <li>We call Moptop_Config_Get_Character to get the instrument code for this instance of the C layer/CCD
 *     with property keyword: "file.fits.instrument_code".
 * <li>We call Moptop_Config_Get_String to get the data directory to store generated FITS images in using the
 *     property keyword: "file.fits.path".
 * <li>We call CCD_Fits_Filename_Initialise to initialise FITS filename data and find the current MULTRUN number.
 * <li>We call CCD_Fits_Header_Initialise to initialise FITS header data.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_Character
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Initialise
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Initialise
 * @see ../ccd/cdocs/ccd_fits_header.html#CCD_Fits_Header_Initialise
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Startup
 */
static int Moptop_Startup_CCD(void)
{
	int enabled;
	char instrument_code;
	char* data_dir = NULL;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_CCD",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* do we want to initialise the CCD.
	** The C layer always has a CCD attached, but if it is unplugged/broken, setting "ccd.enable" to FALSE
	** allows the C layer to initialise to enable control of other mechanisms from the C layer. */
	if(!Moptop_Config_Get_Boolean("ccd.enable",&enabled))
	{
		Moptop_General_Error_Number = 1;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_CCD:Failed to get whether CCD is enabled for initialisation.");
		return FALSE;
	}
	/* if we don't want to initialise the CCD, just return here. */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Startup_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (rotator NOT enabled).");
#endif
		return TRUE;
	}
	/* actually do initialisation of the CCD/Andor library */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling CCD_Setup_Startup.");
#endif
	if(!CCD_Setup_Startup())
	{
		Moptop_General_Error_Number = 2;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_CCD:CCD_Setup_Startup failed.");
		return FALSE;
	}
	/* allocate space for image buffers */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling CCD_Buffer_Initialise.");
#endif
	if(!CCD_Buffer_Initialise())
	{
		Moptop_General_Error_Number = 3;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_CCD:CCD_Buffer_Initialise failed.");
		return FALSE;
	}
	/* fits filename initialisation */
	if(!Moptop_Config_Get_Character("file.fits.instrument_code",&instrument_code))
		return FALSE;
	if(!Moptop_Config_Get_String("file.fits.path",&data_dir))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling CCD_Fits_Filename_Initialise(%c,%s).",instrument_code,data_dir);
#endif
	if(!CCD_Fits_Filename_Initialise(instrument_code,data_dir))
	{
		Moptop_General_Error_Number = 4;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_CCD:CCD_Fits_Filename_Initialise failed.");
		if(data_dir != NULL)
			free(data_dir);
		return FALSE;
	}
	/* free allocated data */
	if(data_dir != NULL)
		free(data_dir);
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling CCD_Fits_Header_Initialise.");
#endif
	if(!CCD_Fits_Header_Initialise())
	{
		Moptop_General_Error_Number = 5;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_CCD:CCD_Fits_Header_Initialise failed.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_CCD",LOG_VERBOSITY_TERSE,"STARTUP","Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown the CCD connection.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "ccd.enable" to see whether the CCD is enabled for initialisation/finislisation.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Call CCD_Setup_Shutdown to shutdown the connection to the CCD.
 * <li>Call CCD_Buffer_Free to free the image buffer memory.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Free
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Shutdown
 */
static int Moptop_Shutdown_CCD(void)
{
	int enabled;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* is the ccd active/enabled for this instance of the C layer */
	if(!Moptop_Config_Get_Boolean("ccd.enable",&enabled))
	{
		Moptop_General_Error_Number = 6;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_CCD:Failed to get whether ccd initialisation is enabled.");
		return FALSE;
	}
	/* if the rotator is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (CCD NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling CCD_Setup_Shutdown.");
#endif
	if(!CCD_Setup_Shutdown())
	{
		Moptop_General_Error_Number = 24;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_CCD:CCD_Setup_Shutdown failed.");
		return FALSE;
	}
	/* free image buffers */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling CCD_Buffer_Free.");
#endif
	if(!CCD_Buffer_Free())
	{
		Moptop_General_Error_Number = 25;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_CCD:CCD_Buffer_Free failed.");
		return FALSE;
	}

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_CCD",LOG_VERBOSITY_TERSE,"STARTUP","Finished.");
#endif
	return TRUE;
}

/**
 * If the rotator is enabled, open a connection to the PI rotator.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "rotator.enable" to see whether the rotator is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Moptop_Config_Get_String to get the device name to use for the 
 *     filter wheel connection ("rotator.device_name").
 * <li>Call PIROT_USB_Open to connect to the PI rotator at PIROT_USB_BAUD_RATE.
 * <li>Call PIROT_Setup_Rotator to configure the rotator.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Rotator
 * @see ../pirot/cdocs/pirot_usb.html#PIROT_USB_Open
 * @see ../pirot/cdocs/pirot_usb.html#PIROT_USB_BAUD_RATE
 */
static int Moptop_Startup_Rotator(void)
{
	char *device_name = NULL;
	int enabled;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the rotator active/enabled for this instance of the C layer */
	if(!Moptop_Config_Get_Boolean("rotator.enable",&enabled))
	{
		Moptop_General_Error_Number = 12;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Rotator:"
			"Failed to get whether rotator is enabled.");
		return FALSE;
	}
	/* if the rotator is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (rotator NOT enabled).");
#endif
		return TRUE;
	}
	/* get device name */
	if(!Moptop_Config_Get_String("rotator.device_name",&device_name))
	{
		Moptop_General_Error_Number = 13;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Rotator:"
			"Failed to get rotator device_name.");
		return FALSE;
	}
        /* open device */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Startup_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Open a connection to the rotator using device '%s'.",device_name);
#endif
	if(!PIROT_USB_Open(device_name,PIROT_USB_BAUD_RATE))
	{
		Moptop_General_Error_Number = 14;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Rotator:"
			"PIROT_USB_Open(%s) failed.",device_name);
		/* free allocated data */
		if(device_name != NULL)
			free(device_name);
		return FALSE;
	}
	/* free allocated data */
	if(device_name != NULL)
		free(device_name);
	/* call the setup rotator routine */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Setup the rotator.");
#endif
	if(!PIROT_Setup_Rotator())
	{
		Moptop_General_Error_Number = 15;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Rotator:PIROT_Setup_Rotator failed.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown a previously opened connection to the rotator.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "rotator.enable" to see whether the rotator is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use PIROT_USB_Close to close the connection to the rotator.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 * @see ../pirot/cdocs/pirot_usb.html#PIROT_USB_Close
 */
static int Moptop_Shutdown_Rotator(void)
{
	int enabled;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* is the rotator active/enabled for this instance of the C layer */
	if(!Moptop_Config_Get_Boolean("rotator.enable",&enabled))
	{
		Moptop_General_Error_Number = 16;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_Rotator:"
			"Failed to get whether rotator is enabled.");
		return FALSE;
	}
	/* if the rotator is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (rotator NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling PIROT_USB_Close.");
#endif
	if(!PIROT_USB_Close())
	{
		Moptop_General_Error_Number = 18;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_Rotator:PIROT_USB_Close failed.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * If the filter wheel is enabled, open a connection to the filter wheel.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "filter_wheel.enable" to see whether the filter wheel is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Moptop_Config_Get_String to get the device name to use for the 
 *     filter wheel connection ("filter_wheel.device_name").
 * <li>Use Filter_Wheel_Command_Open to open a connection to the filter wheel.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_config.html#Moptop_Config_Get_String
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Open
 */
static int Moptop_Startup_Filter_Wheel(void)
{
	char *device_name = NULL;
	int enabled;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the filter wheel active/enabled for this instance of the C layer */
	if(!Moptop_Config_Get_Boolean("filter_wheel.enable",&enabled))
	{
		Moptop_General_Error_Number = 7;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Filter_Wheel:"
			"Failed to get whether filter wheel is enabled.");
		return FALSE;
	}
	/* if the filter wheel is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (filter wheel NOT enabled).");
#endif
		return TRUE;
	}
	/* get device name */
	if(!Moptop_Config_Get_String("filter_wheel.device_name",&device_name))
	{
		Moptop_General_Error_Number = 8;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Filter_Wheel:"
			"Failed to get filter wheel device_name.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Opening connection to filter wheel using device '%s'.",device_name);
#endif
	/* open connection to the filter wheel */
	if(!Filter_Wheel_Command_Open(device_name))
	{
		Moptop_General_Error_Number = 9;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Filter_Wheel:"
			"Filter_Wheel_Command_Open(%s) failed.",device_name);
		/* free allocated data */
		if(device_name != NULL)
			free(device_name);
		return FALSE;
	}
	/* free allocated data */
	if(device_name != NULL)
		free(device_name);
	/* fault 2668. The filter wheel is failing and on startup ends up in position 8 which is impossible.
	** Try driving the filter wheel to a known position after opening a connection to it. */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Moving filter wheel to a known position(1).");
#endif
	if(!Filter_Wheel_Command_Move(1))
	{
		Moptop_General_Error_Number = 26;
		sprintf(Moptop_General_Error_String,"Moptop_Startup_Filter_Wheel:Filter_Wheel_Command_Move(1) failed.");
		return FALSE;
	}		
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown a previously opened connection to the filter wheel.
 * <ul>
 * <li>Use Moptop_Config_Get_Boolean to get "filter_wheel.enable" to see whether the filter wheel is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Filter_Wheel_Command_Close to close the connection to the filter wheel.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Get_Boolean
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Log
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Close
 */
static int Moptop_Shutdown_Filter_Wheel(void)
{
	int enabled;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the filter wheel active/enabled for this instance of the C layer */
	if(!Moptop_Config_Get_Boolean("filter_wheel.enable",&enabled))
	{
		Moptop_General_Error_Number = 10;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_Filter_Wheel:"
			"Failed to get whether filter wheel is enabled.");
		return FALSE;
	}
	/* if the filter wheel is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (filter wheel NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("main","moptop_main.c","Moptop_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Filter_Wheel_Command_Close.");
#endif
	if(!Filter_Wheel_Command_Close())
	{
		Moptop_General_Error_Number = 11;
		sprintf(Moptop_General_Error_String,"Moptop_Shutdown_Filter_Wheel:Filter_Wheel_Command_Close failed.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("main","moptop_main.c","Moptop_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif

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
	fprintf(stdout,"\t[-filter_wheel_log_level|-fwll <level>\n");
	fprintf(stdout,"\t[-rotator_log_level|-rll <level>\n");
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
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Level
 * @see ../pirot/cdocs/pirot_general.html#PIROT_General_Set_Log_Filter_Level
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
		else if((strcmp(argv[i],"-filter_wheel_log_level")==0)||(strcmp(argv[i],"-fwll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Filter_Wheel_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-rotator_log_level")==0)||(strcmp(argv[i],"-rll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				PIROT_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
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

