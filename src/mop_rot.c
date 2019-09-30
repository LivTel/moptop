/** @file   mop_rot.c
  *
  * @brief MOPTOP PI rotator functions  
  *
  * @author asp 
  *
  * @date   2019-07-09 
  */

#include "moptop.h"

/** @brief       Rotator debugging 
  *  
  * @param[in]   *dbg - Debug text to append to position 
  *
  * @return      rotator position angle [degrees] 
  */
double rot_dbg( char *dbg )
{
    double angle = 0.0;

    if ( mop_master )
    {
        if ( PI_qPOS( rot_id, rot_axis, &angle ))
            mop_log( true,  MOP_DBG, FAC_ROT, "angle=%8.3f DBG=%s", angle, dbg );
        else 
            mop_log( false, MOP_ERR, FAC_ROT, "PI_qPOS() fail" );
    }

    return angle;
}


/** @brief       Get the rotator position 
  *  
  * @param[out] *angle = Rotator position angle [degrees]
  *
  * @return      true|false = success/failure
  */
bool rot_get( double *angle )
{
    return PI_qPOS( rot_id, rot_axis, angle );
}


/** @brief       Set the rotator position with timeout
  *  
  * @param[in]   angle   = Rotator position [degrees] 
  * @param[in]   timeout = Timeout [seconds]
  *
  * @return      true|false = success/failure
  */
bool rot_set( double angle, int timeout )
{
    double now;

    mop_log( true, MOP_DBG, FAC_ROT, "rot_set(%f)", angle );
    PI_qPOS( rot_id, rot_axis, &now );
    PI_MOV(  rot_id, rot_axis, &angle );
    return rot_wait( angle, timeout, (angle > now) );  
}


/** @brief       Request rotator moves to a position  
  *  
  * @param[in]   angle = Rotator position [degrees] 
  *
  * @return      true|false = success/failure
  */
bool rot_move( double angle )
{
    mop_log( true, MOP_DBG, FAC_ROT, "rot_move(%f)", angle );
    return PI_MOV( rot_id, rot_axis, &angle );
}


/** @brief       Goto rotator position  
  *  
  * @param[in]   angle   = Requested position [degrees] 
  * @param[in]   timeout = Timeout [seconds]
  * @param[out]  actual  = Actual position (NULL=ignore) 
  *
  * @return      true|false = success/failure
  */
bool rot_goto( double angle, int timeout, double *actual )
{
    BOOL in_pos;                         // Reached target position? 

    int tick  = TIM_TICK;                           // Timer ticks [ms] 
    int count = TIM_MICROSECOND * timeout / tick;  // Timer count [ms] 

    mop_log( true, MOP_DBG, FAC_ROT, "rot_goto(%f)", angle );
    PI_MOV( rot_id, rot_axis, &angle );
    do
    { 
        PI_qONT( rot_id, rot_axis, &in_pos );
        if ( in_pos )
        {
            if ( actual )
            {
                PI_qPOS( rot_id, rot_axis, actual );
                return mop_log( true, MOP_DBG, FAC_ROT, "Goto angle=%f, actual=%f", angle, *actual );
            }
            else
            {
                return mop_log( true, MOP_DBG, FAC_ROT, "Goto angle=%f", angle );
            }
        }

        usleep(tick);
    } while( count-- );   

    return mop_log( false, MOP_ERR, FAC_ROT, "TIMEOUT: rot_goto(angle=%f timeout=%i)", angle, timeout );
}


/** @brief       Wait for rotator to move to/past a position  
  *  
  * @param[in]   pos     = Requested position [degrees] 
  * @param[in]   timeout = Timeout [seconds]
  * @param[in]   cw      = Moving direction is clockwise [boolean]
  *
  * @return      true|false = success/failure
  */
bool rot_wait( double angle, int timeout, bool cw )
{
    int tick  = TIM_TICK;                         // Timer ticks [ms] 
    int count = TIM_MICROSECOND * timeout / tick; // Timer count [ms] 
    double now;                        // Position now
    double dif;                        // Difference between requested angle and position now

    do
    { 
        PI_qPOS( rot_id, rot_axis, &now );
        dif = now - angle;
        if ( (  fabs( dif ) <= ROT_TOLERANCE )||  // Position is already within tolerance
             (  cw && dif   >= ROT_TOLERANCE )||  // Moving clockwise and past point
             ( !cw && dif   <= ROT_TOLERANCE )  ) // Moving counter-clockwise and past point
        {
	    return mop_log( true, MOP_DBG, FAC_ROT, "rot_wait(%f) actual=%f", angle, now );
        }
        usleep(tick);
    } while( count-- );   

    return mop_log( false, MOP_DBG, FAC_ROT, "TIMEOUT: rot_wait(angle=%f, timeout=%i)", angle, timeout );
}


/** @brief       Invoke a rotator command with log message  
  *
  * @param[in]   cmd = Rotator command string
  * @param[in]   inf = Associated log message 
  * 
  * @return      true|false = success/failure
  */
