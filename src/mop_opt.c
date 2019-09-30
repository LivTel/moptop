/** @file   mop_opt.c
  *
  * @brief  Process initialiastion and handle command line options   
  *
  * @author asp 
  *
  * @date   2019-05-01 
  */

#include "moptop.h"


/** @brief     Process initialisation
  *
  * @return    true|false = success/failure
  */
bool mop_init( void )
{
//  Calculate rotator final position
    img_total = img_cycle * rot_revs; 

    if (      rot_sign == ROT_CW )
        rot_final = img_total * rot_stp - ROT_TOLERANCE; // Avoid trigger overrun by stopping short 
    else if ( rot_sign == ROT_CCW )
        rot_final = img_total * rot_stp + ROT_TOLERANCE; // Avoid trigger overrun by stopping short 
     
    if (( rot_sign != 0.0                 )&&
        ( fabs(rot_stp/rot_vel) <= cam_exp)  )
        mop_log( true, MOP_WRN, FAC_OPT,
                "Exposure %f will overrun trigger %f interval", cam_exp, rot_stp/rot_vel );

//  Zero camera data 
    memset( mop_cam, 0, sizeof( mop_cam ));

    return true;
}


/** @brief     Process any command line options 
  *
  * @return    true|false = success/failure
  */
bool mop_opts(int argc, char *argv[] )
{
    int c;
    AT_WC Feature[255];
    char  str[8];
    char *ptr;

    struct stat st = {0};

    opterr = 0;

    while (( c = getopt( argc, argv, "h?q:f:o:r:d:e:x:n:u:t:v:c:r:a:m:p:b:s:E:O:R:D:w:W:F:C:A:Z:N:L:M:S:")) != -1 )
    {
        switch(c)
        {
            case 'r': // Set total number of revolutions  
                if ( ((rot_revs = atoi(optarg)) < 1        )|| 
	             ( rot_revs                 >  MAX_REVS)  )
                    mop_exit( mop_log( true, MOP_ERR, FAC_OPT,
                                      "Rotation count out of range, must be >0 and <%i", MAX_REVS ) );
                break;
            case 'd': // Set debug level
                log_level = atoi(optarg);
                break;
            case 'e': // Set exposure time
                if ( optarg[0] == 'a' || optarg[0] == 'A' )
                {
                    cam_auto = true;
                    break;
                }
                
                cam_exp = atof(optarg);
                if ( (cam_exp < 0.00001) || (cam_exp > 30.0) )
                    mop_exit( mop_log( false, MOP_ERR, FAC_OPT,
                             "Exposure %fs out of range, <0.00001 or >30.0", cam_exp )); 
                cam_auto = false;
		break;
            case 'q': // Don't wait for temp. to stabilise
                cam_quick = atoi(optarg) ? true : false;
		break;
            case 'E': // Display a detector enumerated feature
                mop_log( true, MOP_WRN, FAC_OPT, "-%c must be last option.",c); 
		mbstowcs( Feature, optarg, 254 );
		mop_init();
		cam_init( mop_cam1 );
		cam_open( &mop_cam[mop_cam1], cam_exp );
		cam_feature( &mop_cam[mop_cam1], Feature );
                mop_exit( EXIT_SUCCESS );
		break;
	    case 'n': // Set number of steps in a rotation
                switch ( atoi(optarg) )
                {  
                    case 8:
                        rot_stp   = ROT_STP8;
                        img_cycle = 8;
                        break;
                    case 16:
                        rot_stp   = ROT_STP16;
                        img_cycle = 16;
                        break;
                    default:
                        mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                                 "Unsupported images per rev.=%i. Use 8 or 16", atoi(optarg))); 
                        break;
                }
                break;
            case 'u': // Set the rotator USB device
                rot_usb = optarg;
                break;
            case 'v': // Set rotation velocity 
                rot_vel = atof(optarg);
                if ( rot_vel < 0.0 )
                {
                    rot_vel = -rot_vel;
                    rot_stp = -rot_stp;
                    rot_sign = ROT_CCW;
                }
                else if ( rot_vel == 0.0 )
                {
                    rot_vel  = ROT_VEL;    // Restore default value
                    rot_sign = ROT_STAT;   // Static mode
                    cam_trg  = CAM_TRG_SW; // Use software triggering
                }
                break;
            case 't': // Set target temperature
                cam_temp = atof(optarg);
                break;
            case 'c': // Camera, master or slave process 
                mop_inst = atoi(optarg) - 1;
                if ( mop_inst < 0 || mop_inst > 1 )
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Unsupported camera. Use 1=Master or 2=Slave" )); 
                break;
            case 'f': // Select readout rate
                switch ( atoi(optarg) )
                {
                    case 100:
                        cam_mhz = CAM_MHZ_100; 
                        break;
                    case 270:
                        cam_mhz = CAM_MHZ_270; 
                        break;
                    default:
                        mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                                 "Unsupported read rate=%i. Use 100 or 270", atoi(optarg) )); 
                        break; 
                }
                break;
            case 'o': // Select scan order 
                strtoupper( optarg );
                if      ( !strcmp( optarg, "BUSEQ" ))
                    cam_rd = CAM_RD_BUSEQ;
                else if ( !strcmp( optarg, "BUSIM" ))
                    cam_rd = CAM_RD_BUSIM;
                else if ( !strcmp( optarg, "COSIM" ))
                    cam_rd = CAM_RD_COSIM;
                else if ( !strcmp( optarg, "OISIM" ))
                    cam_rd = CAM_RD_OISIM;
                else if ( !strcmp( optarg, "TDSEQ" ))
                    cam_rd = CAM_RD_TDSEQ;
                else if ( !strcmp( optarg, "TDSIM" ))
                    cam_rd = CAM_RD_TDSIM;
                else 
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Unsupported read-out mode %s. Use BUSEQ, BUSIM, COSIM, OISIM, TDSEQ or TDSIM", optarg )); 
                break;
            case 'm': // Select amplifier mode 
                strtoupper( optarg );
                if      ( !strcmp( optarg, "12H" ))
                    cam_amp = CAM_AMP_12H;
                else if ( !strcmp( optarg, "12L" ))
                    cam_amp = CAM_AMP_12H;
                else if ( !strcmp( optarg, "16L" ))
                    cam_amp = CAM_AMP_12H;
                else 
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Unsupported gain mode %s. Use 12H, 12L or 16L", optarg )); 
                break;
            case 'a': // Fixed angle   
                rot_zero = atof(optarg);
                rot_sign = ROT_STAT;
                rot_stp  = 0.0;
                cam_trg  = CAM_TRG_SW;
                mop_log( true, MOP_WRN, FAC_OPT, "-%c overrides other options, must be last.", c); 
                break;
            case 'p': // Select data size
                strtoupper( optarg );
                if      ( !strcmp( optarg, "12"    ))
                    cam_enc = CAM_ENC_12;
                else if ( !strcmp( optarg, "12PACK"))
                    cam_enc = CAM_ENC_12PACK;
                else if ( !strcmp( optarg, "16"    ))
                    cam_enc = CAM_ENC_16;
