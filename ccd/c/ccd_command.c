/* ccd_command.c
** Moptop Andor Zyla CCD library
** $Header$
*/
/**
 * Command wrapper around the Andor SDK library.
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
#ifdef MBSTOWCS_MUTEXED
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <wchar.h>
#include <atcore.h>	  /* Andor camera access functions */
/* atutility.h contains Andor conversion utils.
** However the function prototypes only work with C++.
** Therefore the installed copy of atutility.h (in /usr/local/include) has to be modified to work with C.
** This is done by removing the '&' from the following function prototypes:
** - AT_GetWidthFromMetadata 
** - AT_GetHeightFromMetadata 
** - AT_GetStrideFromMetadata 
** - AT_GetTimeStampFromMetadata 
*/
#include <atutility.h>	  /* Andor conversion utils (modified to work with C) */
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"

/* AT_TRUE has to match TRUE and AT_FALSE has to match false for calls to some of the routines in this 
** module to work/make sense  (e.g. CCD_Command_Set_Bool)
*/
#if TRUE != AT_TRUE
#error AT_TRUE does not match TRUE.
#endif
#if FALSE != AT_FALSE
#error AT_TRUE does not match TRUE.
#endif

/* hash defines */
/**
 * The default number of times to retry certain Andor library commands, before returning an error.
 */
#define COMMAND_DEFAULT_RETRY_COUNT         (3)
/**
 * The default length of time to sleep between each retry attempt when (re)trying Andor library commands.
 */
#define COMMAND_DEFAULT_RETRY_SLEEP_TIME_MS (1000)
/**
 * The default camera index to try and open a connection to.
 */
#define COMMAND_DEFAULT_CAMERA_INDEX        (0)
/**
 * The maximum feature name length supported by the multi-char <-> wide character conversion routines
 * in this library.
 */
#define COMMAND_MAX_FEATURE_NAME_LENGTH     (64)

/* data types */
/**
 * Data type holding local data to ccd_command. This consists of the following:
 * <dl>
 * <dt>Retry_Count</dt> <dd>The number of times to try certain Andor library commands, before returning an error.</dd>
 * <dt>Retry_Sleep_Time_Ms</dt> <dd>The length of time to sleep between each retry attempt 
 *     when (re)trying Andor library commands.</dd>
 * <dt>Camera_Index</dt> <dd>The camera index to try to open a connection to.</dd>
 * <dt>Handle</dt> <dd>The Andor libraries handle (identifying which camera to talk to).</dd>
 * <dt>MBS_To_WCS_Mutex</dt> <dd>If compiled in, a mutex used when calling mbstowcs.</dd>
 * </dl>
 */