bool rot_cmd( char *cmd, char *log )
{
    if ( PI_GcsCommandset( rot_id, cmd ) != 1 )
        return mop_log( false, MOP_ERR, FAC_ROT, "cmd='%s' %s", cmd, log );
    else
        return mop_log( true,  MOP_DBG, FAC_ROT, "cmd='%s'", cmd );
}


/** @brief       Initialise rotator - negative trigger 
  *
  * @param[in]  *usb     = Full rotator device name 
  * @param[in]   baud    = Interface serial speed 
  * @param[in]   timeout = Timeout [seconds] 
  * 
  */
bool rot_init( char *usb, int baud, int timeout, char *rot_trg_lvl )
{
#define STR_LEN 63 

    char rotdev[128]; // Rotator serial device name
    char rotnum;      // Numer of rotators (must be 1) 

//  Configurable options
    char rot_trg_stp   [STR_LEN+1];  // Step size
    char rot_trg_vel   [STR_LEN+1];  // Velocity
    char rot_ini_pos   [STR_LEN+1];  // Initial position 
    char rot_trg_posend[STR_LEN+1];  // Max. trigger  

    snprintf( rot_trg_vel,   STR_LEN, ROT_TRG_VEL,   rot_vel ); 
    snprintf( rot_trg_stp,   STR_LEN, ROT_TRG_STP,   rot_stp ); 
    snprintf( rot_trg_posend,STR_LEN, ROT_TRG_POSEND,rot_sign * ROT_MAX ); 
    snprintf( rot_ini_pos,   STR_LEN, ROT_INI_POS,   rot_sign * ROT_INI_ANGLE * -1.0 ); 

//  First attempt to connect immediately after a reboot can fail so have several tries
    for( int try = 1; (rot_id = PI_ConnectRS232ByDevName( usb, baud )) < 0; try++ )
    {
        if (try < 5)
        {
            mop_log( false, MOP_WRN, FAC_ROT, "Rotator %s connection attempt %i", usb, try );
            sleep(1);
        }
        else 
        {
            return mop_log( false, MOP_CRIT, FAC_ROT, "Abort connecting to rotator %s.", usb );
        }
    }
    mop_log( true, MOP_DBG, FAC_ROT, "Connected to rotator %s ", usb );

    if (rot_cmd( ROT_CLR_ERR    ,"Clear any errors"          )&&
        rot_cmd( ROT_ALL_STOP   ,"Stop any motion"           )&&
        rot_cmd( ROT_TRG_DIS    ,"Disable trigger"           )&&
        rot_cmd( ROT_INI_VEL    ,"Set init. velocity"        )&&
        rot_cmd( ROT_INI_SVO    ,"Enable servo-ing"          )&&
        rot_cmd( ROT_INI_FRF    ,"Enable relative motion"    )&&
        rot_ont( TMO_ROTATE                                  )&& 
        rot_cmd( rot_trg_vel    ,"Set run velocity"          )&&
        rot_cmd( ROT_TRG_PIN_5  ,"Trigger output=pin 5"      )&&
        rot_cmd( rot_trg_lvl    ,"Trigger polarity"          )&&
	rot_cmd( rot_trg_stp    ,"Trigger step size"         )&&
        rot_cmd( ROT_TRG_POSSTP ,"Trigger at position & step")&&
    	rot_cmd( ROT_TRG_POSINIT,"Trigger past position"     )&&
    	rot_cmd( ROT_TRG_POSBEG ,"Trigger begin position"    )&&
    	rot_cmd( rot_trg_posend ,"Trigger end position"      )&&
//        rot_cmd( ROT_TRG_LEN    ,"Trigger pulse length"      )&&
        rot_cmd( rot_ini_pos    ,"Move to initial position"  )&&
        rot_ont( TMO_ROTATE                                  )  ) 
    {
	return true;
    }
    else
    {
        return mop_log( false,MOP_ERR, FAC_ROT, "rot_init()");
    }
}


/** @brief       Enable/disable rotator hardware trigger output 
  *
  * @param[in]   enable     = true|false 
  *
  * @return      true|false = success/failure
  */
bool rot_trg_ena( bool enable )
{
    if ( enable )
    {
        if ( rot_cmd( ROT_TRG_ENA, "Enable trigger" ))
    	    return true;
    	else
    	    return mop_log( false,MOP_ERR, FAC_ROT, "rot_trg_ena(T)");
    }
    else
    {
        if ( rot_cmd( ROT_TRG_DIS, "Disable trigger" ))
    	    return true;
    	else
    	    return mop_log( false,MOP_ERR, FAC_ROT, "rot_trg_ena(F)");
    }
}	


/** @brief       Reports rotator on target   
  *
  * @param[in]   timeout [s] 
  *
  * @return      true|false = success/failure
  */
bool rot_ont( int timeout )
{
    BOOL in_pos; // Reached target position? 

    int tick  = TIM_TICK;                          // Timer tick  [ms] 
    int count = TIM_MICROSECOND * timeout / tick; // Timer count [ms] 
    do
    { 
        PI_qONT( rot_id, rot_axis, &in_pos );
        if ( in_pos )
	    return true;

        usleep(tick);
    } while( count-- );   

    return mop_log( false, MOP_ERR, FAC_ROT, "TIMEOUT: rot_ont(%i)", timeout );
}
