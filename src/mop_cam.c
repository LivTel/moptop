/** @file   mop_cam.c
  *
  * @brief MOPTOP Andor Zyla camera access and control functions  
  *
  * @author asp 
  *
  * @date   2019-07-09 
  */

#include "moptop.h"


/** @brief     Check Andor API AT_*() function return values
  *
  * @param[in] ret = API return value to be checked
  * @param[in] fn  = Text identification of API function
  *
  * @return    true|false = success|failure
  */
/*
bool at_chk( int ret, char *fn )
{
//    usleep(10);

    if ( !ret )
        return true;
    else if ( ret >= AT_ERR_MIN && ret <= AT_ERR_MAX )
        return mop_log( false, MOP_ERR, FAC_CAM, "AT_%s fail. Ret=%i Err=%s", fn, ret, at_erray[ret - AT_ERR_MIN]);
    else if ( ret >= UT_ERR_MIN && ret <= UT_ERR_MAX )
        return mop_log( false, MOP_ERR, FAC_FTS, "AT_%s fail. Ret=%i Err=%s", fn, ret, ut_erray[ret - UT_ERR_MIN]);
    else
        return mop_log( false, MOP_ERR, FAC_FTS, "AT_%s fail. Ret=%i Err=%s", fn, ret, "ERR_UNKNOWN" );
}
*/

/** @brief     Check Andor API AT_*() function return values
  *
  * @param[in] ret = API return value to be checked
  * @param[in] fn  = Text identification of API function
  *
  * @return    true|false = success|failure
  */
bool at_chk( int ret, char *fn, AT_WC *cmd  )
{
    if ( !ret )
        return mop_log( true,  MOP_DBG, FAC_CAM, "AT_%s(%ls) OK. Ret=%i", fn, cmd, ret );
    else if ( ret >= AT_ERR_MIN && ret <= AT_ERR_MAX )
        return mop_log( false, MOP_ERR, FAC_CAM, "AT_%s(%ls) fail. Ret=%i Err=%s", fn, cmd, ret, at_erray[ret - AT_ERR_MIN]);
    else if ( ret >= UT_ERR_MIN && ret <= UT_ERR_MAX )
        return mop_log( false, MOP_ERR, FAC_FTS, "AT_%s(%ls) fail. Ret=%i Err=%s", fn, cmd, ret, ut_erray[ret - UT_ERR_MIN]);
    else
        return mop_log( false, MOP_ERR, FAC_FTS, "AT_%s(%ls) fail. Ret=%i Err=%s", fn, cmd, ret, "ERR_UNKNOWN" );
}


