/** @file moptop.c
  *
  * @brief Main MOPTOP process
  *
  * @author asp
  *
  * @date 2019-08-09
  */

#define MAIN
#include "moptop.h"

/** @brief     Graceful exit 
  *
  * @param[in] code = exit code to be returned to called
  *
  * @return    code = exit code 
  */
void mop_exit( int code  )
{
    int c = mop_inst; // Camera index

//  Kludge: If debugging single camera then switch identity 
    if ( mop_which != CAM_ALL )
        c = mop_which - 1; // Camera ID starts at 1 but camera index at 0 so adjust 

    cam_close( &mop_cam[c] ); // Tidy-up camera settings

    exit( code ); 
}


/** @brief      main() 
  *
  * @param[in]  argc   = argument count
  * @param[in] *argv[] = arguments 
  *
  * @return     none as exit is handled by mop_exit() 
  */
int main( int argc, char *argv[] )
{
    int c  =  0; // Camera index
    mop_cam_t *cam;

    mop_opts( argc, argv ); // Parse any command line options
    mop_init();             // Init. common data
    c = mop_inst;
    mop_log( utl_mkshm(), MOP_DBG, FAC_MOP, "utl_mkshm()");

    if ( !c )              
    {
//      Master process 
        mop_master = true;

//      Kludge: If debugging single camera then switch master identity 
        if ( mop_which != CAM_ALL )
            c = mop_which - 1; // Camera ID starts at 1 but camera index at 0 so adjust 
        cam = &mop_cam[c];
        mop_log( !fts_mkname( cam, fts_pfx ), MOP_DBG, FAC_MOP, "fts_mkname(INIT)");

//      Init, rotator first start position 
        mop_log( rot_init( rot_usb, ROT_BAUD, TMO_ROTATE, ROT_TRG_HI ), MOP_DBG, FAC_MOP, "rot_init()"); 

//      Camera and image init.
        mop_log( cam_init  ( c            ), MOP_DBG, FAC_MOP, "cam_init()" );
        mop_log( cam_open  ( cam, cam_exp ), MOP_DBG, FAC_MOP, "cam_open()" );
        mop_log( cam_alloc ( cam          ), MOP_DBG, FAC_MOP, "cam_alloc()");
        mop_log( cam_queue ( cam          ), MOP_DBG, FAC_MOP, "cam_queue()");
        mop_log( cam_stable( cam, cam_temp, TMO_STABLE, cam_quick ), MOP_DBG, FAC_MOP, "cam_stable()");

//      If using all cameras then synchronise on temperature stable
        if ( mop_which == CAM_ALL )
            mop_log( utl_shm_wait( SHM_STABLE, CAM2, TMO_STABLE, true ), MOP_DBG, FAC_MOP, "utl_shm_stable(STABLE)");

//      CAUTION: AT_Command can take > 0.5s (!) so call any fn() using them before rotation
//      Reset camera clock and enable acquisition
        mop_log( cam_clk_rst(cam        ), MOP_DBG, FAC_MOP, "cam_clk_rst()" );  
        mop_log( cam_acq_ena(cam,AT_TRUE), MOP_DBG, FAC_MOP, "cam_acq_ena(T)");  

//      Synchronise slave with start of rotation 
        mop_log( utl_shm_post( SHM_ROTATE, CAM1 ), MOP_DBG, FAC_MOP, "utl_shm_post(ROTATE)"); 

//      Position rotator and start motion
        if ( !rot_sign &&  // 0 = Static 
             !rot_stp    ) // 0 = Single position
        {
//          No rotation,single static position  
            mop_log( rot_goto( rot_zero, TMO_ROTATE, &rot_zero ), MOP_INF, FAC_MOP, "Static position=%f", rot_zero );
            mop_log( cam_acq_stat( cam, fts_pfx ), MOP_DBG, FAC_MOP, "cam_acq_stat(ANGLE)");
        }
        else if ( rot_sign ) // Rotating with hardware triggering 
        {
//          Enable hardware trigger, start rotation, circular acquisition 
            mop_log( rot_trg_ena( true ),       MOP_DBG, FAC_MOP, "rot_trg_ena(T)" );
            mop_log( rot_move(rot_final),       MOP_DBG, FAC_MOP, "rot_move(final)");
            mop_log( cam_acq_circ(cam, fts_pfx),MOP_DBG, FAC_MOP, "cam_acq_circ()" );
            mop_log( rot_trg_ena( false ),      MOP_DBG, FAC_MOP, "rot_trg_ena(F)" ); 
        }
        else // Rotating with software triggering 
        {
            mop_log( cam_acq_stat( cam, fts_pfx ), MOP_DBG, FAC_MOP, "cam_acq_stat(ROTATING)");
        }
    }
    else 
    {
//      Slave process
        mop_master = false;
        cam = &mop_cam[c];
        mop_log( !fts_mkname( cam, fts_pfx ), MOP_DBG, FAC_MOP, "fts_mkname(INIT)");

//      Camera and image inits
        mop_log( cam_init  ( c            ), MOP_DBG, FAC_MOP, "cam_init()" );
        mop_log( cam_open  ( cam, cam_exp ), MOP_DBG, FAC_MOP, "cam_open()" );
        mop_log( cam_alloc ( cam          ), MOP_DBG, FAC_MOP, "cam_alloc()");
        mop_log( cam_queue ( cam          ), MOP_DBG, FAC_MOP, "cam_queue()");
        mop_log( cam_stable( cam, cam_temp, TMO_STABLE, cam_quick ), MOP_DBG, FAC_MOP, "cam_stable()");

//      Synchronise on temperature stable
        mop_log( utl_shm_post( SHM_STABLE, CAM2 ), MOP_DBG, FAC_MOP, "utl_shm_post(STABLE)");

//      Reset camera clock and enable acquisition
        mop_log( cam_clk_rst( cam          ), MOP_DBG, FAC_MOP, "cam_clk_rst()" );  
        mop_log( cam_acq_ena( cam, AT_TRUE ), MOP_DBG, FAC_MOP, "cam_acq_ena(T)");  

//      Synchronise on rotation starting
        mop_log( utl_shm_wait( SHM_ROTATE, CAM1, TMO_ROTATE, true ), MOP_DBG, FAC_MOP, "utl_shm_wait(ROTATE)");

//      Acquire images	
        if ( rot_sign )
            mop_log( cam_acq_circ( cam, fts_pfx ), MOP_DBG, FAC_MOP, "cam_acq_circ()");
        else
            mop_log( cam_acq_stat( cam, fts_pfx ), MOP_DBG, FAC_MOP, "cam_acq_stat()");
    }

    mop_exit( EXIT_SUCCESS );
}
