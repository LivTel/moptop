/* pirot_general.c
** PI Rotator library
** $Header$
*/
/**
 * Error and Log handlers.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef MUTEXED
#include <pthread.h>
#endif
#include <sched.h> /* for sched_setscheduler */
#include "pirot_general.h"
#include "pirot_usb.h"
#include "pirot_command.h"
#include "pirot_setup.h"
#include "pirot_move.h"

/* defines */
/**
 * How long some buffers are when generating logging messages.
 */
#define LOG_BUFF_LENGTH           (4096)

/* data types */
/**
 * Data type holding local data to pirot_general. This consists of the following:
 * <dl>
 * <dt>Mutex</dt> <dd>Optionally compiled mutex locking over sending commands down the USB connection 
 *                    and receiving a reply.</dd>
 * <dt>Log_Handler</dt> <dd>Function pointer to the routine that will log messages passed to it.</dd>
 * <dt>Log_Filter</dt> <dd>Function pointer to the routine that will filter log messages passed to it.
 * 		The funtion will return TRUE if the message should be logged, and FALSE if it shouldn't.</dd>
 * <dt>Log_Filter_Level</dt> <dd>A globally maintained log filter level. 
 * 		This is set using PIROT_Set_Log_Filter_Level.
 * 		PIROT_Log_Filter_Level_Absolute and PIROT_Log_Filter_Level_Bitwise 
 *              test it against message levels to determine whether to log messages.</dd>
 * </dl>
 * @see #PIROT_Log
 * @see #PIROT_Set_Log_Filter_Level
 * @see #PIROT_Log_Filter_Level_Absolute
 * @see #PIROT_Log_Filter_Level_Bitwise
 */
