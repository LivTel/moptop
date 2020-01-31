/* filter_wheel_command.c
** Starlight Express filter wheel command routines
** $Header$
*/
/**
 * Starlight Express filter wheel command routines. Based on Zej's code and the Starlight Express The 
 * Universal Filter Wheel pdf (The_Universal_Filter_Wheel.pdf).
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/hidraw.h>
#include "log_udp.h"
#include "filter_wheel_general.h"
#include "filter_wheel_command.h"

/* hash defines */
/**
 * Length to use for character arrays.
 */
#define STRING_LENGTH                (256)
/**
 * The default filter wheel move timeout in milliseconds.
 */
#define DEFAULT_MOVE_TIMEOUT_MS      (10000)
/**
 * The number of filters in the wheel.
 */
#define FILTER_WHEEL_FILTER_COUNT    (5)

/* data types/structures */
/**
 * Data type holding local data to filter_wheel_command. This consists of the following:
 * <dl>
 * <dt>Fd</dt> <dd>The file descriptor of the Linux ioctl device opened to connect with the filter wheel.</dd>
 * <dt>Move_Timeout_Ms</dt> <dd>How long to attempt a move, in ms, before timing out with an error.</dd>
 * <dt>Raw_Name</dt> <dd>The raw name of the HID (filter wheel) device, of length STRING_LENGTH.</dd>
 * <dt>Filter_Count</dt> <dd>The number of filters in the wheel.</dd>
 * </dl>
 * @see #STRING_LENGTH
 */
struct Command_Struct
{
	int Fd;
	int Move_Timeout_Ms;
	char Raw_Name[STRING_LENGTH];
	int Filter_Count;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Command_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Fd</dt> <dd>-1</dd>
 * <dt>Move_Timeout_Ms</dt> <dd>DEFAULT_MOVE_TIMEOUT_MS</dd>
 * <dt>Raw_Name</dt> <dd>""</dd>
 * <dt>Filter_Count</dt> <dd>FILTER_WHEEL_FILTER_COUNT</dd>
 * </dl>
 * @see #DEFAULT_MOVE_TIMEOUT_MS
 * @see #FILTER_WHEEL_FILTER_COUNT
 * @see #Command_Struct
 */
static struct Command_Struct Command_Data = 
{
	-1,DEFAULT_MOVE_TIMEOUT_MS,"",FILTER_WHEEL_FILTER_COUNT
};

/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see filter_wheel_general.html#FILTER_WHEEL_ERROR_STRING_LENGTH
 */
static char Command_Error_String[FILTER_WHEEL_ERROR_STRING_LENGTH] = "";

/* =======================================
**  external functions 
** ======================================= */
/**
 * Open a connection to the filter wheel.
 * <ul>
 * <li>If compiled in the filter wheel mutex is obtained.
 * <li>A connection is opened to the filter wheel using open on the device_name parameter.
 * <li>Retrieve the raw name of the HID device, and store it in Command_Data.Raw_Name.
 * <li>If compiled in the filter wheel mutex is released.
 * </ul>
 * @param device_name The device filename of the device to open e.g. /dev/hidraw0.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #STRING_LENGTH
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_Log_Format
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Lock
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Unlock
 */
int Filter_Wheel_Command_Open(char *device_name)
{
	int open_errno,ioctl_retval;

#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Open(device_name=%s): Started.",device_name);
#endif /* LOGGING */
	/* initialise error number */
	Command_Error_Number = 0;
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Lock())
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Open: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	/* open device connection */
	Command_Data.Fd = open(device_name,O_RDWR|O_NONBLOCK);
	if(Command_Data.Fd < 0)
	{
		open_errno = errno;
#ifdef MUTEXED
		Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Open: open(%s) failed with errno %d.",device_name,
			open_errno);
		return FALSE;
	}
	/* get HID raw name */
	ioctl_retval = ioctl(Command_Data.Fd,HIDIOCGRAWNAME(STRING_LENGTH),Command_Data.Raw_Name);
	if(ioctl_retval < 0)
	{
#ifdef MUTEXED
		Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Open: reading raw name failed with retval %d.",
			ioctl_retval);
		return FALSE;

	}
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Open: Raw Name = '%s'.",
				Command_Data.Raw_Name);
