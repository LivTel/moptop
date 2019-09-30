/** @file  mop_fts.c
  *
  * @brief MOPTOP output FITS file functions  
  * 
  * File name format is CAM_e_YYYYMMDD_RUN_0_SEQ_0.fits where,
  *   CAM      = single character camera identifier = 1,2
  *   YYYYMMDD = ISO format Julian date 
  *   RUN      = Run number starting at 1
  *   SEQ      = Sequence number (==rotation angle) starting at 1
  *
  * E.g.  d_e_20190130_4_0_8.fits
  *
  * @author asp 
  *
  * @date   2019-05-01 
  */

#include "moptop.h"

// Module common data used by dirent() function
char  fts_file_str[128];
char  fts_file_pfx[128]; 

/**
  * @brief      File selection function called by scandir() from within fts_mkname()
  *
  * @param[out] entry = structure holding found file names 
  *
  * return      true|false indicating if file name matches selection criteria
  */
int fts_selname(const struct dirent *entry )
{   
    return !strncmp( entry->d_name, fts_file_pfx, strlen(fts_file_pfx))&& // Starts with "1_"   
            strstr ( entry->d_name, fts_file_str                      )&& // has today's ISO date  
            strstr ( entry->d_name, FTS_SFX                           );  // and ends with "_0.fits"
}


/** @brief     Generate a full filespec name for the destination file
  *            Checks existing files to evaluate run and sequence numbers
  *            Called mulitple time so uses static data 
  * 
  * @param[in] cam = pointer to camera data structure
  *
  * @return    pointer to the generated filename 
  */
char *fts_mkname( mop_cam_t *cam, char pfx )
{
    int     i; 
    time_t  tim;          // Unix time, since 1970-01-01
    const struct tm *now; // Broken down time
    struct dirent **fts_files;

//  Data retained between calls 
    static bool first = true; 
    static char filename[256]; 
    static int  day;
    static int  mon;
    static int  year;
    static int  hour;
    static int  run;

    int c; // Camera number 
    int r; // Possible run number  

    // If not first call generate the next filename using saved values 
    // The 1+, -1 corrects modulus arithmetic as sequences start at 1 (rather than 0)
    if ( ! first )
    {
        sprintf( filename, "%s/%c_%c_%04i%02i%02i_%i_%i_%i_0.fits",
                 fts_dir, cam->id, pfx, year, mon, day, run, 1+(cam->seq)/img_cycle, 1+(cam->seq)%img_cycle );
        cam->seq++;

        return filename;
    } 

//  First call so clear flag and init. sequence number       
    first    = false;    
    cam->seq = 0;

//  Get time now and convert to broken-down numeric fields    
    if ( time(&tim) == (time_t) -1 )
        mop_log( false, MOP_SYS, FAC_FTS, "time()" );
    now = localtime(&tim);
    day  = now->tm_mday;
    mon  = now->tm_mon  + 1;
    year = now->tm_year + 1900;
    hour = now->tm_hour;

//  32-bit Unix time rolls-over in 2038. So has it been fixed yet?      
    if (year>2038)
        mop_log( true, MOP_WRN, FAC_FTS, "Year > 2038 unix bug?");

//  MJD starts at mid-day so adjust day count
    if (hour <= 11)
        day--;

//  Check if day rolls back into previous month and adjust accordingly
    if (!day)
    {
        mon--;
//            Sep         Apr         Jun         Nov
        if ( (mon==9) || (mon==4) || (mon==6) || (mon==11) )
            day=30;
	else if ( mon==2 )
            day = year % 4 ? 28 : 29; // Leap year check
	else
            day = 31;
    }

//  Check if month was adjusted back into previous year     
    if (!mon)
    {
        year--;
        mon=12;
        day=31;
    }

//  Generate common data to be used by fts_selname() in the scandir() call  
    sprintf( fts_file_str, "_%04i%02i%02i_", year, mon, day);

//  Search existing files for next available run number 
    for ( run = 1, c = 1; c <= CAM_COUNT; c++ )
    {
        sprintf( fts_file_pfx, FTS_PFX, c );
        i = scandir( fts_dir, &fts_files, fts_selname, versionsort );
        if ( i ) 
        {
           r = atoi( fts_files[i-1]->d_name+13 );
           run = r >= run ? r+1: run;  
        } 
//     Release allocated memory    
       while ( i-- )
           free( fts_files[i] );
    }

    return NULL;
}


/** @brief      Write FITS file  
  *
  * @param[out] filename = pointer to returned filespec + filename 
  * @param[in]  cam      = pointer to cameras data structure 
  * @param[in]  seq      = image sequence number 
  * @param[in]  buf      = which buffer has image
  * return     true|false = success/failure
  */