struct General_Struct
{
#ifdef MUTEXED
	pthread_mutex_t Mutex;
#endif
	void (*Log_Handler)(int level,char *string);
	int (*Log_Filter)(int level,char *string);
	int Log_Filter_Level;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of General_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Mutex</dt> <dd>If compiled in, PTHREAD_MUTEX_INITIALIZER</dd>
 * <dt>Log_Handler</dt> <dd>NULL</dd>
 * <dt>Log_Filter</dt> <dd>NULL</dd>
 * <dt>Log_Filter_Level</dt> <dd>0</dd>
 * </dl>
 * @see #General_Struct
 */
static struct General_Struct General_Data = 
{
#ifdef MUTEXED
	PTHREAD_MUTEX_INITIALIZER,
#endif
	NULL,NULL,0,
};

/**
 * Variable holding error code of last operation performed.
 */
static int General_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see #PIROT_ERROR_STRING_LENGTH
 */
static char General_Error_String[PIROT_ERROR_STRING_LENGTH] = "";

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Basic error reporting routine, to stderr.
 * @see #General_Error_Number
 * @see #General_Error_String
 * @see #PIROT_General_Get_Current_Time_String
 * @see pirot_usb.html#PIROT_USB_Get_Error_Number
 * @see pirot_usb.html#PIROT_USB_Error
 * @see pirot_command.html#PIROT_Command_Get_Error_Number
 * @see pirot_command.html#PIROT_Command_Error
 * @see pirot_setup.html#PIROT_Setup_Get_Error_Number
 * @see pirot_setup.html#PIROT_Setup_Error
 * @see pirot_move.html#PIROT_Move_Get_Error_Number
 * @see pirot_move.html#PIROT_Move_Error
 */
void PIROT_General_Error(void)
{
	char time_string[32];
	int found = FALSE;

	if(PIROT_USB_Get_Error_Number() != 0)
	{
		found = TRUE;
		PIROT_USB_Error();
	}
	if(PIROT_Command_Get_Error_Number() != 0)
	{
		found = TRUE;
		PIROT_Command_Error();
	}
	if(PIROT_Setup_Get_Error_Number() != 0)
	{
		found = TRUE;
		PIROT_Setup_Error();
	}
	if(PIROT_Move_Get_Error_Number() != 0)
	{
		found = TRUE;
		PIROT_Move_Error();
	}
	if(General_Error_Number != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t\t\t");
		PIROT_General_Get_Current_Time_String(time_string,32);
		fprintf(stderr,"%s PIROT_General:Error(%d) : %s\n",time_string,General_Error_Number,
			General_Error_String);
	}
	if(!found)
	{
		fprintf(stderr,"%s PIROT_General_Error:An unknown error has occured.\n",time_string);
	}
}

/**
 * Basic error reporting routine, to the specified string.
 * @param error_string Pointer to an already allocated area of memory, to store the generated error string. 
 *        This should be at least 256 bytes long.
 * @see #General_Error_Number
 * @see #General_Error_String
 * @see #PIROT_General_Get_Current_Time_String
 * @see pirot_usb.html#PIROT_USB_Get_Error_Number
 * @see pirot_usb.html#PIROT_USB_Error_String
 * @see pirot_command.html#PIROT_Command_Get_Error_Number
 * @see pirot_command.html#PIROT_Command_Error_String
 * @see pirot_setup.html#PIROT_Setup_Get_Error_Number
 * @see pirot_setup.html#PIROT_Setup_Error_String
 * @see pirot_move.html#PIROT_Move_Get_Error_Number
 * @see pirot_move.html#PIROT_Move_Error_String
 */
void PIROT_General_Error_To_String(char *error_string)
{
	char time_string[32];

	strcpy(error_string,"");
	if(PIROT_USB_Get_Error_Number() != 0)
	{
		PIROT_USB_Error_String(error_string);
	}
	if(PIROT_Command_Get_Error_Number() != 0)
	{
		PIROT_Command_Error_String(error_string);
	}
	if(PIROT_Setup_Get_Error_Number() != 0)
	{
		PIROT_Setup_Error_String(error_string);
	}
	if(PIROT_Move_Get_Error_Number() != 0)
	{
		PIROT_Move_Error_String(error_string);
	}
	if(General_Error_Number != 0)
	{
		PIROT_General_Get_Current_Time_String(time_string,32);
		sprintf(error_string+strlen(error_string),"%s PIROT_General:Error(%d) : %s\n",time_string,
			General_Error_Number,General_Error_String);
	}
	if(strlen(error_string) == 0)
	{
		sprintf(error_string,"%s Error:PIROT_General:Error not found\n",time_string);
	}
}

/**
 * Routine to return the current value of the error number.
 * @return The value of General_Error_Number.
 * @see #General_Error_Number
 */
int PIROT_General_Get_Error_Number(void)
{
	return General_Error_Number;
}

/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59.123 UTC'.
 * The time is in UTC.
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 * @see #PIROT_GENERAL_ONE_MILLISECOND_NS
 */
void PIROT_General_Get_Current_Time_String(char *time_string,int string_length)
{
	char timezone_string[16];
	char millsecond_string[8];
	struct timespec current_time;
	struct tm *utc_time = NULL;

	clock_gettime(CLOCK_REALTIME,&current_time);
	utc_time = gmtime(&(current_time.tv_sec));
	strftime(time_string,string_length,"%d-%m-%YT%H:%M:%S",utc_time);
	sprintf(millsecond_string,"%03d",(current_time.tv_nsec/PIROT_GENERAL_ONE_MILLISECOND_NS));
	strftime(timezone_string,16,"%z",utc_time);
	if((strlen(time_string)+strlen(millsecond_string)+strlen(timezone_string)+3) < string_length)
	{
		strcat(time_string,".");
		strcat(time_string,millsecond_string);
		strcat(time_string," ");
		strcat(time_string,timezone_string);
	}
}

/**
 * Routine to log a message to a defined logging mechanism. This routine has an arbitary number of arguments,
 * and uses vsprintf to format them i.e. like fprintf. 
 * PIROT_Log is then called to handle the log message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param format A string, with formatting statements the same as fprintf would use to determine the type
 * 	of the following arguments.
 * @see #PIROT_Log
 * @see #LOG_BUFF_LENGTH
 */
void PIROT_Log_Format(int level,char *format,...)
{
	char buff[LOG_BUFF_LENGTH];
	va_list ap;

/* format the arguments */
	va_start(ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
/* call the log routine to log the results */
	PIROT_Log(level,buff);
}

/**
 * Routine to log a message to a defined logging mechanism. If the string or General_Data.Log_Handler are NULL
 * the routine does not log the message. If the General_Data.Log_Filter function pointer is non-NULL, the
 * message is passed to it to determine whether to log the message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param string The message to log.
 * @see #General_Data
 */
void PIROT_Log(int level,char *string)
{
/* If the string is NULL, don't log. */
	if(string == NULL)
		return;
/* If there is no log handler, return */
	if(General_Data.Log_Handler == NULL)
		return;
/* If there's a log filter, check it returns TRUE for this message */
	if(General_Data.Log_Filter != NULL)
	{
		if(General_Data.Log_Filter(level,string) == FALSE)
			return;
	}
/* We can log the message */
	(*General_Data.Log_Handler)(level,string);
}

/**
 * Routine to set the General_Data.Log_Handler used by Filter_Wheel_Log.
 * @param log_fn A function pointer to a suitable handler.
 * @see #General_Data
 * @see #PIROT_Log
 */
void PIROT_Set_Log_Handler_Function(void (*log_fn)(int level,char *string))
{
	General_Data.Log_Handler = log_fn;
}

/**
 * Routine to set the General_Data.Log_Filter used by Filter_Wheel_Log.
 * @param log_fn A function pointer to a suitable filter function.
 * @see #General_Data
 * @see #PIROT_Log
 */
void PIROT_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string))
{
	General_Data.Log_Filter = filter_fn;
}

