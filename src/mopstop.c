#define MAIN
#include "moptop.h"

#define SHM_NAME "/MOPTOP"
#define SHM_OPTS (O_RDWR | O_CREAT )

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define SEM_ROTATE "ROTATE"
#define SEM_STABLE "STABLE"
sem_t *sem_stable;
sem_t *sem_rotate;

int main( int argc, char *argv[] )
{
    int   c        =  0; // Camera index
    int   cam_tmo  = 60; // [s] Camera stabilisation timeout
    int   rot_tmo  =  5; // [s] Rotator motion timeout
    int   sync_tmo = 60; // [s] Synchronisation timeout 
    char *id;            // Master/slave string
    int   i;

    char *(shm[2]);	// Shared memory block

    mop_opts( argc, argv );                 // Parse any command line options
    mop_init();                             // Init. data

    c  = mop_inst;
    id = ( !c ) ? "Master" : "Slave "; // Figure out if we control rotator 

    if ( !c )
    {
//      If single camera mode then switch master identity to selected
        if ( mop_which != CAM_ALL )
        {
            c  = mop_which - 1;
            id = "Master";
        }

//      Master 
        mop_master = true;
        mop_log( true,                                  MOP_DBG, FAC_MOP, "ID=%s=%i"        ,id, c);
        mop_log( rot_init  ( rot_usb, ROT_BAUD, rot_tmo,ROT_TRG_HI ), MOP_DBG, FAC_MOP,  "rot_init()" ); 
        mop_log( utl_mksync( c                       ), MOP_DBG, FAC_MOP, "utl_mksync()"          );
        mop_log( cam_init  ( c                       ), MOP_DBG, FAC_MOP, "%s cam_init(%i)" ,id, c);
        mop_log( cam_open  ( &mop_cam[c], cam_exp    ), MOP_DBG, FAC_MOP, "%s cam_open(%i)" ,id, c);
        mop_log( cam_alloc ( &mop_cam[c], ALLOC_FRAG ), MOP_DBG, FAC_MOP, "%s cam_alloc(%i)",id, c);
        mop_log( cam_queue ( &mop_cam[c]             ), MOP_DBG, FAC_MOP, "%s cam_queue(%i)",id, c);
        mop_log( cam_stable( &mop_cam[c], cam_temp, cam_tmo, cam_quick ), MOP_DBG, FAC_MOP, "%s cam_stable(%i)", id, c);

//      If using all cameras then synchronise on stable temperature
        if ( mop_which == CAM_ALL )
        {
            utl_sync_set ( SYNC_STABLE );
            mop_log( true, MOP_DBG, FAC_MOP, "CAM0 stable, awaiting CAM1");
            utl_sync_wait( CAM1, SYNC_STABLE, true, sync_tmo ); 
            mop_log( true, MOP_DBG, FAC_MOP, "All stable" );
        }

//      Initialise and position rotator at start
        cam_acquireth( &c );
        cam_close( &mop_cam[c] );
        if ( !fts_sync )
            for( i =0; i < img_total; i++ )
                fts_write( fts_mkname( &mop_cam[c]), &mop_cam[c], i );
    }
    else
    {
//      Child process
        mop_master = false;
        mop_log( true,                                  MOP_DBG, FAC_MOP, "ID=%s=%i"        ,id, c);
        mop_log( utl_mksync( c                       ), MOP_DBG, FAC_MOP, "ult_mksync()"          );
        mop_log( cam_init  ( c                       ), MOP_INF, FAC_MOP, "%s cam_init(%i)" ,id, c);
        mop_log( cam_open  ( &mop_cam[c], cam_exp    ), MOP_DBG, FAC_MOP, "%s cam_open(%i)" ,id, c);
        mop_log( cam_alloc ( &mop_cam[c], ALLOC_FRAG ), MOP_DBG, FAC_MOP, "%s cam_alloc(%i)",id, c);
        mop_log( cam_queue ( &mop_cam[c]             ), MOP_DBG, FAC_MOP, "%s cam_queue(%i)",id, c);
        mop_log( cam_stable( &mop_cam[c], cam_temp, cam_tmo, cam_quick ), MOP_DBG, FAC_MOP, "%s cam_stable(%i)", id, c);

        utl_sync_set( SYNC_STABLE );
        mop_log( true,  MOP_DBG, FAC_MOP, "CAM1 stable, awaiting CAM0");
        utl_sync_wait( CAM0, SYNC_STABLE, true, sync_tmo ); 
        mop_log( true,  MOP_DBG, FAC_MOP, "All stable" );

        cam_acquireth( &c );
        cam_close( &mop_cam[c] );
        if ( !fts_sync )
            for( i =0; i < img_total; i++ )
                fts_write( fts_mkname( &mop_cam[c]), &mop_cam[c], i );
    }
}