bool at_try( mop_cam_t *cam, void *fn, AT_WC *cmd, ...  )
{
    bool ok;

    va_list val;

    for ( int try = 1; try <= AT_TRY; try++ )
    {
        va_start( val, cmd );

/*  Yikes! This needs some explaining
 *  The function to be invoked is passed in and identified using "if" because switch/case can't be used.  
 *  Apart from AT_Open the appropriate camera handle is passed to the function.
 *  cmd is the sub-function to be invoked and is passed as a unicode string
 *  To handle variable types nay parameter is extracted using va_arg() with the appropriate type.
 *  Read strings are fixed at maximum length of 256 bytes
 *  Find final 2 string params are passed to at_chk() for printing debug info   
 */
//               Function               Function parameters                          Debug: Name &  Sub-command  
        if      ( fn == AT_Open         )         
            ok=at_chk( AT_Open         (cam->USBIndex,    &cam->Handle            ),"Open"         ,cmd);
        else if ( fn == AT_SetBool      )      
            ok=at_chk( AT_SetBool      (cam->Handle, cmd, va_arg(val,AT_BOOL )    ),"SetBool"      ,cmd);
        else if ( fn == AT_SetEnumString)
            ok=at_chk( AT_SetEnumString(cam->Handle, cmd, va_arg(val,AT_WC  *)    ),"SetEnumString",cmd);
        else if ( fn == AT_SetFloat     )     
            ok=at_chk( AT_SetFloat     (cam->Handle, cmd, va_arg(val,double  )    ),"SetFloat"     ,cmd);
        else if ( fn == AT_GetBool      )     
            ok=at_chk( AT_GetBool      (cam->Handle, cmd, va_arg(val,AT_BOOL*)    ),"GetBool"      ,cmd);
        else if ( fn == AT_GetString    )    
            ok=at_chk( AT_GetString    (cam->Handle, cmd, va_arg(val,AT_WC  *),256),"GetString"    ,cmd);
        else if ( fn == AT_GetFloat     )     
            ok=at_chk( AT_GetFloat     (cam->Handle, cmd, va_arg(val,double *)    ),"GetFloat"     ,cmd);
        else if ( fn == AT_GetInt       )      
            ok=at_chk( AT_GetInt       (cam->Handle, cmd, va_arg(val,AT_64  *)    ),"GetInt"       ,cmd);
        else if ( fn == AT_GetFloatMin  ) 
            ok=at_chk( AT_GetFloatMin  (cam->Handle, cmd, va_arg(val,double *)    ),"GetFloatMin"  ,cmd);
        else if ( fn == AT_GetFloatMax  ) 
            ok=at_chk( AT_GetFloatMax  (cam->Handle, cmd, va_arg(val,double *)    ),"GetFloatMax"  ,cmd);
        else if ( fn == AT_Flush        )        
            ok=at_chk( AT_Flush        (cam->Handle                               ),"Flush"        ,cmd);  
        else if ( fn == AT_Close        )        
            ok=at_chk( AT_Close        (cam->Handle                               ),"Close"        ,cmd);  
        else if ( fn == AT_Command      )        
            ok=at_chk( AT_Command      (cam->Handle, cmd                          ),"Command"        ,cmd);  
        else if ( fn == AT_IsReadOnly   )        
            ok=at_chk( AT_IsReadOnly   (cam->Handle, cmd, va_arg(val,AT_BOOL*)    ),"IsReadOnly"   ,cmd);  
        else if ( fn == AT_GetEnumIndex )
            ok=at_chk( AT_GetEnumIndex (cam->Handle, cmd, va_arg(val,int    *)    ),"GetEnumIndex" ,cmd); 
        else if ( fn == AT_GetEnumStringByIndex)
            ok=at_chk( AT_GetEnumStringByIndex(cam->Handle, cmd, va_arg(val, int), va_arg(val, AT_WC *), 256),"GetEnumStringByIndex", cmd);
        else
            ;

        if ( ok ) 
        {
            va_end( val );
            return true;
        } 
        else
        {
            mop_log( true, MOP_WRN, FAC_CAM, "retry %i ...", try ); 
            sleep( AT_DLY );
        }
    }

   va_end( val );
   return false;
}


/** @brief     Per camera initialisation 
  *
  * @param[in] Index to camera info array
  * 
  * @return    true|false = success|failure
  */
bool cam_init( int i )
{
//  Initialise camera defaults that are not easily settable in mopdat.h    
    memset( mop_cam, 0, sizeof(mop_cam));
    mop_cam[i].id       = fts_id[i];
    mop_cam[i].USBIndex = i;
    mop_cam[i].Handle   = AT_HANDLE_UNINITIALISED;

//  Init. Andor camera and conversion utility libraries
    return (at_chk( AT_InitialiseLibrary(),       "InitialiseLibrary"       , L"")&&
            at_chk( AT_InitialiseUtilityLibrary(),"InitialiseUtilityLibrary", L"")  );
}


/** @brief     Tidy-up camera data before exiting process  
  *
  * @param[in] Pointer to camera info structure 
  *
  * @return    true|false = success|failure
  */
bool cam_close( mop_cam_t *cam )
{
    if ( cam->Handle )
    {
//      Inhibit image acquisition
        cam_acq_ena( cam, AT_FALSE   );
        cam_trg_set( cam, CAM_TRG_SW );

//      Flush and close camera 
        at_try(cam, AT_Flush, L"", NULL );
        at_try(cam, AT_Close, L"", NULL );
    }

//  Release output buffer memory 
    if ( img_mono16 )
    {
        free( img_mono16 );
        img_mono16 = NULL;
    }

//  Release circular buffer memory 
    for ( int i = 0; i < MAX_CYCLE; i++ )
        if ( cam->ImageBuffer[i] )
        { 
           free( cam->ImageBuffer[i] );
           cam->ImageBuffer[i] = NULL;
        }

//  Close AT libraries
    return at_chk( AT_FinaliseLibrary()       ,"FinaliseLibrary"       , L"")&&
           at_chk( AT_FinaliseUtilityLibrary(),"FinaliseUtilityLibrary", L"")  ;
}


/** @brief     Ensure cooling is enabled. Waits for temperature to stabilise below target
  *
  * @param[in] Pointer to camera info structure
  * @param[in] Target temperature [deg C] to reach before returning
  * @param[in] Timeout [sec]
  * @param[in] true = fast cooling avoiding stabilisation, false = wait for it to report as stable 
  *
  * @return    true|false = success|failure
  */
