/* pirot_move.c
** PI Rotator move routines
** $Header$
*/
/**
 * PI Rotator move routines.
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
#include "log_udp.h"
#include "pirot_general.h"
#include "pirot_command.h"
#include "pirot_move.h"

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Variable holding error code of last operation performed.
 */
static int Move_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see #PIROT_ERROR_STRING_LENGTH
 */
static char Move_Error_String[PIROT_ERROR_STRING_LENGTH] = "";

/* =======================================
**  external functions 
** ======================================= */
/**
 * Wait for the rotator to report it is "on target".
 * We enter a loop and call PIROT_Command_Query_ONT until the rotator reports "on target" or the timeout period
 * has elapsed.
 * @param timeout_ms The length of time to wait (in milliseconds) for the rotator to report it is "on target". 
 *        If the rotator is not in position / "on target" after this of length, the routine returns an error/FALSE.
 * @return The routine returns TRUE on success and FALSE on failure. The routine returns FALSE if the 
 *         rotator does not report "on_target" within the specified timeout period.
 * @see #Move_Error_Number
 * @see #Move_Error_String
 * @see pirot_command.html#PIROT_Command_Query_ONT
 * @see pirot_general.html#PIROT_GENERAL_ONE_MILLISECOND_NS
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#fdifftime
 */
int PIROT_Move_Wait_For_On_Target(int timeout_ms)
{
	struct timespec loop_start_time,current_time,sleep_time;
	int on_target;

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Move_Wait_For_On_Target: Started.");
#endif /* LOGGING */
	Move_Error_Number = 0;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,
			 "PIROT_Move_Wait_For_On_Target: Waiting for up to %d ms for the rotator to report on target.",
			 timeout_ms);
#endif /* LOGGING */
	/* initialise loop variables */
	on_target = FALSE;
	clock_gettime(CLOCK_REALTIME,&loop_start_time);
	clock_gettime(CLOCK_REALTIME,&current_time);
	/* loop until the rotator reports it is on target, or we have waited longer than the timeout.
	** Note fdifftime reports elapsed time in _seconds_. */
	while((on_target == FALSE) && (fdifftime(current_time,loop_start_time) < 
				       ((double)(timeout_ms/PIROT_GENERAL_ONE_SECOND_MS))))
	{
		/* are we on target yet */
		if(!PIROT_Command_Query_ONT(&on_target))
		{
			Move_Error_Number = 1;
			sprintf(Move_Error_String,"PIROT_Move_Wait_For_On_Target: Failed to query on target.");
			return FALSE;
		}
		/* update current time */
		clock_gettime(CLOCK_REALTIME,&current_time);
		/* sleep a bit (1ms) */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = PIROT_GENERAL_ONE_MILLISECOND_NS;
		nanosleep(&sleep_time,&sleep_time);
	}/* end while */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Move_Wait_For_On_Target: Exited loop after %.2f seconds with on_target=%d.",
			 fdifftime(current_time,loop_start_time),on_target);
#endif /* LOGGING */
	if(on_target == FALSE)
	{
		Move_Error_Number = 2;
		sprintf(Move_Error_String,
			"PIROT_Move_Wait_For_On_Target: Rotator failed to move on target after %.2f seconds.",
			fdifftime(current_time,loop_start_time));
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Move_Wait_For_On_Target: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Move_Error_Number
 */
int PIROT_Move_Get_Error_Number(void)
{
	return Move_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Move_Error_Number
 * @see #Move_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Move_Error(void)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Move_Error_Number == 0)
		sprintf(Move_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s PIROT_Move:Error(%d) : %s\n",time_string,Move_Error_Number,Move_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Move_Error_Number
 * @see #Move_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Move_Error_String(char *error_string)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Move_Error_Number == 0)
		sprintf(Move_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s PIROT_Move:Error(%d) : %s\n",time_string,
		Move_Error_Number,Move_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