#endif /* LOGGING */
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Unlock())
	{
		Command_Error_Number = 7;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Open: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Open: Finished.");
#endif /* LOGGING */
	return TRUE;
}
/**
 * Close the previously opened connection to the filter wheel. 
 * <ul>
 * <li>If compiled in the filter wheel mutex is obtained.
 * <li>The close routine is called on the file descriptor held in Command_Data.Fd.
 * <li>If compiled in the filter wheel mutex is released.
 * </ul>
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_Log_Format
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Lock
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Unlock
 */
int Filter_Wheel_Command_Close(void)
{
	int close_retval, close_errno;

#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Close: Started.");
#endif /* LOGGING */
	/* initialise error number */
	Command_Error_Number = 0;
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Lock())
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Close: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	close_retval = close(Command_Data.Fd);
	if(close_retval < 0)
	{
		close_errno = errno;
#ifdef MUTEXED
		Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Close: close(%d) failed with errno %d.",
			Command_Data.Fd,close_errno);
		return FALSE;
	}
	Command_Data.Fd = -1;
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Unlock())
	{
		Command_Error_Number = 9;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Close: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Close: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Move the filter wheel into the specified position.
 * <ul>
 * <li>We check whether the position argument is out of range (1..Command_Data.Filter_Count).
 * <li>We setup the write data packet, loop timing/timout and loop exit variables.
 * <li>We enter a while loop, until the wheel is reporting in position or we time out (take too long). The configured
 *     timeout is held in Command_Data.Move_Timeout_Ms.
 *     <ul>
 *     <li>If compiled in we lock a mutex over writing to the device and receiving a reply. 
 *         We do this every time round the loop
 *         so an external thread can attempt to query the wheel's position, whilst a move is in operation.
 *     <li>We write the write data packet to the file descriptor Command_Data.Fd.
 *     <li>We sleep a bit, the manual recommends about 1 millisecond - we sleep for 10 milliseconds.
 *     <li>We update the current time (used for timeout calculations).
 *     <li>We read two bytes from the file descriptor Command_Data.Fd into a read data packet.
 *     <li>If compiled in we unlock the mutex.
 *     <li>We extract the current position from the read data packet. Note the current position returned is 0
 *         if the wheel is moving.
 *     <li>We check whether the current position is the target position, and set a variable used to
 *         determine whether to exit the loop.
 *     <li>We increment a loop counter (used to moderate logging).
 *     </ul>
 * <li>We check whether the loop exited due to a timeout (Command_Data.Move_Timeout_Ms), and return an error if this
 *     is the case.
 * <li>We check whether we have exited the loop without being in position, and return an error if this is the case.
 *     Note if the loop timeout test and  previous test are identical this test should never be TRUE.
 * </ul>
 * @param position The position to move the filter wheel to. This is a positive integer from 1 to the number of filters
 *        in the wheel.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_Log_Format
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Lock
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Unlock
 */
int Filter_Wheel_Command_Move(int position)
{
	struct timespec loop_start_time,current_time,sleep_time;
	char write_data_packet[2];
	char read_data_packet[2];
	int byte_count,retval,in_position,current_position,write_errno,read_errno,loop_count;

#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Move: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Move: Move wheel to position %d.",position);
#endif /* LOGGING */
	if((position < 1)||(position > Command_Data.Filter_Count))
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Move: position %d out of range(1..%d).",
			position,Command_Data.Filter_Count);
		return FALSE;
	}
	/* setup data packet to write */
	write_data_packet[0] = position;
	write_data_packet[1] = 0;	
	/* setup loop exit variable and timeout timestamps */
	clock_gettime(CLOCK_REALTIME,&loop_start_time);
	clock_gettime(CLOCK_REALTIME,&current_time);
	in_position = FALSE;
	loop_count = 0;
	/* loop until we are in position, we timeout or an error occurs.
	 * Note fdifftime reports elapsed time in _seconds_. */
	while((in_position == FALSE) && (fdifftime(current_time,loop_start_time) < 
					 ((double)(Command_Data.Move_Timeout_Ms/FILTER_WHEEL_GENERAL_ONE_SECOND_MS))))
	{
#if LOGGING > 0
		/* only log once every 10 loops to reduce logging */
		if((loop_count % 10) == 0)
		{
			Filter_Wheel_Log_Format(LOG_VERBOSITY_VERBOSE,
						"Filter_Wheel_Command_Move: Writing command bytes {%d,%d}, loop %d.",
						write_data_packet[0],write_data_packet[1],loop_count);
		}
#endif /* LOGGING */
#ifdef MUTEXED
		if(!Filter_Wheel_Mutex_Lock())
		{
			Command_Error_Number = 10;
			sprintf(Command_Error_String,"Filter_Wheel_Command_Move: failed to lock mutex.");
			return FALSE;
		}
#endif /* MUTEXED */
		/* write request to filter wheel */
		byte_count = write(Command_Data.Fd,write_data_packet,2);
		if(byte_count != 2)
		{
			write_errno = errno;
#ifdef MUTEXED
			Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
			Command_Error_Number = 5;
			sprintf(Command_Error_String,
				"Filter_Wheel_Command_Move: write(%d,{%d,%d},2) failed with errno %d.",
				Command_Data.Fd,write_data_packet[0],write_data_packet[1],write_errno);
			return FALSE;
		}
		/* wait a bit (10ms) before reading a response */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = 10*FILTER_WHEEL_GENERAL_ONE_MILLISECOND_NS;
		nanosleep(&sleep_time,&sleep_time);
		/* update current time */
		clock_gettime(CLOCK_REALTIME,&current_time);
		/* read back a reply containing the current position of the wheel */
#if LOGGING > 0
		/* only log once every 10 loops to reduce logging */
		if((loop_count % 10) == 0)
		{
			Filter_Wheel_Log_Format(LOG_VERBOSITY_VERBOSE,
						"Filter_Wheel_Command_Move: Reading command bytes, loop %d.",loop_count);
		}
#endif /* LOGGING */
		byte_count = read(Command_Data.Fd,read_data_packet,2);
		if(byte_count != 2)
		{
			read_errno = errno;
#ifdef MUTEXED
			Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
			Command_Error_Number = 11;
			sprintf(Command_Error_String,
				"Filter_Wheel_Command_Move: read(%d,{%d,%d},2) failed with errno %d.",
				Command_Data.Fd,read_data_packet[0],read_data_packet[1],read_errno);
			return FALSE;
		}
#ifdef MUTEXED
		if(!Filter_Wheel_Mutex_Unlock())
		{
			Command_Error_Number = 12;
			sprintf(Command_Error_String,"Filter_Wheel_Command_Move: failed to unlock mutex.");
			return FALSE;
		}
#endif /* MUTEXED */
		/* retrieve current position from the read data packet, byte 0 */
		current_position = read_data_packet[0];
		/* are we in the requested position? */
		in_position = (position == current_position);
#if LOGGING > 0
		/* only log once every 10 loops to reduce logging */
		if((loop_count % 10) == 0)
		{
			Filter_Wheel_Log_Format(LOG_VERBOSITY_VERBOSE,
						"Filter_Wheel_Command_Move: Current Position %d, In Position %d, "
						"Elapsed time %.2f s, loop count %d.",
						current_position,in_position,fdifftime(current_time,loop_start_time),
						loop_count);
		}
#endif /* LOGGING */
		/* increment the loop counter. This is used to moderate the amount of logging generated. */
		loop_count++;
	}/* end while */
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_VERBOSE,
				"Filter_Wheel_Command_Move: Finished loop: Current Position %d, In Position %d, "
				"Elapsed time %.2f s, loop count %d.",
				current_position,in_position,fdifftime(current_time,loop_start_time),loop_count);