bool fts_write( char *filename, mop_cam_t *cam, int seq, int buf )
{
#define STR_LEN 127

    char  t[80]; // Time string
    char  d[80]; // Date string
    char dt[80]; // Date:time string
    char  s[80]; // usec string 
    int   len;   // String length	
    double duration;

//  Fixed/fake/default FITS header values 
    int bitpix      = 16;
    int naxis       = 2;
    int prescan     = 0;
    int postscan    = 0;
    int wavshort    = 4200; // Angstrom
    int wavlong     = 6800; // Angstrom
    double bzero    = 0.0;
    double bscale   = 1.0;
    double equinox  = 2000.0;
    double ccdxpixe = cam->PixelWidth  / 1000000.0;    // metres
    double ccdypixe = cam->PixelHeight / 1000000.0;    // metres
    double ccdatemp = cam->SensorTemperature + 273.15; // K 
    double mjd      = (time(NULL)/86400.0) + 40587;

//  For converting wide char strings to ASCII. +1 ensures space for \nul 
    char det_model[STR_LEN + 1];
    char det_serno[STR_LEN + 1];
    char det_encod[STR_LEN + 1];
    char det_rate [STR_LEN + 1];
    char det_amp  [STR_LEN + 1];
    char trigger  [STR_LEN + 1];
    char det_rd   [STR_LEN + 1];

    static int   stat;     // FITS status value
    static char  text[80]; // FITS status text

    fitsfile *fp;

    const struct tm *tim; 

//  Convert those cursed wide-chars into proper ASCII 
    wcstombs( det_model, cam->Model, STR_LEN );
    wcstombs( det_serno, cam->SerialNumber, STR_LEN );
    wcstombs( det_encod, cam_enc, STR_LEN );
    wcstombs( det_rate,  cam_mhz, STR_LEN );
    wcstombs( det_amp,   cam_amp, STR_LEN );
    wcstombs( trigger,   cam_trg, STR_LEN );
    wcstombs( det_rd,    cam_rd,  STR_LEN );

//  Create file and image
    fits_create_file( &fp, filename, &stat);
    if (stat)
    {
        fits_get_errstatus(stat, text);
        mop_log( false, MOP_ERR, FAC_FTS, "fits_create_file(%s) status=%i=%s", filename, stat, text );
    }

    fits_create_img(fp, USHORT_IMG, IMG_DIMENSIONS, cam->Dimension, &stat);
    if (stat)
    {
        fits_get_errstatus(stat, text);
        mop_log( false, MOP_ERR, FAC_FTS, "fits_create_img() status=%i=%s", stat, text );
    }
    else
    {
//        mop_log( true,  MOP_DBG, FAC_CAM, "Opened file %s", filename );
    }

//  Generate start date/time strings    
    tim=localtime(&cam->ObsStart.tv_sec);
    len=strftime( t, sizeof(t)-1, "%H:%M:%S", tim); // Time
    strftime( d, sizeof( d)-1, "%Y-%m-%d",    tim); // Date
    snprintf( dt,sizeof(dt)-1, "%sT%s", d, t );     // Date & time in ISO format (but no fractional seconds)
    snprintf( s, sizeof( s)-1, "%.3f", cam->ObsStart.tv_usec/TIM_MICROSECOND ); // Get fractional sec
    strncat( &dt[len], &s[1], sizeof(dt)-1 - len);  // Append fraction omitting decimal point

//  Fake obs. info
    fits_write_key(fp, TLOGICAL,"SIMPLE  ","T"            ,"FITS standard file"                  ,&stat);
    fits_write_key(fp, TINT    ,"BITPIX  ",&bitpix        ,"Bits per pixel"                      ,&stat);
    fits_write_key(fp, TINT    ,"NAXIS   ",&naxis         ,"Number of image axes"                ,&stat);
    fits_write_key(fp, TINT    ,"NAXIS1  ",&cam->Dimension[IMG_WIDTH] ,"Axis 1: number of pixels",&stat);
    fits_write_key(fp, TINT    ,"NAXIS2  ",&cam->Dimension[IMG_HEIGHT],"Axis 2: number of pixels",&stat);
    fits_write_key(fp, TLOGICAL,"EXTEND  ","T"            ,"May contain FITS extensions"         ,&stat);

//  Obs. info
    fits_write_key(fp, TSTRING ,"OBSTYPE ",fts_typ        ,"Type of observation",&stat);
    fits_write_key(fp, TSTRING ,"ORIGIN  ","Liverpool JMU","Liverpool Telescope",&stat);
    fits_write_key(fp, TSTRING ,"INSTRUME","MOPTOP"       ,"Instrument name"    ,&stat);

//  Default is with filter but if removed use the -N option
    if ( tel_clear )
    {
        fits_write_key(fp, TSTRING ,"FILTER1 ","None"     ,"Filter "            ,&stat);
        fits_write_key(fp, TSTRING ,"FILTERI1","None"     ,"Filter ID"          ,&stat);
    }
    else
    {
        fits_write_key(fp, TSTRING ,"FILTER1 ",cam->Filter    ,"Filter "            ,&stat);
        fits_write_key(fp, TSTRING ,"FILTERI1",cam->FilterID  ,"Filter ID"          ,&stat);
    }

    fits_write_key(fp, TINT    ,"PRESCAN ",&prescan       ,""                   ,&stat);
    fits_write_key(fp, TINT    ,"POSTSCAN",&postscan      ,""                   ,&stat);
    fits_write_key(fp, TINT    ,"WAVSHORT",&wavshort      ,"[angstrom] Min. wavelength",&stat);
    fits_write_key(fp, TINT    ,"WAVLONG ",&wavlong       ,"[angstrom] Max. wavelength",&stat);
    fits_write_key(fp, TINT    ,"RUNNUM  ",&cam->RotN[seq],""                   ,&stat);
    fits_write_key(fp, TINT    ,"EXPNUM  ",&cam->SeqN[seq],""                   ,&stat);
    fits_write_key(fp, TINT    ,"EXPTOTAL",&img_total     ,""                   ,&stat);

//  Telescope axes/focus info
    fits_write_key(fp, TDOUBLE ,"ALTITUDE",&tel_alt       ,"Telescope altitude axis angle",&stat);
    fits_write_key(fp, TDOUBLE ,"AZIMUTH ",&tel_azm       ,"Telescope azimuth axis angle" ,&stat);
    fits_write_key(fp, TDOUBLE ,"ROTANGLE",&tel_cas       ,"CAS rotator angle"            ,&stat);
    fits_write_key(fp, TDOUBLE ,"FOCUSPOS",&tel_foc       ,"Focus position"               ,&stat);

//  Target info
    fits_write_key(fp, TSTRING ,"RA      ",fts_ra         ,"",&stat);
    fits_write_key(fp, TSTRING ,"DEC     ",fts_dec        ,"",&stat);
    fits_write_key(fp, TSTRING ,"OBJECT  ",fts_obj        ,"",&stat);
    fits_write_key(fp, TSTRING ,"RADECSYS","FK5"          ,"",&stat);
    fits_write_key(fp, TDOUBLE ,"EQUINOX ",&equinox       ,"",&stat);
    fits_write_key(fp, TDOUBLE ,"MJD     ",&mjd           ,"",&stat);

//  Write pre-amble and starting time fields    
    fits_write_key(fp, TSTRING ,"DATE    ",d              ,"[UTC] Start date of obs.",&stat);
    fits_write_key(fp, TSTRING ,"DATE-OBS",dt             ,"[UTC] Start of obs."     ,&stat);
    fits_write_key(fp, TSTRING ,"UTSTART ",t              ,"[UTC] Start time of obs.",&stat);

//  Generate end date/time strings    
    tim=localtime(&cam->ObsEnd.tv_sec);
    len=strftime( t, sizeof(t)-1, "%H:%M:%S", tim); // Time
    strftime( d, sizeof( d)-1, "%Y-%m-%d",    tim); // Date
    snprintf( dt,sizeof(dt)-1, "%sT%s", d, t );     // Date & time in ISO format (no fractional seconds)
    snprintf( s, sizeof( s)-1, "%.3f", cam->ObsEnd.tv_usec/TIM_MICROSECOND ); // Get fractional sec 
    strncat( &dt[len], &s[1], sizeof(dt)-1 -len);   // Append fraction omitting decimal point
    duration = (cam->ObsEnd.tv_sec  - cam->ObsStart.tv_sec ) +
               (cam->ObsEnd.tv_usec - cam->ObsStart.tv_usec)/TIM_MICROSECOND;

//  Write the remaining FITS tags    
    fits_write_key(fp, TSTRING,"ENDDATE ",d                ,"[UTC] End date of obs."    ,&stat);
    fits_write_key(fp, TSTRING,"END-OBS ",dt               ,"[UTC] End of obs."         ,&stat);
    fits_write_key(fp, TSTRING,"UTEND   ",t                ,"[UTC] End time of obs."    ,&stat);
    fits_write_key(fp, TDOUBLE,"DURATION",&duration        ,"[sec] Total obs. duration" ,&stat);

//  MOPTOP rotator info
    fits_write_key(fp, TDOUBLE,"MOPRREQ ",&cam->RotReq[seq],"[deg] MOPTOP Rotator requested angle"  ,&stat);
    fits_write_key(fp, TDOUBLE,"MOPRBEG ",&cam->RotAng[seq],"[deg] MOPTOP Rotator begin angle"      ,&stat);
    fits_write_key(fp, TDOUBLE,"MOPREND ",&cam->RotEnd[seq],"[deg] MOPTOP Rotator angle"            ,&stat);
    fits_write_key(fp, TDOUBLE,"MOPRARC ",&cam->RotDif[seq],"[deg] MOPTOP Rotator exposure arc"     ,&stat);
    fits_write_key(fp, TINT   ,"MOPRNUM ",&cam->RotN[seq]  ,"MOPTOP Rotation number"                ,&stat);
    fits_write_key(fp, TINT   ,"MOPRPOS ",&cam->SeqN[seq]  ,"MOPTOP Position number within rotation",&stat);

//  Andor detector info
    fits_write_key(fp, TSTRING,"TRIGGER ",trigger          ,"Detector trigger mode"     ,&stat);
    fits_write_key(fp, TDOUBLE,"EXPREQST",&cam->ExpReq     ,"[sec] Requested exposure"  ,&stat);
    fits_write_key(fp, TDOUBLE,"EXPTIME ",&cam->ExpVal     ,"[sec] Actual exposure"     ,&stat);
    fits_write_key(fp, TDOUBLE,"GAIN    ",&cam->Gain       ,"[e/ADU] Detector gain"     ,&stat);
    fits_write_key(fp, TINT   ,"CCDXBIN ",&fts_ccdxbin     ,"X binning"                 ,&stat);
    fits_write_key(fp, TINT   ,"CCDYBIN ",&fts_ccdybin     ,"Y binning"                 ,&stat);
    fits_write_key(fp, TDOUBLE,"CCDATEMP",&ccdatemp        ,"[K] Detector temperature"  ,&stat);
    fits_write_key(fp, TSTRING,"CCDTYPE ","sCMOS"          ,"Detector type"             ,&stat);
    fits_write_key(fp, TSTRING,"CCDMODEL",det_model        ,"Detector model"            ,&stat);
    fits_write_key(fp, TSTRING,"CCDSERNO",det_serno        ,"Detector serial number"    ,&stat);
    fits_write_key(fp, TSTRING,"CCDRATE ",det_rate         ,"[MHz] Detector read rate " ,&stat);
    fits_write_key(fp, TSTRING,"CCDORDER",det_rd           ,"Detector read order"       ,&stat);
    fits_write_key(fp, TSTRING,"CCDENCOD",det_encod        ,"Detector pixel encoding"   ,&stat);
    fits_write_key(fp, TSTRING,"CCDAMP  ",det_amp          ,"Detector pre-amp gain mode",&stat);
    fits_write_key(fp, TINT   ,"CCDDEPTH",&cam->WellDepth  ,"[e] Detector well depth"   ,&stat);
    fits_write_key(fp, TDOUBLE,"CCDDARK ",&cam->DarkCurrent,"[e/px/s] Detector median dark current"  ,&stat);
    fits_write_key(fp, TDOUBLE,"CCDXPIXE",&ccdxpixe        ,"[m] Detector pixel width"  ,&stat);
    fits_write_key(fp, TDOUBLE,"CCDYPIXE",&ccdypixe        ,"[m] Detector pixel height" ,&stat);
    fits_write_key(fp, TULONG ,"CLKFREQ ",&cam->TimestampClockFrequency, "[Hz] Detector clock tick frequency",&stat);
    fits_write_key(fp, TULONG ,"CLKSTAMP",&cam->TimestampClock[seq],     "Image clock tick value", &stat);

//  Convert to Mono16 using metadata in case 12-bit was selected
    at_chk( AT_ConvertBufferUsingMetadata( cam->ImageBuffer[buf], img_mono16, cam->ImageSizeBytes, L"Mono16" ),
           "ConvertBufferUsingMetadata", L"Mono16" );  

//  Write the image and close file
    fits_write_img(fp, TUSHORT, 1, img_mono16size, img_mono16, &stat);
    if ( stat )
    {
        fits_get_errstatus(stat, text);
        mop_log( false, MOP_DBG, FAC_FTS,"fits_write_img() status=%s seq=%i buf=%i ", text, seq, buf );
    }
    else
    {
//        mop_log( true,  MOP_DBG, FAC_CAM, "Wrote image to file %s", filename );
    }
    fits_close_file(fp, &stat);

    return true;
}