bool cam_stable( mop_cam_t *cam, double TargetTemperature, int timeout, bool fast ) 
{
    int TempStatusIndex;
    
//  Wait for camera to report stable temperature
    if ( !fast )
        do 
        {
            sleep(1);
            at_try( cam, AT_GetFloat            ,L"SensorTemperature", &cam->SensorTemperature );
            at_try( cam, AT_GetEnumIndex        ,L"TemperatureStatus", &TempStatusIndex        );
            at_try( cam, AT_GetEnumStringByIndex,L"TemperatureStatus", cam->TemperatureStatus, TempStatusIndex        );
            mop_log( true, MOP_INF, FAC_CAM, "Thermal=%ls T=%-6.2fC" , cam->TemperatureStatus, cam->SensorTemperature );
        } while( wcscmp( L"Stabilised", cam->TemperatureStatus) && ( timeout-- > 0 ) );

//  Wait for target temperature to be reached
    do 
    {
        sleep(1);
        at_try( cam, AT_GetFloat            ,L"SensorTemperature", &cam->SensorTemperature );
        at_try( cam, AT_GetEnumIndex        ,L"TemperatureStatus", &TempStatusIndex        );
        at_try( cam, AT_GetEnumStringByIndex,L"TemperatureStatus", cam->TemperatureStatus, TempStatusIndex        );
        mop_log( true, MOP_INF, FAC_CAM, "Thermal=%ls T=%-6.2fC" , cam->TemperatureStatus, cam->SensorTemperature );
    } while ( ( cam->SensorTemperature > TargetTemperature ) && ( timeout-- > 0 ) ); 

//  Was target temperature reached within timeout? 
    if ( timeout <= 0 )
    	return mop_log( false, MOP_WRN, FAC_CAM, "Cooling Timeout" );
    else
        return mop_log( true,  MOP_INF, FAC_CAM, "Thermal=%ls T=%-6.2fC < %-6.2f", 
                        cam->TemperatureStatus, cam->SensorTemperature, TargetTemperature );
}


/** @brief     Debug: Checks camera parameter readonly status    
  *
  * @param[in] Pointer to camera info structure 
  *
  * @return    true 
  */
bool cam_chk( mop_cam_t *cam )
{
    AT_BOOL ReadOnly;
    
    at_try( cam, AT_IsReadOnly, L"Framecount", &ReadOnly );
    mop_log( ReadOnly, MOP_DBG, FAC_CAM, "FrameCount Readonly=%i", ReadOnly );

    at_try( cam, AT_IsReadOnly, L"AccumulateCount", &ReadOnly );
    mop_log( ReadOnly, MOP_DBG, FAC_CAM, "AccumulateCount Readonly=%i", ReadOnly );

    return true;
}


/** @brief     Setup the camera
  *
  * @param[in] Pointer to camera structure
  * @param[in] Exposure time [seconds]
  *
  * @return    true|false = success|failure
  */