struct Command_Struct
{
	int Retry_Count;
	int Retry_Sleep_Time_Ms;
	int Camera_Index;
	AT_H Handle;
#ifdef MBSTOWCS_MUTEXED
	pthread_mutex_t MBS_To_WCS_Mutex;
#endif
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Command_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Retry_Count</dt> <dd>COMMAND_DEFAULT_RETRY_COUNT</dd>
 * <dt>Retry_Sleep_Time_Ms</dt> <dd>COMMAND_DEFAULT_RETRY_SLEEP_TIME_MS</dd>
 * <dt>Camera_Index</dt> <dd>COMMAND_DEFAULT_CAMERA_INDEX</dd>
 * <dt>Handle</dt> <dd>AT_HANDLE_UNINITIALISED</dd>
 * <dt>MBS_To_WCS_Mutex</dt> <dd>If compiled in, PTHREAD_MUTEX_INITIALIZER</dd>
 * </dl>
 * @see #COMMAND_DEFAULT_RETRY_COUNT
 * @see #COMMAND_DEFAULT_RETRY_SLEEP_TIME_MS
 * @see #COMMAND_DEFAULT_CAMERA_INDEX
 */
static struct Command_Struct Command_Data = 
{
	COMMAND_DEFAULT_RETRY_COUNT,
	COMMAND_DEFAULT_RETRY_SLEEP_TIME_MS,
	COMMAND_DEFAULT_CAMERA_INDEX,
	AT_HANDLE_UNINITIALISED
#ifdef MBSTOWCS_MUTEXED
	,PTHREAD_MUTEX_INITIALIZER
#endif
};

/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Command_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */
static char* Command_Get_Andor_Error_String(int retval);
static int Command_MBS_To_WCS_String(char *mbs_string,AT_WC *wide_string,int wide_string_length);
static int Command_WCS_To_MBS_String(AT_WC *wide_string,char *mbs_string,int mbs_string_length);
#ifdef MBSTOWCS_MUTEXED
static int Command_MBS_To_WCS_Mutex_Lock(void);
static int Command_MBS_To_WCS_Mutex_Unlock(void);
#endif

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Initialise the CCD library. This is done by initialising the underlying Andor library, 
 * by calling AT_InitialiseLibrary and AT_InitialiseUtilityLibrary.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Initialise(void)
{
	int retval;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Initialise: Calling AT_InitialiseLibrary.");
#endif /* LOGGING */
	retval = AT_InitialiseLibrary();
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"CCD_Command_Initialise: AT_InitialiseLibrary failed (%d) : %s.",
			retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Initialise: Calling AT_InitialiseUtilityLibrary.");
#endif /* LOGGING */
	retval = AT_InitialiseUtilityLibrary();
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"CCD_Command_Initialise: AT_InitialiseUtilityLibrary failed (%d) : %s.",
			retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Finalise (finish using) the CCD library. This involves calling AT_FinaliseLibrary and AT_FinaliseUtilityLibrary.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Finalise(void)
{
	int retval;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Finalise: Calling AT_FinaliseLibrary.");
#endif /* LOGGING */
	retval = AT_FinaliseLibrary();
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"CCD_Command_Finalise: AT_FinaliseLibrary failed (%d) : %s.",
			retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Finalise: Calling AT_FinaliseUtilityLibrary.");
#endif /* LOGGING */
	retval = AT_FinaliseUtilityLibrary();
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"CCD_Command_Finalise: AT_FinaliseUtilityLibrary failed (%d) : %s.",
			retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Open a connection to the Andor camera and get a camera handle.
 * The AT_Open command does not always work first time, so we do this in a retry loop.
 * We try to open a camera connection to camera index Command_Data.Camera_Index. 
 * We retry the AT_Open command Command_Data.Retry_Count times. 
 * We sleep Command_Data.Retry_Sleep_Time_Ms between each attempt.
 * If successful, we store the returned handle in Command_Data.Handle.
 * @return The routine returns TRUE on success and FALSE if AT_Open is not successful 
 *         after Command_Data.Retry_Count attempts.
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Open(void)
{
	struct timespec sleep_time;
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Open: Started.");
#endif /* LOGGING */
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS to 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Open: Calling AT_Open(camera index = %d), attempt %d.",
				       Command_Data.Camera_Index,(retry_index+1));
#endif /* LOGGING */
		retval = AT_Open(Command_Data.Camera_Index,&(Command_Data.Handle));
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 5;
			sprintf(Command_Error_String,
				"CCD_Command_Open: AT_Open(camera index = %d) attempt %d, failed (%d) : %s.",
				Command_Data.Camera_Index,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log Error though */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,
			"CCD_Command_Open: AT_Open(camera index = %d) failed (%d) : %s after %d attempts.",
			Command_Data.Camera_Index,retval,Command_Get_Andor_Error_String(retval),retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Open: AT_Open(camera index = %d) returned camera handle %d "
			       "after %d attempts.",
			       Command_Data.Camera_Index,Command_Data.Handle,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Open: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Close an open connection to the Andor camera.
 * The AT_Close command does not always work first time, so we do this in a retry loop.
 * We retry the AT_Close command Command_Data.Retry_Count times. 
 * We sleep Command_Data.Retry_Sleep_Time_Ms between each attempt.
 * @return The routine returns TRUE on success and FALSE if AT_Close is not successful 
 *         after Command_Data.Retry_Count attempts.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Close(void)
{
	struct timespec sleep_time;
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Started.");
#endif /* LOGGING */
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS to 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Close: Calling AT_Close(camera handle = %d), attempt %d.",
				       Command_Data.Handle,(retry_index+1));
#endif /* LOGGING */
		retval = AT_Close(Command_Data.Handle);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 7;
			sprintf(Command_Error_String,
				"CCD_Command_Close: AT_Close(camera handle = %d) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,
			"CCD_Command_Close: AT_Close(camera handle = %d) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,retval,Command_Get_Andor_Error_String(retval),retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Close: AT_Close(camera handle = %d) succeeded after %d attempts.",
			       Command_Data.Handle,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Invoke the command specified by the feature_name_string.
 * @param feature_name_string The name of the command to invoke, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Command(char *feature_name_string)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Command: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Command: Call %s.",feature_name_string);
#endif /* LOGGING */
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Command: Calling AT_Command(camera handle = %d,"
				       "feature name = '%s'), attempt %d.",
				       Command_Data.Handle,feature_name_string,(retry_index+1));
#endif /* LOGGING */
		retval = AT_Command(Command_Data.Handle,feature_name_wide_string);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 42;
			sprintf(Command_Error_String,"CCD_Command_Command: AT_Command(camera handle = %d,"
				"feature name = '%s') attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 43;
		sprintf(Command_Error_String,"CCD_Command_Command: AT_Command(camera handle = %d,"
			"feature name = '%s') failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Command: AT_Command(camera handle = %d,"
			       "feature name = '%s') succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Command: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Flush the input and output queues associated with the Andor camera.
 * The AT_Flush command does not always work first time, so we do this in a retry loop.
 * We retry the AT_Flush command Command_Data.Retry_Count times. 
 * We sleep Command_Data.Retry_Sleep_Time_Ms between each attempt.
 * @return The routine returns TRUE on success and FALSE if AT_Flush is not successful 
 *         after Command_Data.Retry_Count attempts.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Flush(void)
{
	struct timespec sleep_time;
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Flush: Started.");
#endif /* LOGGING */
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS to 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Flush: Calling AT_Flush(camera handle = %d), attempt %d.",
				       Command_Data.Handle,(retry_index+1));
#endif /* LOGGING */
		retval = AT_Flush(Command_Data.Handle);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 44;
			sprintf(Command_Error_String,
				"CCD_Command_Close: AT_Flush(camera handle = %d) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 45;
		sprintf(Command_Error_String,
			"CCD_Command_Flush: AT_Flush(camera handle = %d) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,retval,Command_Get_Andor_Error_String(retval),retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Flush: AT_Flush(camera handle = %d) succeeded after %d attempts.",
			       Command_Data.Handle,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Flush: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the boolean value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an integer, that on success will be set to 
 *        the current boolean value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Bool(char *feature_name_string,int *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Bool: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Bool: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 16;
		sprintf(Command_Error_String,"CCD_Command_Get_Bool: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Bool: Calling AT_GetBool(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetBool(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 17;
			sprintf(Command_Error_String,"CCD_Command_Get_Bool: AT_GetBool(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 18;
		sprintf(Command_Error_String,"CCD_Command_Get_Bool: AT_GetBool(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Bool: AT_GetBool(camera handle = %d,"
			       "feature name = '%s', value = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Bool: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the number (count) of enumeration values of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the number of values for, as a normal string. 
 *        The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param count A pointer to an integer, that on success will be set to 
 *        the number of enumeration values for the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Enum_Count(char *feature_name_string,int *count)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Count: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Count: Get enumeration count for '%s'.",
			       feature_name_string);
#endif /* LOGGING */
	if(count == NULL)
	{
		Command_Error_Number = 52;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_Count: '%s' count is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Enum_Count: Calling AT_GetEnumCount(camera handle = %d,"
				       "feature name = '%s', count = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,count,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetEnumCount(Command_Data.Handle,feature_name_wide_string,count);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 53;
			sprintf(Command_Error_String,"CCD_Command_Get_Enum_Count: AT_GetEnumCount(camera handle = %d,"
				"feature name = '%s', count = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,count,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 54;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_Count: AT_GetEnumCount(camera handle = %d,"
			"feature name = '%s', count = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,count,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Enum_Count: AT_GetEnumCount(camera handle = %d,"
			       "feature name = '%s', count = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*count),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Count: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the enumeration index value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an integer, that on success will be set to 
 *        the enumeration index of the value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Enum_Index(char *feature_name_string,int *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Index: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Index: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 26;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_Index: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Enum_Index: Calling AT_GetEnumIndex(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetEnumIndex(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 27;
			sprintf(Command_Error_String,"CCD_Command_Get_Enum_Index: AT_GetEnumIndex(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 28;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_Index: AT_GetEnumIndex(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Enum_Index: AT_GetEnumIndex(camera handle = %d,"
			       "feature name = '%s', value = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_Index: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the string value of the enumeration called feature_name_string, with index index.
 * @param feature_name_string The name of the enumeration feature to get the value of, 
 *        as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param index The index of the value to retrieve in the enumeration.
 * @param value_string A string, that on success will be set to the value of the specified feature_name 
 *        enumeration with the specified index.
 * @param value_length The length of allocated space for value_string.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see #Command_WCS_To_MBS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Enum_String_By_Index(char *feature_name_string,int index,
					 char *value_string,int value_length)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	AT_WC value_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_String_By_Index: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,
			       "CCD_Command_Get_Enum_String_By_Index: Get enumerastion %s index %d.",
			       feature_name_string,index);
#endif /* LOGGING */
	if(value_string == NULL)
	{
		Command_Error_Number = 49;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_String_By_Index: '%s' value is NULL.",
			feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Enum_String_By_Index: "
				       "Calling AT_GetEnumStringByIndex(camera handle = %d,"
				       "feature name = '%s',index = %d), attempt %d.",
				       Command_Data.Handle,feature_name_string,index,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetEnumStringByIndex(Command_Data.Handle,feature_name_wide_string,index,
						 value_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 50;
			sprintf(Command_Error_String,"CCD_Command_Get_Enum_String_By_Index: "
				"AT_GetEnumStringByIndex(camera handle = %d,"
				"feature name = '%s',index = %d) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,index,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 51;
		sprintf(Command_Error_String,"CCD_Command_Get_Enum_String_By_Index: "
			"AT_GetEnumStringByIndex(camera handle = %d,"
			"feature name = '%s',index = %d) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,index,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
	/* convert value_wide_string to value */
	if(!Command_WCS_To_MBS_String(value_wide_string,value_string,value_length))
		return FALSE;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Enum_String_By_Index: "
			       "AT_GetEnumStringByIndex(camera handle = %d,"
			       "feature name = '%s', index = %d, value = '%s') succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,index,value_string,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Enum_String_By_Index: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the float (double) value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an double, that on success will be set to 
 *        the current double value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Float(char *feature_name_string,double *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 19;
		sprintf(Command_Error_String,"CCD_Command_Get_Float: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Float: Calling AT_GetFloat(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetFloat(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 20;
			sprintf(Command_Error_String,"CCD_Command_Get_Float: AT_GetFloat(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 21;
		sprintf(Command_Error_String,"CCD_Command_Get_Float: AT_GetFloat(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Float: AT_GetFloat(camera handle = %d,"
			       "feature name = '%s', value = %.2f) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the minimum float (double) value allowed of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an double, that on success will be set to 
 *        the minimum allowed double value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Float_Min(char *feature_name_string,double *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Min: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Min: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 36;
		sprintf(Command_Error_String,"CCD_Command_Get_Float_Min: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Float_Min: Calling AT_GetFloatMin(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetFloatMin(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 37;
			sprintf(Command_Error_String,"CCD_Command_Get_Float_Min: AT_GetFloatMin(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 38;
		sprintf(Command_Error_String,"CCD_Command_Get_Float_Min: AT_GetFloatMin(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Float_Min: AT_GetFloatMin(camera handle = %d,"
			       "feature name = '%s', value = %.2f) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Min: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the maximum float (double) value allowed of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the maximum value of, as a normal string. 
 *        The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an double, that on success will be set to 
 *        the maximum allowed double value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Float_Max(char *feature_name_string,double *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Max: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Max: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 39;
		sprintf(Command_Error_String,"CCD_Command_Get_Float_Max: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Float_Max: Calling AT_GetFloatMax(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetFloatMax(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 40;
			sprintf(Command_Error_String,"CCD_Command_Get_Float_Max: AT_GetFloatMax(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 41;
		sprintf(Command_Error_String,"CCD_Command_Get_Float_Max: AT_GetFloatMax(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Float_Max: AT_GetFloatMax(camera handle = %d,"
			       "feature name = '%s', value = %.2f) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Float_Max: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the integer value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A pointer to an integer, that on success will be set to 
 *        the current integer value of the specified feature_name.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Int(char *feature_name_string,int *value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	AT_64 andor_value;
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Int: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Int: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 33;
		sprintf(Command_Error_String,"CCD_Command_Get_Int: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_Int: Calling AT_GetInt(camera handle = %d,"
				       "feature name = '%s', value = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetInt(Command_Data.Handle,feature_name_wide_string,&andor_value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 34;
			sprintf(Command_Error_String,"CCD_Command_Get_Int: AT_GetInt(camera handle = %d,"
				"feature name = '%s', value = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,&andor_value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 35;
		sprintf(Command_Error_String,"CCD_Command_Get_Int: AT_GetInt(camera handle = %d,"
			"feature name = '%s', value = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
	/* convert andor return value to an integer */
	(*value) = (int)andor_value;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Int: AT_GetInt(camera handle = %d,"
			       "feature name = '%s', value = %ld = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,andor_value,(*value),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_Int: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the string value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to get the value of, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value A string, that on success will be set to the current value of the specified feature_name.
 * @param value_length The length of allocated space for value.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see #Command_WCS_To_MBS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_String(char *feature_name_string,char *value,int value_length)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	AT_WC value_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_String: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_String: Get %s.",feature_name_string);
#endif /* LOGGING */
	if(value == NULL)
	{
		Command_Error_Number = 29;
		sprintf(Command_Error_String,"CCD_Command_Get_String: '%s' value is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Get_String: Calling AT_GetString(camera handle = %d,"
				       "feature name = '%s'), attempt %d.",
				       Command_Data.Handle,feature_name_string,(retry_index+1));
#endif /* LOGGING */
		retval = AT_GetString(Command_Data.Handle,feature_name_wide_string,value_wide_string,
				      COMMAND_MAX_FEATURE_NAME_LENGTH+1);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 30;
			sprintf(Command_Error_String,"CCD_Command_Get_String: AT_GetString(camera handle = %d,"
				"feature name = '%s') attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 31;
		sprintf(Command_Error_String,"CCD_Command_Get_String: AT_GetString(camera handle = %d,"
			"feature name = '%s') failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
	/* convert value_wide_string to value */
	if(!Command_WCS_To_MBS_String(value_wide_string,value,value_length))
		return FALSE;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_String: AT_GetString(camera handle = %d,"
			       "feature name = '%s', value = '%s') succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,value,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Get_String: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Find out whether the specified feature is read only.
 * @param feature_name_string The name of the feature to test, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param is_readonly A pointer to an integer, that on success will be set to TRUE if the feature id read-only,
 *        and FALSE if the feature is read-write.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Is_Readonly(char *feature_name_string,int *is_readonly)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Is_Readonly: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Is_Readonly: Test %s is readonly.",feature_name_string);
#endif /* LOGGING */
	if(is_readonly == NULL)
	{
		Command_Error_Number = 46;
		sprintf(Command_Error_String,"CCD_Command_Is_Readonly: '%s' is_readonly is NULL.",feature_name_string);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Is_Readonly: Calling AT_IsReadOnly(camera handle = %d,"
				       "feature name = '%s', is_readonly = %p), attempt %d.",
				       Command_Data.Handle,feature_name_string,is_readonly,(retry_index+1));
#endif /* LOGGING */
		retval = AT_IsReadOnly(Command_Data.Handle,feature_name_wide_string,is_readonly);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 47;
			sprintf(Command_Error_String,"CCD_Command_Is_Readonly: AT_IsReadOnly(camera handle = %d,"
				"feature name = '%s', is_readonly = %p) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,is_readonly,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 48;
		sprintf(Command_Error_String,"CCD_Command_Is_Readonly: AT_IsReadOnly(camera handle = %d,"
			"feature name = '%s', is_readonly = %p) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,is_readonly,retval,
			Command_Get_Andor_Error_String(retval),retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Is_Readonly: AT_IsReadOnly(camera handle = %d,"
			       "feature name = '%s', is_readonly = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,(*is_readonly),retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Is_Readonly: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the boolean value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to set, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value The boolean value to set the feature_name string to.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Bool(char *feature_name_string,int value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Bool: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Bool: Set %s to %d.",feature_name_string,value);
#endif /* LOGGING */
	if(!CCD_GENERAL_IS_BOOLEAN(value))
	{
		Command_Error_Number = 9;
		sprintf(Command_Error_String,"CCD_Command_Set_Bool: '%s' value %d is not a boolean.",
			feature_name_string,value);
		return FALSE;
	}
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Set_Bool: Calling AT_SetBool(camera handle = %d,"
				       "feature name = '%s', value = %d), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_SetBool(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 12;
			sprintf(Command_Error_String,"CCD_Command_Set_Bool: AT_SetBool(camera handle = %d,"
				"feature name = '%s', value = %d) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 13;
		sprintf(Command_Error_String,"CCD_Command_Set_Bool: AT_SetBool(camera handle = %d,"
			"feature name = '%s', value = %d) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Set_Bool: AT_SetBool(camera handle = %d,"
			       "feature name = '%s', value = %d) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,value,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Bool: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the specified feature_name_string to the value represented by the enumeration value string enum_value_string.
 * @param feature_name_string The name of the feature to set, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param enum_value_string The enumeration value string. The String parameter must be one of
 *        the allowed values for the feature.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Enum_String(char *feature_name_string,char *enum_value_string)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	AT_WC enum_value_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Enum_String: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Enum_String: Set %s to %s.",feature_name_string,
			       enum_value_string);
#endif /* LOGGING */
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* convert enumerated value string to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(enum_value_string,enum_value_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Set_Enum_String: Calling AT_SetEnumString(camera handle = %d,"
				       "feature name = '%s', value = '%s'), attempt %d.",
				       Command_Data.Handle,feature_name_string,enum_value_string,(retry_index+1));
#endif /* LOGGING */
		retval = AT_SetEnumString(Command_Data.Handle,feature_name_wide_string,enum_value_wide_string);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 24;
			sprintf(Command_Error_String,
				"CCD_Command_Set_Enum_String: AT_SetEnumString(camera handle = %d,"
				"feature name = '%s', value = '%s') attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,enum_value_string,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 25;
		sprintf(Command_Error_String,"CCD_Command_Set_Enum_String: AT_SetEnumString(camera handle = %d,"
			"feature name = '%s', value = '%s') failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,enum_value_string,retval,
			Command_Get_Andor_Error_String(retval),retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Set_Enum_String: AT_SetEnumString(camera handle = %d,"
			       "feature name = '%s', value = '%s') succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,enum_value_string,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Enum_String: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the float (double) value of the specified feature_name_string.
 * @param feature_name_string The name of the feature to set, as a normal string. The routine will fail if
 *        the string is longer than COMMAND_MAX_FEATURE_NAME_LENGTH, which is used to allocate space
 *        for the wide character version of this string passed into the Andor API.
 * @param value The float value to set the feature_name to.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_MAX_FEATURE_NAME_LENGTH
 * @see #Command_Data
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Float(char *feature_name_string,double value)
{
	struct timespec sleep_time;
	AT_WC feature_name_wide_string[COMMAND_MAX_FEATURE_NAME_LENGTH+1];
	int retval,retry_index;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Float: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Float: Set %s to %.2f.",feature_name_string,value);
#endif /* LOGGING */
	/* convert feature name to a wide character string, suitable for the Andor API */
	if(!Command_MBS_To_WCS_String(feature_name_string,feature_name_wide_string,COMMAND_MAX_FEATURE_NAME_LENGTH+1))
		return FALSE;
	/* initialse loop variables */
	retry_index = 0;
	/* AT_SUCCESS is 0, so this initialises retval to something other than AT_SUCCESS, currently -1 */
	retval = AT_SUCCESS-1; 
	while((retval != AT_SUCCESS)&&(retry_index < Command_Data.Retry_Count))
	{
#if LOGGING > 0
		CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				       "CCD_Command_Set_Float: Calling AT_SetFloat(camera handle = %d,"
				       "feature name = '%s', value = %.2f), attempt %d.",
				       Command_Data.Handle,feature_name_string,value,(retry_index+1));
#endif /* LOGGING */
		retval = AT_SetBool(Command_Data.Handle,feature_name_wide_string,value);
		if(retval != AT_SUCCESS)
		{
			Command_Error_Number = 22;
			sprintf(Command_Error_String,"CCD_Command_Set_Float: AT_SetFloat(camera handle = %d,"
				"feature name = '%s', value = %.2f) attempt %d, failed (%d) : %s.",
				Command_Data.Handle,feature_name_string,value,(retry_index+1),retval,
				Command_Get_Andor_Error_String(retval));
			/* don't fail here, just retry. Log the error though. */
			CCD_General_Error();
			/* also sleep for a time before retrying */
			sleep_time.tv_sec = Command_Data.Retry_Sleep_Time_Ms/CCD_GENERAL_ONE_SECOND_MS;
			sleep_time.tv_nsec = (Command_Data.Retry_Sleep_Time_Ms*CCD_GENERAL_ONE_MILLISECOND_NS)%
				CCD_GENERAL_ONE_SECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
		}/* end if retval was not a success */
		retry_index ++;
	}/* end while */
	/* if we retried too many times without success, give up and return an error */
	if(retry_index >= Command_Data.Retry_Count)
	{
		Command_Error_Number = 23;
		sprintf(Command_Error_String,"CCD_Command_Set_Float: AT_SetFloat(camera handle = %d,"
			"feature name = '%s', value = %.2f) failed (%d) : %s after %d attempts.",
			Command_Data.Handle,feature_name_string,value,retval,Command_Get_Andor_Error_String(retval),
			retry_index);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Set_Float: AT_SetFloat(camera handle = %d,"
			       "feature name = '%s', value = %.2f) succeeded after %d attempts.",
			       Command_Data.Handle,feature_name_string,value,retry_index);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Set_Float: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to pass the specified buffer into the Andor libraries queue of buffers to put image data into.
 * @param buffer_ptr A pointer to the image buffer to add to the queue. 
 *        The buffer's start address has to be aligned to an 8-byte boundary according to the Andor SDK manual.
 * @param buffer_length The length of the image buffer in bytes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_Data
 */
int CCD_Command_Queue_Buffer(AT_U8* buffer_ptr,int buffer_length)
{
	int retval;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Queue_Buffer: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Queue_Buffer: AT_QueueBuffer(camera handle = %d,"
			       "buffer ptr = %p, buffer length = %d).",Command_Data.Handle,buffer_ptr,buffer_length);
#endif /* LOGGING */
	retval = AT_QueueBuffer(Command_Data.Handle,buffer_ptr,buffer_length);
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 55;
		sprintf(Command_Error_String,"CCD_Command_Queue_Buffer: AT_QueueBuffer(camera handle = %d,"
			"buffer ptr = %p, buffer length = %d) failed (%d) : %s.",
			Command_Data.Handle,buffer_ptr,buffer_length,retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Queue_Buffer: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Wait for a read-out buffer to become available.
 * @param buffer_ptr The address of a pointer to store a pointer to the newly acquired data into.
 * @param buffer_length The address of an integer to store the returned buffer length.
 * @param timeout How long to wait for new data to appear before returning an error. Use AT_INFINITE to wait forever. 
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_Data
 */
int CCD_Command_Wait_Buffer(AT_U8** buffer_ptr,int *buffer_length,unsigned int timeout)
{
	int retval;

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Wait_Buffer: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Wait_Buffer: Calling "
			       "AT_WaitBuffer(handle = %d, buffer ptr = %p, buffer length address = %p, timeout = %d).",
			       Command_Data.Handle,buffer_ptr,buffer_length,timeout);
#endif /* LOGGING */
	retval = AT_WaitBuffer(Command_Data.Handle,buffer_ptr,buffer_length,timeout);
	if(retval != AT_SUCCESS)
	{
		Command_Error_Number = 56;
		sprintf(Command_Error_String,"CCD_Command_Wait_Buffer: AT_WaitBuffer(camera handle = %d,"
			"buffer ptr = %p, buffer length = %d) failed (%d) : %s.",
			Command_Data.Handle,buffer_ptr,buffer_length,retval,Command_Get_Andor_Error_String(retval));
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,
			       "CCD_Command_Wait_Buffer: AT_WaitBuffer returned buffer %p of length %d .",
			       (*buffer_ptr),(*buffer_length));
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Wait_Buffer: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int CCD_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Command_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Command_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
/**
 * For the specified Andor SDK error code, try and return a sensible descriptive string.
 * @param retval An Andor SDK error code returned from an Andor function call.
 * @return A string describing the Andor SDK error code.
 */
static char* Command_Get_Andor_Error_String(int retval)
{
	switch(retval)
	{
		case AT_SUCCESS:
			return "SUCCESS";
		case AT_ERR_NOTINITIALISED:
			return "NOT INITIALISED";
		case AT_ERR_NOTIMPLEMENTED:
			return "NOT IMPLEMENTED";
		case AT_ERR_READONLY:
			return "READONLY";
		case AT_ERR_NOTREADABLE:
			return "NOT READABLE";
		case AT_ERR_NOTWRITABLE:
			return "NOT WRITABLE";
		case AT_ERR_OUTOFRANGE:
			return "OUT OF RANGE";
		case AT_ERR_INDEXNOTAVAILABLE:
			return "INDEX NOT AVAILABLE";
		case AT_ERR_INDEXNOTIMPLEMENTED:
			return "INDEX NOT IMPLEMENTED";
		case AT_ERR_EXCEEDEDMAXSTRINGLENGTH:
			return "EXCEEDED MAX STRING LENGTH";
		case AT_ERR_CONNECTION:
			return "CONNECTION";
		case AT_ERR_NODATA:
			return "NO DATA";
		case AT_ERR_INVALIDHANDLE:
			return "INVALID HANDLE";
		case AT_ERR_TIMEDOUT:
			return "TIMEDOUT";
		case AT_ERR_BUFFERFULL:
			return "BUFFER FULL";
		case AT_ERR_INVALIDSIZE:
			return "INVALID SIZE";
		case AT_ERR_INVALIDALIGNMENT:
			return "INVALID ALIGNMENT";
		case AT_ERR_COMM:
			return "COMM";
		case AT_ERR_STRINGNOTAVAILABLE:
			return "STRING NOT AVAILABLE";
		case AT_ERR_STRINGNOTIMPLEMENTED:
			return "STRING NOT IMPLEMENTED";
		case AT_ERR_NULL_FEATURE:
			return "NULL FEATURE";
		case AT_ERR_NULL_HANDLE:
			return "NULL HANDLE";
		case AT_ERR_NULL_IMPLEMENTED_VAR:
			return "NULL IMPLEMENTED VAR";
		case AT_ERR_NULL_READABLE_VAR:
			return "NULL READABLE VAR";
		case AT_ERR_NULL_READONLY_VAR:
			return "NULL READONLY VAR";
		case AT_ERR_NULL_WRITABLE_VAR:
			return "NULL WRITABLE VAR";
		case AT_ERR_NULL_MINVALUE:
			return "NULL MIN VALUE";
		case AT_ERR_NULL_MAXVALUE:
			return "NULL MAX VALUE";
		case AT_ERR_NULL_VALUE:
			return "NULL VALUE";
		case AT_ERR_NULL_STRING:
			return "NULL STRING";
		case AT_ERR_NULL_COUNT_VAR:
			return "NULL COUNT VAR";
		case AT_ERR_NULL_ISAVAILABLE_VAR:
			return "NULL IS AVAILABLE VAR";
		case AT_ERR_NULL_MAXSTRINGLENGTH:
			return "NULL MAX STRING LENGTH";
		case AT_ERR_NULL_EVCALLBACK:
			return "NULL EV CALLBACK";
		case AT_ERR_NULL_QUEUE_PTR:
			return "NULL QUEUE PTR";
		case AT_ERR_NULL_WAIT_PTR:
			return "NULL WAIT PTR";
		case AT_ERR_NULL_PTRSIZE:
			return "NULL PTR SIZE";
		case AT_ERR_NOMEMORY:
			return "NO MEMORY";
		case AT_ERR_DEVICEINUSE:
			return "DEVICE IN USE";
		case AT_ERR_DEVICENOTFOUND:
			return "DEVICE NOT FOUND";
		case AT_ERR_INVALIDOUTPUTPIXELENCODING:
			return "INVALID OUTPUT PIXEL ENCODING";
		case AT_ERR_INVALIDINPUTPIXELENCODING:
			return "INVALID INPUT PIXEL ENCODING";
		case AT_ERR_INVALIDMETADATAINFO:
			return "INVALID META DATA INFO";
		case AT_ERR_CORRUPTEDMETADATA:
			return "CORRUPTED META DATA";
		case AT_ERR_METADATANOTFOUND:
			return "META DATA NOT FOUND";
		default:
			return "UNKNOWN";
	}
}

/**
 * Internal function to convert a multi-byte character string to a wide character string using mbstowcs.
 * If MBSTOWCS_MUTEXED is defined, a mutex is acquired/released around the mbstowcs call using 
 * Command_MBS_To_WCS_Mutex_Lock/Command_MBS_To_WCS_Mutex_Unlock.
 * @param mbs_string The multi-byte string to convert.
 * @param wide_string The wide character string to return. This should already have been allocated (i.e. be non-NULL),
 *        and be able to take wide_string_length characters.
 * @param wide_string_length The number of characters space in wide_string.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_Mutex_Lock
 * @see #Command_MBS_To_WCS_Mutex_Unlock
 */
static int Command_MBS_To_WCS_String(char *mbs_string,AT_WC *wide_string,int wide_string_length)
{
	size_t n_bytes;

	if(strlen(mbs_string) >= wide_string_length)
	{
		Command_Error_Number = 10;
		sprintf(Command_Error_String,"Command_MBS_To_WCS_String: string '%s' is too long (%d vs %d).",
			mbs_string,strlen(mbs_string),wide_string_length);
		return FALSE;
	}
#ifdef MBSTOWCS_MUTEXED
	if(!Command_MBS_To_WCS_Mutex_Lock())
		return FALSE;
#endif
	/* convert mbs_string to wide characters */
	/* mbstowcs may not be thread-safe, should we be using mbsrtowcs instead?
        ** But mbsrtowcs changes the value of mbs_string which I don't want. */
	n_bytes = mbstowcs(wide_string,mbs_string,wide_string_length);
	if(n_bytes < 0)
	{
#ifdef MBSTOWCS_MUTEXED
		Command_MBS_To_WCS_Mutex_Unlock();
#endif
		Command_Error_Number = 11;
		sprintf(Command_Error_String,"Command_MBS_To_WCS_String: Converting string '%s' failed (%d).",
			mbs_string,n_bytes);
		return FALSE;
	}
#ifdef MBSTOWCS_MUTEXED
	if(!Command_MBS_To_WCS_Mutex_Unlock())
		return FALSE;
#endif
	return TRUE;
}

/**
 * Internal function to convert a wide character string to a multi-byte character string using wcstombs.
 * If WCSTOMBS_MUTEXED is defined, a mutex is acquired/released around the wcstombs call using 
 * Command_MBS_To_WCS_Mutex_Lock/Command_MBS_To_WCS_Mutex_Unlock.
 * @param wide_string The wide character string to convert.
 * @param mbs_string The multi-byte string to return. This should already have been allocated (i.e. be non-NULL),
 *        and be able to take mbs_string_length characters.
 * @param mbs_string_length The number of characters space in mbs_string.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_MBS_To_WCS_Mutex_Lock
 * @see #Command_MBS_To_WCS_Mutex_Unlock
 */
static int Command_WCS_To_MBS_String(AT_WC *wide_string,char *mbs_string,int mbs_string_length)
{
	size_t n_bytes;

#ifdef WCSTOMBS_MUTEXED
	if(!Command_MBS_To_WCS_Mutex_Lock())
		return FALSE;
#endif
	/* convert wide_string to mbs_string */
	n_bytes = wcstombs(mbs_string,wide_string,mbs_string_length);
	if(n_bytes < 0)
	{
#ifdef WCSTOMBS_MUTEXED
		Command_MBS_To_WCS_Mutex_Unlock();
#endif
		Command_Error_Number = 32;
		sprintf(Command_Error_String,"Command_WCS_To_MBS_String: Converting string '%ls' failed (%d).",
			wide_string,n_bytes);
		return FALSE;
	}
#ifdef WCSTOMBS_MUTEXED
	if(!Command_MBS_To_WCS_Mutex_Unlock())
		return FALSE;
#endif
	return TRUE;
}

#ifdef MBSTOWCS_MUTEXED
/**
 * Routine to lock the mbstowcs access mutex. This will block until the mutex has been acquired,
 * unless an error occurs.
 * @return Returns TRUE if the mutex has been locked for access by this thread,
 * 	FALSE if an error occured.
 * @see #Command_Data
 */
static int Command_MBS_To_WCS_Mutex_Lock(void)
{
	int error_number;

	error_number = pthread_mutex_lock(&(Command_Data.MBS_To_WCS_Mutex));
	if(error_number != 0)
	{
		Command_Error_Number = 14;
		sprintf(Command_Error_String,"Command_MBS_To_WCS_Mutex_Lock:Mutex lock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to unlock the mbstowcs access mutex. 
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #Command_Data
 */
static int Command_MBS_To_WCS_Mutex_Unlock(void)
{
	int error_number;

	error_number = pthread_mutex_unlock(&(Command_Data.MBS_To_WCS_Mutex));
	if(error_number != 0)
	{
		Command_Error_Number = 15;
		sprintf(Command_Error_String,"Command_MBS_To_WCS_Mutex_Unlock:Mutex unlock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}
#endif
