/** @file   mop_log.c
  *
  * @brief MOPTOP logging functions  
  *
  * @author asp 
  *
  * @date   2019-05-01 
  */

#include "moptop.h"

/** @brief     Log message to screen 
  *
  * @param[in] ok    = boolean return value
  * @param[in] level = severity 
  * @param[in] fac   = facility ID
  * @param[in] fmt   = vprintf(...) format and data to be displayed
  *
  * @return    ret   = value passed in for return by calling function
  */
bool mop_log( bool ok, int level, int fac, char *fmt, ... )
{
    va_list args;
    char    hms[32]; // Timestamp string hh:mm:ss.ss 
    char    sec[16]; // Fractional seconds 

    struct timeval  t;  // 

//  Only output messages for current log level    
    if ( level <= log_level )
    {
	va_start( args, fmt );

//      Get timestamd and check if fractional part rounds up 
        gettimeofday( &t, NULL );
        if (t.tv_usec >= 950000)
        {
            t.tv_sec++;           // round up integer seconds
            t.tv_usec -= 950000;  // round down fractional part 
        }

//      Set appropriate colour, format and output full log message, finally reset colour
        if ( log_colour[ level ] )
            printf("%s", log_colour[ level ] );
	strftime( hms, sizeof(hms)-1, "%Y-%m-%dT%H:%M:%S", localtime(&t.tv_sec) );
        snprintf( sec, sizeof(sec)-1, "%2.3f", t.tv_usec/TIM_MICROSECOND );
        printf  ("%s%s%s %s: %s%i ", dbg_prefix, hms, &sec[1], log_levels[level], fac_levels[fac], mop_inst+1 );
        vprintf ( fmt, args );
        printf  (" %s", ok ? "OK":"Fail"); 

//      Reset colour and print newline
        if ( log_colour[ level ] )
            puts( COL_RESET );
        else
            puts( "" );
    }
    return ok;
}