bool cam_open( mop_cam_t *cam, double exp )
{
     cam->Handle = AT_HANDLE_UNINITIALISED;

     if (at_try(cam,(void*)AT_Open         ,L""                         ,&cam->Handle                 )&&
         at_try(cam,(void*)AT_SetBool      ,L"SensorCooling"            ,AT_TRUE                      )&&
         at_try(cam,(void*)AT_SetBool      ,L"MetadataEnable"           ,AT_TRUE                      )&&
         at_try(cam,(void*)AT_SetBool      ,L"MetadataTimestamp"        ,AT_TRUE                      )&&
         at_try(cam,(void*)AT_SetBool      ,L"SpuriousNoiseFilter"      ,AT_FALSE                     )&& 
         at_try(cam,(void*)AT_SetBool      ,L"StaticBlemishCorrection"  ,AT_FALSE                     )&&
         at_try(cam,(void*)AT_SetBool      ,L"RollingShutterGlobalClear",AT_TRUE                      )&&
         at_try(cam,(void*)AT_SetEnumString,L"ElectronicShutteringMode" ,L"Rolling"                   )&&
         at_try(cam,(void*)AT_SetEnumString,L"SensorReadoutMode"        ,cam_rd                       )&&
         at_try(cam,(void*)AT_SetEnumString,L"SimplePreAmpGainControl"  ,cam_amp                      )&&
         at_try(cam,(void*)AT_SetEnumString,L"PixelEncoding"            ,cam_enc                      )&&
         at_try(cam,(void*)AT_SetEnumString,L"PixelReadoutRate"         ,cam_mhz                      )&&
         at_try(cam,(void*)AT_SetEnumString,L"CycleMode"                ,L"Continuous"                )&&
         at_try(cam,(void*)AT_SetEnumString,L"AOIBinning"               ,cam_bin                      )&&
         at_try(cam,(void*)AT_SetEnumString,L"TriggerMode"              ,cam_trg                      )&&
         at_try(cam,(void*)AT_SetFloat     ,L"ExposureTime"             ,exp                          )&&
         at_try(cam,(void*)AT_GetBool      ,L"FullAOIControl"           ,&cam->FullAOIControl         )&&
         at_try(cam,(void*)AT_GetString    ,L"SerialNumber"             ,cam->SerialNumber            )&&
         at_try(cam,(void*)AT_GetString    ,L"FirmwareVersion"          ,cam->FirmwareVersion         )&&
         at_try(cam,(void*)AT_GetFloat     ,L"ExposureTime"             ,&cam->ExpVal                 )&&
         at_try(cam,(void*)AT_GetFloat     ,L"ReadoutTime"              ,&cam->ReadoutTime            )&&
         at_try(cam,(void*)AT_GetFloat     ,L"BytesPerPixel"            ,&cam->BytesPerPixel          )&&
         at_try(cam,(void*)AT_GetFloat     ,L"PixelWidth"               ,&cam->PixelWidth             )&&
         at_try(cam,(void*)AT_GetFloat     ,L"PixelHeight"              ,&cam->PixelHeight            )&&
         at_try(cam,(void*)AT_GetInt       ,L"SensorWidth"              ,&cam->SensorWidth            )&&
         at_try(cam,(void*)AT_GetInt       ,L"SensorHeight"             ,&cam->SensorHeight           )&&
         at_try(cam,(void*)AT_GetInt       ,L"TimestampClockFrequency"  ,&cam->TimestampClockFrequency)&&
         at_try(cam,(void*)AT_GetInt       ,L"ImageSizeBytes"           ,&cam->ImageSizeBytes         )&&
         at_try(cam,(void*)AT_GetFloatMin  ,L"ExposureTime"             ,&cam->ExpMin                 )&&
         at_try(cam,(void*)AT_GetFloatMax  ,L"ExposureTime"             ,&cam->ExpMax                 )&&
         at_try(cam,(void*)AT_Flush        ,L""                         ,NULL                         )  )
     {              
//   If automatic exposure then evaluate a time for current camera settings
         if ( cam_auto &&  // Use auto exposure 
                 rot_sign   ) // and not static
         {
             cam_exp = exp = fabs((rot_stp / rot_vel)) - 1.5 * cam->ReadoutTime;
             at_try(cam, AT_SetFloat, L"ExposureTime", exp         );
             at_try(cam, AT_GetFloat, L"ExposureTime", &cam->ExpVal);
             mop_log( true, MOP_INF, FAC_CAM, "Automatic exposure = %fs", cam->ExpVal ); 
         }
     
//       Re-get actual exposure in case it was changed in auto mode
         at_try(cam, AT_GetFloat, L"ExposureTime", &exp);
     
//       Get constants for this camera
         cam_param( cam );
         cam->Dimension[IMG_WIDTH]  = (int)cam->SensorWidth/img_bin;
         cam->Dimension[IMG_HEIGHT] = (int)cam->SensorHeight/img_bin;
     
//       Exposure info
         cam->ExpReq = exp;                       // Requested exposure time
         cam->ExpDif = cam->ExpReq - cam->ExpVal; // Difference from actual
     
//       Padding space added to line up camera info output 
         mop_log( true, MOP_INF, FAC_CAM, 
                           "Ser. No.  = %ls"
                 LOG_BLANK "Model     = %ls"
                 LOG_BLANK "Firmware  = %ls"
                 LOG_BLANK "Trig. Mode= %ls"
                 LOG_BLANK "Encoding  = %ls"
                 LOG_BLANK "Read Rate = %ls"
                 LOG_BLANK "Amp. Mode = %ls"
                 LOG_BLANK "Gain      = %f e/ADU"
                 LOG_BLANK "Well Depth= %i e"
                 LOG_BLANK "Dark Curr.= %f e/px/s"
                 LOG_BLANK "Binning   = %ls"
                 LOG_BLANK "FullAOICtl= %s",
                 cam->SerialNumber, cam->Model, cam->FirmwareVersion, 
                 cam_trg, cam_enc, cam_mhz, cam_amp, 
                 cam->Gain, cam->WellDepth, cam->DarkCurrent, cam_bin, btoa(cam->FullAOIControl) );
	 return mop_log( true, MOP_INF, FAC_CAM, 
                         "ImageSizeBytes=0x%X"
                         LOG_BLANK "ReadoutTime   =%.4fs" 
                         LOG_BLANK "ExpTime       =%.4fs" 
                         LOG_BLANK "ExpMax        =%.4fs" 
                         LOG_BLANK "ExpMin        =%.5fs"
                         LOG_BLANK "Exp. per rot. =%i"
                         LOG_BLANK "Exp. Total    =%i",
	                 cam->ImageSizeBytes, cam->ReadoutTime, cam->ExpVal, cam->ExpMax, cam->ExpMin, img_cycle, img_total ); 
    }
    return mop_log( false, MOP_ERR, FAC_CAM, "cam_open()" ); 
}