#endif /* LOGGING */
	/* check whether we timed out and return an error if so */
	if(fdifftime(current_time,loop_start_time) >=
	   ((double)(Command_Data.Move_Timeout_Ms/FILTER_WHEEL_GENERAL_ONE_SECOND_MS)))
	{
		Command_Error_Number = 13;
		sprintf(Command_Error_String,
			"Filter_Wheel_Command_Move: Move timed out after %.2f seconds (%d loops).",
			fdifftime(current_time,loop_start_time),loop_count);
		return FALSE;
	}
	/* check whether we exited the loop not in position.
	** As we have already checked for a timeout this should _never_ happen. */
	if(in_position == FALSE)
	{
		Command_Error_Number = 14;
		sprintf(Command_Error_String,
			"Filter_Wheel_Command_Move: Move finished but in_position FALSE after %d loops.",loop_count);
		return FALSE;		
	}
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Move: Finished Move to position %d.",position);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current position of the filter wheel.
 * <ul>
 * <li>We check the input parameter is OK.
 * <li>If compiled in we lock a mutex over sending the "Request current filter number" command and receiving a reply.
 * <li>We setup a data packet to write.
 * <li>We write the data packet to the filter wheel file descriptor (Command_Data.Fd).
 * <li>We sleep for a while (10ms). The manual suggests the reply should be sent after around 1ms.
 * <li>We read the reply data packet from the file descriptor (Command_Data.Fd).
 * <li>If compiled in we unlock the mutex.
 * <li>We extract the returned data from the returned reply data packet.
 * <li>We set the returned position to be the current position returned in the reply data packet.
 * </ul>
 * @param position The address of an integer to store the current position. The returned value will be
 *        the current filter position (1 to the number of filters in the wheel) or 0 if the wheel is moving.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_Log_Format
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Lock
 * @see filter_wheel_general.html#Filter_Wheel_Mutex_Unlock
 */