/**
 * A log handler to be used for the General_Data.Log_Handler function.
 * Prints the message to stdout, terminated by a newline and Prepended by a timestamp generated by 
 * PIROT_General_Get_Current_Time_String.
 * @param level The log level for this message.
 * @param string The log message to be logged. 
 * @see #PIROT_General_Get_Current_Time_String
 */
void PIROT_Log_Handler_Stdout(int level,char *string)
{
	char time_string[32];

	if(string == NULL)
		return;
	PIROT_General_Get_Current_Time_String(time_string,32);
	fprintf(stdout,"%s %s\n",time_string,string);
}

/**
 * Routine to set the General_Data.Log_Filter_Level.
 * @see #General_Data
 */
void PIROT_Set_Log_Filter_Level(int level)
{
	General_Data.Log_Filter_Level = level;
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level is less than or equal to the General_Data.Log_Filter_Level,
 * 	otherwise it returns FALSE.
 * @see #General_Data
 */
int PIROT_Log_Filter_Level_Absolute(int level,char *string)
{
	return (level <= General_Data.Log_Filter_Level);
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level has bits set that are also set in the 
 * 	General_Data.Log_Filter_Level, otherwise it returns FALSE.
 * @see #General_Data
 */
int PIROT_Log_Filter_Level_Bitwise(int level,char *string)
{
	return ((level & General_Data.Log_Filter_Level) > 0);
}

#ifdef MUTEXED
/**
 * Routine to lock the access mutex. This will block until the mutex has been acquired,
 * unless an error occurs.
 * @return Returns TRUE if the mutex has been locked for access by this thread,
 * 	FALSE if an error occured.
 * @see #General_Data
 */
int PIROT_Mutex_Lock(void)
{
	int error_number;

	error_number = pthread_mutex_lock(&(General_Data.Mutex));
	if(error_number != 0)
	{
		General_Error_Number = 1;
		sprintf(General_Error_String,"PIROT_Mutex_Lock:Mutex lock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to unlock the access mutex. 
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #General_Data
 */
int PIROT_Mutex_Unlock(void)
{
	int error_number;

	error_number = pthread_mutex_unlock(&(General_Data.Mutex));
	if(error_number != 0)
	{
		General_Error_Number = 2;
		sprintf(General_Error_String,"PIROT_Mutex_Unlock:Mutex unlock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}
#endif

/**
 * Routine to replace instances of find_string with instances of replace_string in string, and return a 
 * string.
 * @param string The initial string. This is <b>not</b> modified.
 * @param find_string The string to look for.
 * @param replace_string The string to replace the found string with. Note the replacement string should
 *        not contain the find_string, otherwise an infinite loop can occur.
 * @return The routine returns the string, with instances of find_string replaced with replace_string.
 *        Note the string is internally static, and is over-written on each call to this routine. If not
 *        immediately used, it should be copied. The routine can return NULL if an error occured.
 * @see #LOG_BUFF_LENGTH
 */
char *PIROT_Replace_String(char *string,char *find_string,char *replace_string)
{
	static char return_string[LOG_BUFF_LENGTH];
	char *ptr = NULL;
	int done,start_index,end_index,i,move_count;

	if(string == NULL)
	{
		General_Error_Number = 3;
		sprintf(General_Error_String,"PIROT_Replace_String:string is null.");
		return NULL;
	}
	if(find_string == NULL)
	{
		General_Error_Number = 4;
		sprintf(General_Error_String,"PIROT_Replace_String:find_string is null.");
		return NULL;
	}
	if(replace_string == NULL)
	{
		General_Error_Number = 5;
		sprintf(General_Error_String,"PIROT_Replace_String:replace_string is null.");
		return NULL;
	}
	if(strlen(string) >= LOG_BUFF_LENGTH)
	{
		General_Error_Number = 6;
		sprintf(General_Error_String,"PIROT_Replace_String:string was too long (%d,%d).",
			strlen(string),LOG_BUFF_LENGTH);
		return NULL;
	}
	strcpy(return_string,string);
	done = FALSE;
	move_count = (int)(strlen(replace_string)-strlen(find_string));
	while(done == FALSE)
	{
		ptr = strstr(return_string,find_string);
		if(ptr != NULL)
		{
			start_index = (int)(ptr-return_string);
			end_index = strlen(return_string);
			if(move_count > 0)
			{
				if((strlen(return_string) + move_count) > LOG_BUFF_LENGTH)
				{
					General_Error_Number = 7;
					sprintf(General_Error_String,
						"PIROT_Replace_String:string is too short ((%d + %d) > %d).",
						strlen(return_string),move_count,LOG_BUFF_LENGTH);
					return NULL;
				}
				for(i=end_index;i>start_index;i--)
					return_string[i+move_count] = return_string[i];
			}
			else
			{
				for(i=start_index;i<end_index;i++)
					return_string[i] = return_string[i+move_count];
			}
			strncpy(return_string+start_index,replace_string,strlen(replace_string));
		}
		else
			done = TRUE;
	}/* end while */
	return return_string;
}

/**
 * Try to change the scheduler used for this process, to the round robin real-time scheduler.
 * This improves things like nanosleep response, without the process being real-time the
 * nanosleep(1 millisecond) can take 20ms to return. You have to be root for this function to succeed (under Linux).
 * @return The routine returns TRUE on success and FALSE on failure.
 */
int PIROT_General_Set_Real_Time_Scheduler(void)
{
	struct sched_param sched_param;

	/* setup real-time scheduler to get better nanosleep resolution.
	** Without doing this, nansleeps of 1ms take 20ms on ltobs9 */
	sched_param.sched_priority = sched_get_priority_max(SCHED_RR);
	if(sched_setscheduler(0, SCHED_RR, &sched_param) != 0)
	{
		General_Error_Number = 8;
		sprintf(General_Error_String,"PIROT_General_Set_Real_Time_Scheduler:Failed to set process scheduler.");
		return FALSE;
	}
	return TRUE;
}

/*
** $Log$
*/