/** @brief     Get camera data sheet parameters based on serial number 
  *
  * @param[in] Pointer to camera structure
  *
  * @return    true|false = success|failure
  */
bool cam_param( mop_cam_t *cam )
{
   int c; // Index by camera
   int m; // Index by MHz to get gain

   for ( c = 0; c < CAM_COUNT; c++ )
       if ( !wcscmp( cam_info[c].SerialNumber, cam->SerialNumber ))
       {
//         Easy stuff from look-up table first
           cam->Model       = cam_info[c].Model;
           cam->WellDepth   = cam_info[c].WellDepth;
           cam->DarkCurrent = cam_info[c].DarkCurrent;
           cam->Filter      = cam_info[c].Filter;
           cam->FilterID    = cam_info[c].FilterID;

//         What's the frequency Kenneth? 
           if      (!wcscmp( CAM_MHZ_100, cam_mhz ))
               m = IDX_MHZ_100;
           else if (!wcscmp( CAM_MHZ_270, cam_mhz ))
               m = IDX_MHZ_270;
           else
               return mop_log( false, MOP_ERR, FAC_CAM, "cam_param(). Unsupported read-rate." ); 

//         Get the gain and noise for the selected read rate
           if      (!wcscmp( CAM_AMP_16L, cam_amp ))
           {
               cam->Gain  = cam_info[c].mhz[m].gain [IDX_AMP_16L];
               cam->Noise = cam_info[c].mhz[m].noise[IDX_AMP_16L];
           }
           else if (!wcscmp( CAM_AMP_12L, cam_amp ))
           {
               cam->Gain  = cam_info[c].mhz[m].gain [IDX_AMP_12L];
               cam->Noise = cam_info[c].mhz[m].noise[IDX_AMP_12L];
           }
           else if (!wcscmp( CAM_AMP_12H, cam_amp ))
           {
               cam->Gain  = cam_info[c].mhz[m].gain [IDX_AMP_12H];
               cam->Noise = cam_info[c].mhz[m].noise[IDX_AMP_12H];
           }
           else 
           {
               return mop_log( false, MOP_ERR, FAC_CAM, "cam_param(). Unsupported pre-amp gain" ); 
           }

           if ( cam_info[c].FtsIndex != cam->USBIndex )
               return mop_log( true, MOP_WRN, FAC_CAM, "cam_param(). Wrong camera index. Swap USB leads" ); 
           else
               return true;
       }

    return mop_log( false, MOP_ERR, FAC_CAM, "cam_param(). Unrecognised camera=%ls", cam->SerialNumber ); 
}


/** @brief     Queue up exposure buffers 
  *
  * @param[in] Pointer to camera info structure
  *
  * @return    true|false = success|failure
  */
bool cam_queue( mop_cam_t *cam )
{
    int b = 0;

    for ( int i = 0; i < img_total; i++ )
    { 
        if ( !at_chk( AT_QueueBuffer( cam->Handle, cam->ImageBuffer[b], cam->ImageSizeBytes), "QueueBuffer", L""))
            return mop_log( false, MOP_ERR, FAC_CAM, "cam_queue()" );

        if ( ++b >= IMG_CYCLE )
             b = 0;       
    }
    return true;
}


/** @brief     Allocate memory blocks for image storage
  *
  * @param[in] Pointer to camera info structure
  * @param[in] Allocation type, Fragmented or Monolithic 
  *
  * @return    true|false = success|failure
  */