int Filter_Wheel_Command_Get_Position(int *position)
{
	struct timespec sleep_time;
	char write_data_packet[2];
	char read_data_packet[2];
	int byte_count,retval,write_errno,read_errno,current_filter_position,filter_count;

#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Get_Position: Started.");
#endif /* LOGGING */
	if(position == NULL)
	{
		Command_Error_Number = 15;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Get_Position: position was NULL.");
		return FALSE;
	}
	/* initialise error number */
	Command_Error_Number = 0;
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Lock())
	{
		Command_Error_Number = 16;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Get_Position: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	/* setup data packet to write. {0,0} is "Request current filter number" */
	write_data_packet[0] = 0;
	write_data_packet[1] = 0;	
	/* write request to filter wheel */
#if LOGGING > 5
	Filter_Wheel_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				"Filter_Wheel_Command_Get_Position: Writing command bytes {%d,%d}.",
				write_data_packet[0],write_data_packet[1]);
#endif /* LOGGING */
	byte_count = write(Command_Data.Fd,write_data_packet,2);
	if(byte_count != 2)
	{
		write_errno = errno;
#ifdef MUTEXED
		Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
		Command_Error_Number = 17;
		sprintf(Command_Error_String,
			"Filter_Wheel_Command_Get_Position: write(%d,{%d,%d},2) failed with errno %d.",
			Command_Data.Fd,write_data_packet[0],write_data_packet[1],write_errno);
		return FALSE;
	}
#if LOGGING > 5
	Filter_Wheel_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"Filter_Wheel_Command_Get_Position: Sleeping for 10ms.");
#endif /* LOGGING */
	/* wait a bit (10ms) before reading a response */
	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = 10*FILTER_WHEEL_GENERAL_ONE_MILLISECOND_NS;
	nanosleep(&sleep_time,&sleep_time);
	/* read reply from filter wheel */
#if LOGGING > 5
	Filter_Wheel_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				"Filter_Wheel_Command_Get_Position: Reading reply from filter wheel.");
#endif /* LOGGING */
	byte_count = read(Command_Data.Fd,read_data_packet,2);
	if(byte_count != 2)
	{
		read_errno = errno;
#ifdef MUTEXED
		Filter_Wheel_Mutex_Unlock();
#endif /* MUTEXED */
		Command_Error_Number = 18;
		sprintf(Command_Error_String,
			"Filter_Wheel_Command_Get_Position: read(%d,{%d,%d},2) failed with errno %d.",
			Command_Data.Fd,read_data_packet[0],read_data_packet[1],read_errno);
		return FALSE;
	}
#ifdef MUTEXED
	if(!Filter_Wheel_Mutex_Unlock())
	{
		Command_Error_Number = 19;
		sprintf(Command_Error_String,"Filter_Wheel_Command_Get_Position: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	/* extract returned data from filer wheel */
	current_filter_position = read_data_packet[0];
	filter_count = read_data_packet[1];
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				"Filter_Wheel_Command_Get_Position: Current position = %d, filter count = %d.",
				current_filter_position,filter_count);
#endif /* LOGGING */
	/* return current position */
	(*position) = current_filter_position;
#if LOGGING > 0
	Filter_Wheel_Log_Format(LOG_VERBOSITY_TERSE,"Filter_Wheel_Command_Get_Position: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int Filter_Wheel_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see filter_wheel_general.html#Filter_Wheel_General_Get_Current_Time_String
 */
void Filter_Wheel_Command_Error(void)
{
	char time_string[32];

	Filter_Wheel_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Filter_Wheel_Command:Error(%d) : %s\n",time_string,
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
 * @see filter_wheel_general.html#Filter_Wheel_General_Get_Current_Time_String
 */
void Filter_Wheel_Command_Error_String(char *error_string)
{
	char time_string[32];

	Filter_Wheel_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Filter_Wheel_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