//                else if ( !strcmp( optarg, "32"    )) \\ UNSUPPORTED
//                    cam_enc = CAM_ENC_32;
                else
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Invalid pixel encoding . Use 12, 12PACK or 16")); 
                break; 
            case 'b': // Set binning
                switch ( img_bin=atoi(optarg) )
                {
                    case 1:
                       cam_bin = CAM_BIN_1;
                       img_bin = fts_ccdxbin = fts_ccdybin = 1;
                       break;
                    case 2:
                       cam_bin = CAM_BIN_2;
                       img_bin = fts_ccdxbin = fts_ccdybin = 2;
                       break;
                    case 3:
                       cam_bin = CAM_BIN_3;
                       img_bin = fts_ccdxbin = fts_ccdybin = 3;
                       break;
                    case 4:
                       cam_bin = CAM_BIN_4;
                       img_bin = fts_ccdxbin = fts_ccdybin = 4;
                       break;
                    case 8:
                       cam_bin = CAM_BIN_8;
                       img_bin = fts_ccdxbin = fts_ccdybin = 8;
                       break;
                    default:
                        mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                                          "Invalid binning . Use 1, 2, 3, 4 or 8")); 
                        break;
                }               
                break;
            case 's': // Select single camera
                if (((mop_which = atoi(optarg)) < -1       )||
                     (mop_which                 > CAM_COUNT)  )
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Out of range. Use 1 to 2, or -1=ALL")); 
                break;
            case 'x': // Set exposure type
                switch ( fts_pfx = optarg[0] )
                {
                    case FTS_PFX_BIAS:
                       fts_typ = FTS_TYP_BIAS;
                       break;
                    case FTS_PFX_DARK:
                       fts_typ = FTS_TYP_DARK;
                       break;
                    case FTS_PFX_EXP:
                       fts_typ = FTS_TYP_EXP;
                       break;
                    case FTS_PFX_FLAT:
                       fts_typ = FTS_TYP_FLAT;
                       break;
                    case FTS_PFX_ACQ:
                       fts_typ = FTS_TYP_ACQ;
                       break;
                    case FTS_PFX_STD:
                       fts_typ = FTS_TYP_STD;
                       break;
                    default:
                       mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                                "Invalid exposure code. Use b,d,e,f,q, or s")); 
                       break; 
                }
                break;
            case 'w': // Set destination write folder (must exist)
                fts_dir = optarg;
                ptr = fts_dir + strlen(fts_dir) - 1; // Point to last char
                if ( *ptr == '/' )                  // If it is a directory terminator char ...
                    *ptr = '\0';                    // ... erase it as we add our own later

                if ( stat( fts_dir, &st ) )
                    mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                             "Cannot created destination %s with -w Use -W instead", fts_dir)); 
                break;
            case 'W': // Set destination write folder (will create) 
                fts_dir = optarg;
                ptr = fts_dir + strlen(fts_dir) - 1; // Point to last char
                if ( *ptr == '/' )                  // If it is a directory terminator char ...
                    *ptr = '\0';                    // ... erase it as we add our own later

                if ( stat( fts_dir, &st ) )
                {
                    if ( mkdir( fts_dir, 0777 ) && (errno != EEXIST) ) 
                        mop_exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_OPT,
                                 "Failed to create destination %s/. %s", fts_dir, strerror(errno))); 
                    else
                        mop_log( true, MOP_INF, FAC_OPT, "Created destination %s/", fts_dir); 
                }
                else
                {
                    mop_log( true, MOP_WRN, FAC_OPT, "Destination %s/ already exists", fts_dir); 
                }
                break;
            case 'O': // Set object name 
                fts_obj = optarg;
                break; 
            case 'R': // Set object RA 
                fts_ra = optarg;
                break; 
            case 'D': // Set object DEC 
                fts_dec = optarg;
                break; 
            case 'F': // Set telescope focus position 
                tel_foc = atof(optarg);
                break; 
            case 'C': // Set telescope CAS angle 
                tel_cas = atof(optarg);
                break; 
            case 'A': // Set telescope Altitude angle 
                tel_alt = atof(optarg);
                break; 
            case 'Z': // Set telescope aZimuth angle 
                tel_azm = atof(optarg);
                break; 
            case 'N': // No filter 
                tel_clear = true;
                break; 
            case 'L': // Logging prefix 
                dbg_prefix = optarg;
                break; 
            case 'h': // Help! 
                snprintf( str, sizeof(str)-1, "%i", mop_which+1 );
                printf("Usage: %s <-OPTION>             [ Version=%s  ]\n", argv[0], MOP_VERSION);
                printf("Opt:                                [ Default       ]\n"              );
                printf("  -e  Exposure time                 [% 6.3f sec     ]\n"  , cam_exp   );
                printf("  -x  eXosure type   <b,d,e,f,q,s>  [     %c         ]\n"   , fts_pfx   );
                printf("  -b  Binning        <1,2,3,4,8>    [   %ls         ]\n"  , cam_bin   );
                printf("  -f  read Freq.     <100,270>      [   %ls     ]\n"      , cam_mhz   );
                printf("  -m  Mode amp. gain <12H,12L,16L>  [   %ls ]\n"          , cam_amp   );
                printf("  -p  Pixel encoding <12,12PACK,16> [   %ls      ]\n"     , cam_enc   );
                printf("  -n  Images per rev <8,16>         [% 6i         ]\n"    , img_cycle );
                printf("  -o  read Order <BUSEQ,BUSIM,      [   %ls     ]\n"      , cam_rd    );
                printf("      COSIM,OISIM,TDSEQ,TDSIM>\n"                                     );
                printf("  -r  rotator Revolutions           [% 6i         ]\n"    , rot_revs  );
                printf("  -v  rotator Velocity (+ve,0,-ve)  [% 6.1f deg/sec ]\n"  , rot_vel   );
                printf("  -u  rotator USB device            [ %s  ]\n"            , rot_usb   );
                printf("  -t  target Temperature            [ <% 2.1f C       ]\n", cam_temp  );
                printf("  -q  Quick start <0=false,1=true>  [ %5.5s         ]\n"  , btoa(cam_quick));
                printf("  -c  Camera <1=Master, 2=Slave>    [     %i         ]\n" , mop_inst+1);
                puts("Obs:");
                printf("  -O  Obj. name                     [ %-13.13s ]\n"       , fts_obj   );
                printf("  -R  Obj. RA  <hh:mm:ss.ss>        [ %-11.11s   ]\n"     , fts_ra    );
                printf("  -D  Obj. DEC <dd.mm.ss.ss>        [ %-12.12s  ]\n"      , fts_dec   );
                printf("  -F  Focus position                [% 8.3f       ]\n"    , tel_foc   );
                printf("  -C  CAS angle                     [% 8.3f       ]\n"    , tel_cas   );
                printf("  -A  Altitude angle                [% 8.3f       ]\n"    , tel_alt   );
                printf("  -Z  aZimuth angle                 [% 8.3f       ]\n"    , tel_azm   );
                puts("Dbg:");
                printf("  -d  Level <0=CRIT,1=SYS, 2=ERR,   [     %i         ]\n" , log_level );
                printf("      3=WRN,4=IMG,5=INF,6=DBG>\n"                                     );
                printf("  -s  Single camera <1,%i or -1=All> [     %-3.3s       ]\n" , CAM_COUNT, mop_which >= 0 ? str: "All");
                printf("  -w  Write destination (exists)    [    %s/         ]\n" , fts_dir   );
                printf("  -W  Write destination (create)    [    %s/         ]\n" , fts_dir   );
                printf("  -a  static fixed Angle            [  % 2.1f deg     ]\n", rot_zero  );
                printf("  -L  Log prefix                    [ %8.8s      ]\n"     , dbg_prefix);
                printf("  -N  No filter installed           [ %5.5s         ]\n"  , btoa(tel_clear));
                printf("  -E  Enumerate options for <feature>\n");
                puts("  -h  Help");
                puts("");
                mop_exit( EXIT_SUCCESS );
                break;
            case '?': // Catch argument errors
                if ( strchr( "arnexfompbuvtqcsdhwWORDFCAZNELMS", optopt ) )
                   fprintf( stderr, "Option -%c missing argument\n", optopt );
                else if ( isprint(optopt) )
                   fprintf( stderr, "Option -%c unrecognised\n", optopt );
                else
                   fprintf( stderr, "Invalid option -0x%02x character\n", optopt );
                break;
            default: // Something really bad so give up
                mop_exit( mop_log( EXIT_FAILURE, MOP_CRIT, FAC_OPT, "Invalid run options")); 
         }
    }

    return true;
}