bool cam_alloc( mop_cam_t *cam )
{
//  Allocate a conversion buffer in case input is 12-bit. 
//  Output will be 16-bit mono so double buffer size   
//  img_mono16size = cam->SensorWidth * cam->SensorHeight;
    img_mono16size = (cam->SensorWidth/img_bin) * (cam->SensorHeight/img_bin);
    if ( !( img_mono16 = aligned_alloc( 16,  2 * img_mono16size )))
        return mop_log( false, MOP_SYS, FAC_CAM, "Mono16 image aligned_alloc()" );

    for ( int i = 0; i < IMG_CYCLE; i++ )
        if ( !(cam->ImageBuffer[i] = aligned_alloc( 16, cam->ImageSizeBytes )))
           return mop_log( false, MOP_SYS, FAC_CAM, "aligned_alloc(CIRC)" );

    return true;
}


/** @brief     Image acquisition - circular frame buffer
  *
  * @param[in] Camera ID
  * @param[in] File type 
  *
  * @return    true|false = success|failure
  */
bool cam_acq_circ( mop_cam_t *cam, char pfx )
{
    int    b = 0;              // Image buffer
    double rot_req = rot_zero; // Requested rotator angle
    double clk_dif;            // Camera timestamp clock difference
    double rot_now;
    double timeout = TIM_MILLISECOND * cam->ExpVal + TMO_XFER;

//  If bias frame then use minimum exposure else restore global value
    if ( fts_pfx == FTS_PFX_BIAS )
    {
        cam->ExpVal = cam->ExpMin;
        at_try(cam, AT_SetFloat, L"ExposureTime", cam->ExpMin );
    }
    else
    {
        cam->ExpVal = cam_exp; 
        at_try(cam, AT_SetFloat, L"ExposureTime", cam_exp );
    }

//  Loop to acquire all images
    for ( int i = 0; i < img_total; i++ )
    {
        gettimeofday(&cam->ObsStart, NULL);

        cam->RotReq[i] = rot_req;
        cam->RotAng[i] = fmod( rot_req, 360.0 );
        cam->RotN[i]   = 1 + (i / img_cycle);
        cam->SeqN[i]   = 1 + (i % img_cycle);

//      Wait for image buffers to be filled.
        if ( !at_chk( AT_WaitBuffer( cam->Handle, &cam->ReturnBuffer[b], &cam->ReturnSize[i], timeout ),"WaitBuffer",L""))
            mop_exit( mop_log( false, MOP_ERR, FAC_CAM, "Missed image %i. Exiting", i+1 ));

        if ( mop_master )
        {
//          Get final angle and length of arc for this exposure
            rot_get( &rot_now );
            cam->RotDif[i] = rot_now - rot_req;
            cam->RotEnd[i] = fmod( rot_now, 360.0 );
        }
        else // Slave does not have access to rotator postion
        {
//          Fake final angle and length of arc
            cam->RotDif[i] = rot_stp;
            cam->RotEnd[i] = fmod(rot_req + rot_stp, 360.0 );
        }

        cam->TimestampClock[i] = cam_ticks( cam, b );
        if (i)
            clk_dif = (double)(cam->TimestampClock[i] - cam->TimestampClock[i-1]) / cam->TimestampClockFrequency;
        else
            clk_dif = (double)cam->TimestampClock[i] / cam->TimestampClockFrequency;

//      Write to file 
        gettimeofday(&cam->ObsEnd, NULL);
        fts_write( fts_mkname( cam, fts_pfx ), cam, i, b );

//      Logging
        mop_log( true, MOP_IMG, FAC_CAM,
                "Exp %2.2i %-2.2i %f Rot %9.2f %7.2f %7.2f Dif %6.2f %6.4f %5.4f",
                 cam->RotN[i],   cam->SeqN[i],   cam->ExpVal, cam->RotReq[i], cam->RotAng[i],
                 cam->RotEnd[i], cam->RotDif[i], fabs(cam->RotDif[i]/rot_vel - rot_sign*cam->ExpVal), clk_dif);

        rot_req += rot_stp;
        if ( ++b >= MAX_CYCLE )
            b = 0; // Loop circular buffer back to start 
    }

//  Stop acquisition, get temperature and don't forget to flush
    cam_acq_ena( cam, AT_FALSE   );
    cam_trg_set( cam, CAM_TRG_SW );
    at_try( cam, AT_GetFloat,  L"SensorTemperature", &cam->SensorTemperature );
    at_try( cam, AT_Flush   ,  L"", NULL );

    return true;
}

/** @brief     Start image acquisition - STATIC
  *
  * @param[in] Camera ID 
  * @param[in] File type 
  *
  * @return    true|false = success|failure
  */
bool cam_acq_stat( mop_cam_t *cam, char pfx )
{
    int    b = 0;              // Image buffer
    double rot_req = rot_zero; // Requested rotator angle. Default zero position
    double clk_dif;            // Camera timestamp clock difference
    double rot_now;
    double timeout = TIM_MILLISECOND * cam->ExpVal + TMO_XFER;

//  If bias frame then use minimum exposure else restore global value
    if ( fts_pfx == FTS_PFX_BIAS )
    {
        cam->ExpVal = cam->ExpMin;
        at_try(cam, AT_SetFloat, L"ExposureTime", cam->ExpMin );
    }
    else
    {
        cam->ExpVal = cam_exp; 
        at_try(cam, AT_SetFloat, L"ExposureTime", cam_exp );
    }

//  Loop to acquire images
    for ( int i = 0; i < img_total; i++ )
    {
        gettimeofday(&cam->ObsStart, NULL);

        cam->RotReq[i] = rot_req;                 // Absolute rotation
        cam->RotAng[i] = fmod( rot_req, 360.0 );  // 0-360 rotation
        cam->RotN[i]   = 1 + (i / img_cycle);     // Rotation number
        cam->SeqN[i]   = 1 + (i % img_cycle);     // Position within rotation

        if ( mop_master ) // Master process
        {
            if ( !rot_goto( rot_req, TMO_ROTATE, &cam->RotEnd[i] )||
                 !utl_shm_post( SHM_TRIG, CAM2                   )  )
                exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_CAM, "cam_acq_stat() Exiting" ));
            mop_log( true, MOP_DBG, FAC_CAM, "Master trigger posted" );
        }
        else // Slave process 
        {
            cam->RotEnd[i] = rot_req;
            if ( !utl_shm_wait( SHM_TRIG, CAM2, TMO_TRIG, true ))
                exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_CAM, "cam_acq_stat() timeout. Exiting" ));
            mop_log( true, MOP_DBG, FAC_CAM, "Slave trigger received" );
        }

        at_try( cam, AT_Command, L"SoftwareTrigger", NULL );
        if (!at_chk( AT_WaitBuffer(cam->Handle, &cam->ReturnBuffer[b], &cam->ReturnSize[i], timeout),"WaitBuffer",L""))
            exit( mop_log( EXIT_FAILURE, MOP_ERR, FAC_CAM, "Missed image %i. Exiting", i+1 ));

        cam->RotEnd[i] = fmod( cam->RotEnd[i], 360.0 );
        cam->RotDif[i] = cam->RotEnd[i] - cam->RotAng[i];

        cam->TimestampClock[i] = cam_ticks( cam, b );
        if (i)
            clk_dif = (double)(cam->TimestampClock[i] - cam->TimestampClock[i-1]) / cam->TimestampClockFrequency;
        else
            clk_dif = (double)cam->TimestampClock[i] / cam->TimestampClockFrequency;

//      Write to file 
        gettimeofday(&cam->ObsEnd, NULL);
        fts_write( fts_mkname( cam, fts_pfx ), cam, i, b );

//      Logging
        mop_log( true, MOP_IMG, FAC_CAM,
                "Exp %2.2i %-2.2i %f Rot %9.2f %7.2f %7.2f Dif %6.2f %6.4f %5.4f",
                 cam->RotN[i],   cam->SeqN[i],   cam->ExpVal, cam->RotReq[i], cam->RotAng[i],
                 cam->RotEnd[i], cam->RotDif[i], fabs(cam->RotDif[i]/rot_vel - rot_sign*cam->ExpVal), clk_dif);

        rot_req += rot_stp;
        if ( ++b >= MAX_CYCLE )
            b = 0; // Loop circular buffer back to start 
    }

//  Stop acquisition, get temperature and don't forget to flush   
    at_try( cam, AT_Command,  L"AcquisitionStop", NULL);
    at_try( cam, AT_GetFloat, L"SensorTemperature", &cam->SensorTemperature);
    at_try( cam, AT_Flush,    L"", NULL);

    return true;
}



/** @brief     Read camera ticks from image meta-data
  *            Meta-data reporting must be enabled  
  *
  * @param[in] Pointer to camera info structure
  * @param[in] Image number 
  *
  * @return    Image size or 0=failure  
  */
unsigned long cam_ticks( mop_cam_t *cam, int buf )
{
    int i;

    unsigned char *ptr1;
    unsigned int len;
    unsigned int cid;

    ptr1 = cam->ImageBuffer[buf] + cam->ImageSizeBytes;

// DEBUG:  Used for per-byte verify of meta-data structure    
//    for( i = 0; i < 40; i++ )
//      printf( "%i=%i\n", i, *--ptr0 );

//  Handles Andor's tacked-on meta-data, see Sec 4.5 METADATA in SDK 3.11 for gruesome details
//  Parse backwards from the end of image 
    for( i = 0; i < 3; i++ )
    {
//      Move back and read int length field
        ptr1 -= 4;
        len = *(unsigned int *)ptr1;

//      Move back and read int CID field        
        ptr1 -= 4;
        cid = *(unsigned int *)ptr1;

//      Move back and point to data by subtracting CID field length     
        ptr1 -= (len - 4);
        switch ( cid )
        {
            case 0:
// DEBUG         printf( "cid=%i len=%i\n", cid, len-4 );
                 break;
            case 1:
// DEBUG         printf( "cid=%i ticks=%lu\n", cid, *(unsigned long*)ptr1 );
                 return *(unsigned long*)ptr1;
                 break;
            case 7:
// DEBUG         printf( "cid=%i\n", cid );
                 break;

            default:
                 break;
        }
    }

    return 0;
}


/** @brief     Show implementation state of an enumerated camera feature 
  *
  * @param[in] Pointer to camera info structure 
  * @param[in] Enumerate feature name 
  * 
  * @return    void
  */
void cam_feature( mop_cam_t *cam, AT_WC *Feature )
{	
    int i;
    int Count;
    AT_WC String[256];
    AT_BOOL Available;
    AT_BOOL Implemented;
    
    at_chk( AT_GetEnumCount( cam->Handle, Feature, &Count ),"GetEnumCount()",L"");

//  Print out what is available and implemented    
    puts( "Avail. Impl." );
    for ( i = 0; i < Count; i++ )
    {
        at_chk(AT_GetEnumStringByIndex  (cam->Handle,Feature,i,String, 256 ),"GetEnumStringByIndex"  ,L"");
        at_chk(AT_IsEnumIndexAvailable  (cam->Handle,Feature,i,&Available  ),"IsEnumIndexAvailable"  ,L"");
        at_chk(AT_IsEnumIndexImplemented(cam->Handle,Feature,i,&Implemented),"IsEnumIndexImplemented",L"");
        printf( "%-5s  %-5s  %ls\n", btoa(Available), btoa(Implemented), String );
    }

    at_chk(AT_GetEnumIndex(cam->Handle, Feature, &i ),"SetEnumIndex",L"" );
    at_chk(AT_GetEnumStringByIndex(cam->Handle,Feature,i,String, 256 ),"GetEnumStringByIndex",L"");
    printf( "\nActual = %ls\n", String );
}


/** @brief     Camera Acquisition Enable/disable      
  *
  * @param[in] Pointer to camera info structure 
  * @param[in] AT_TRUE | AT_FALSE  Enable/disable 
  * 
  * @return    true | false = Success | Failure 
  */
bool cam_acq_ena( mop_cam_t *cam, AT_BOOL new )
{
    AT_BOOL now;

    at_try( cam, AT_GetBool, L"CameraAcquiring", &now ); 
    if ( now != new )
    {
        if ( new == AT_TRUE ) 
        {
           if (at_try( cam, AT_Command, L"AcquisitionStart", NULL))
               return mop_log( true , MOP_INF, FAC_CAM, "Command(AcquisitionStart)");
           else
               return mop_log( false, MOP_ERR, FAC_CAM, "Command(AcquisitionStart)");
        }
        else 
        {
            if (at_try( cam, AT_Command, L"AcquisitionStop", NULL))
                return mop_log( true , MOP_INF, FAC_CAM, "Command(AcquisitionStop)");
            else
                return mop_log( false, MOP_ERR, FAC_CAM, "Command(AcquisitionStop)");
        }
    }

    return true;
}


/** @brief     Camera Acquisition Enable/disable      
  *
  * @param[in] Pointer to camera info structure 
  * @param[in] Camera trigger mode 
  * 
  * @return    true | false - Success | Failure 
  */
bool cam_trg_set( mop_cam_t *cam, AT_WC *trg )
{
   return at_try( cam, AT_SetEnumString, L"TriggerMode", trg );
}


/** @brief     Camera internal clock reset       
  *
  * @param[in] Pointer to camera info structure 
  * 
  * @return    true | false - Success | Failure 
  */
bool cam_clk_rst( mop_cam_t *cam )
{
   return at_try( cam, AT_Command, L"TimestampClockReset", NULL); 	
}
