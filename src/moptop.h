/** @file   moptop.h
  *
  * @brief  General header for all MOPTOP modules.
  *
  * @author asp 
  *
  * @date   2019-05-01 
  */

// Needed for extended dirent() functionality
#define _GNU_SOURCE

// Standard C headers 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <wchar.h> 

// System headers
#include <sys/dir.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

// Non-standard third party headers
#include <PI_GCS2_DLL.h>	/// PI rotator
#include <atcore.h>		/// Andor camera access
#include <atutility.h>		/// Andor conversion utils NB Hacked to work with C 
#include <fitsio.h>		/// Starlink FITS 

#define MOP_VERSION	    "0.17"

// Return values
#define MOP_SUCCESS 0
#define MOP_FAILURE 1
#define MOP_MASTER  "MASTER"
#define MOP_SLAVE   "SLAVE0"

// Andor error ranges
#define AT_ERR_MIN 0
#define AT_ERR_MAX 39 
#define UT_ERR_MIN 1000
#define UT_ERR_MAX 1006 

// For when Andor things go wrong
#define AT_TRY   3  /// AT command max. retries
#define AT_DLY   1  /// AT command retry delay  

// Log levels
#define MOP_CRIT 0  /// Critical error, process will exit 
#define MOP_SYS  1  /// System error, operation failed
#define MOP_ERR  2  /// Error, operation failed 
#define MOP_WRN  3  /// Warning but can progress 
#define MOP_IMG  4  /// Image acquisition   
#define MOP_INF  5  /// Informational  
#define MOP_DBG  6  /// Debug message 

// Facilities
#define FAC_MOP  0
#define FAC_LOG  1
#define FAC_UTL  2
#define FAC_OPT  3
#define FAC_CAM  4
#define FAC_ROT  5
#define FAC_FTS  6 

// Basic text colour 30=Black, 31=red, 32=green, 33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white  
#define COL_RED     "\x1b[31m"  
#define COL_GREEN   "\x1b[32m"  
#define COL_YELLOW  "\x1b[33m"  
#define COL_BLUE    "\x1b[34m"  
#define COL_MAGENTA "\x1b[35m"    
#define COL_CYAN    "\x1b[36m"   
#define COL_WHITE   "\x1b[37m"    
#define COL_RESET   "\x1b[0m"    
#define COL_NULL    NULL    

// Space fillng for multi-informational lines
#define LOG_BLANK "\n\t\t\t\t  "

//Timeouts
#define TMO_ROTATE      30.0            /// [s]  Rotator motion to complete 
#define TMO_STABLE      60.0            /// [s]  Thermal stabilisation 
#define TMO_TRIG        45.0            /// [s]  Inter process trigger 
#define TMO_XFER        30000           /// [ms] Image transfer timeout 

// PI prefix = ROT_
//#define ROT_BAUD        460800          /// Baud rate 
#define ROT_BAUD        115200          /// Alternative baud rate 
#define ROT_USB         "/dev/ttyUSB0"  /// Default USB device
#define ROT_STP8        45.0            /// [deg] Step between triggers,  8 position  
#define ROT_STP16       22.5            /// [deg] Step between triggers, 16 position
#define ROT_STP32       11.25           /// [deg] Step between triggers, 32 postion 
#define ROT_VEL         45.0            /// [deg/s] Default rotator velocity deg/sec 
#define ROT_REVS         3              /// Default revolutions
#define ROT_ZERO         0.0            /// [deg] Default starting position
#define ROT_TOLERANCE    0.004          /// [deg] Rotator tolerance 
#define ROT_MAX         36000.0         /// Limit: Maximum travel distance      

#define ROT_REBOOT      "RBT"           /// Reboot rotator 
#define ROT_CLR_ERR     "STP"           /// Stop all axis motion 
#define ROT_ALL_STOP    "ERR?"          /// Clear error(s) 
#define ROT_INI_SVO     "SVO 1 1"       /// Enable servo
#define ROT_INI_FRF     "FRF 1"         /// Enable relative motion 
#define ROT_INI_VEL     "VEL 1 360"     /// Default velocity at init. [deg/s] 
#define ROT_INI_ANGLE    22.0           /// [deg] Initial angle (unsigned) 
#define ROT_INI_POS     "MOV 1 %f"      /// Move to initial angle       
#define ROT_MOV         "MOV 1 %f"      /// Move to position       

#define ROT_TRG_VEL     "VEL 1 %f"      /// Angular velocity while triggering 
#define ROT_TRG_STP     "CTO 1 1 %f"    /// Value: Trigger every 22.5 degrees of distance travel       
#define ROT_TRG_PIN_5   "CTO 1 2 1"     /// Pin:   Trigger via output 1 (physical pin 5) on axis 1       
#define ROT_TRG_DST     "CTO 1 3 0"     /// Mode:  Use distance travel for trigger events    
#define ROT_TRG_POSSTP  "CTO 1 3 7"     /// Mode:  Use start position and distance travel for trigger events    
#define ROT_TRG_HI      "CTO 1 7 1"     /// Trigger polarity high
#define ROT_TRG_LO      "CTO 1 7 0"     /// Trigger polarity low
#define ROT_TRG_POSBEG  "CTO 1 8 0"     /// Value: Trigger start when position=0.0 deg       
#define ROT_TRG_POSEND  "CTO 1 9 %f"    /// Value: Trigger ends at this travel limit      
#define ROT_TRG_POSINIT "CTO 1 10 0"    /// Value: Trigger enabled at position=0.0 deg       
#define ROT_TRG_LEN     "CTO 1 11 50"   /// Trigger pulse width in 33.3ns increments
#define ROT_TRG_ENA     "TRO 1 1"       /// Enable trigger       
#define ROT_TRG_DIS     "TRO 1 0"       /// Disable trigger       

#define ROT_CW           1              /// Clockwise rotation
#define ROT_CCW         -1              /// Counter-clockwise rotation
#define ROT_STAT         0              /// Static mode 

// FITS file definitions
#define FTS_SFX       "_0.fits"         /// File file suffix 
#define FTS_PFX       "%i_"             /// File file suffix 

// File prefix
#define FTS_PFX_BIAS  'b'               /// Bias frame
#define FTS_PFX_DARK  'd'               /// Dark frame
#define FTS_PFX_EXP   'e'               /// Observation exposure
#define FTS_PFX_FLAT  'f'               /// Flat frame 
#define FTS_PFX_ACQ   'q'               /// Acquisition frame
#define FTS_PFX_STD   's'               /// Standard object

// Image type written into FITS as OBSTYPE 
#define FTS_TYP_BIAS  "BIAS"            /// Mnimum exposure time  
#define FTS_TYP_DARK  "DARK"            /// Dark
#define FTS_TYP_EXP   "EXPOSE"          /// Normal   
#define FTS_TYP_FLAT  "SKY-FLAT"        /// Sky-flat
#define FTS_TYP_ACQ   "ACQUIRE"         /// Acquisition frame  
#define FTS_TYP_STD   "STANDARD"        /// Standard object

#define FTS_ID        "123456"          /// Permissible set of first character in FITS filename
#define FTS_DIR       "."               /// Default destination         

// Defaults used for testing
#define FTS_TST_OBJ   "Undefined"
#define FTS_TST_RA    "00:00:00.00"
#define FTS_TST_DEC   "00:00:00.00"

// Time related constants
#define TIM_TICK         1000
#define TIM_MILLISECOND  1000.0
#define TIM_MICROSECOND  1000000.0
#define TIM_NANOSECOND   1000000000

// For synchronisation via share memory functions  
#define SHM_RESET      0    /// Unset state
#define SHM_STABLE     1    /// Slave temperature stable
#define SHM_ROTATE     2    /// Master rotation start
#define SHM_TRIG       3    /// Trigger slave in static mode 

// Camera idenification
#define CAM1     0     // Device indices. 0 is master
#define CAM2     1
#define CAM3     2
#define CAM4     3
#define CAM5     4 
#define CAM6     5 
#define CAM_ALL -1 

// Camera defaults
#define CAM_COUNT      2     /// Total number: Prototype = 2, Final = 4
#define CAM_TEMP       4.0   /// [deg C] Target cooling temperature  
#define CAM_EXP        0.45  /// [s] Default exposure time 

// Binning 
#define CAM_BIN_1      L"1x1"
#define CAM_BIN_2      L"2x2"
#define CAM_BIN_3      L"3x3"
#define CAM_BIN_4      L"4x4"
#define CAM_BIN_8      L"8x8"

// Camera setables
#define CAM_MHZ_100    L"100 MHz" /// Zyla
#define CAM_MHZ_270    L"270 MHz" /// Zyla
#define IDX_MHZ_100    0          /// Array index to gain/noise in cam_info
#define IDX_MHZ_270    1          /// Array index to gain/noise in cam_info 

// Amplifier
#define CAM_AMP_16L    L"16-bit (low noise & high well capacity)"
#define CAM_AMP_12L    L"12-bit (low noise)"  
#define CAM_AMP_12H    L"12-bit (high well capacity)"
#define IDX_AMP_16L    0
#define IDX_AMP_12L    1
#define IDX_AMP_12H    2

// Read direction
#define CAM_RD_BUSEQ   L"Bottom Up Sequential"
#define CAM_RD_BUSIM   L"Bottom Up Simultaneous"
#define CAM_RD_COSIM   L"Centre Out Simultaneous"
#define CAM_RD_OISIM   L"Outside In Simultaneous"
#define CAM_RD_TDSEQ   L"Top Down Sequential"
#define CAM_RD_TDSIM   L"Top Down Simultaneous"

// Transfer encoding
#define CAM_ENC_12     L"Mono12"
#define CAM_ENC_12PACK L"Mono12Packed"
#define CAM_ENC_16     L"Mono16"
#define CAM_ENC_32     L"Mono32"

// Trigger source
#define CAM_TRG_INT    L"Internal" 
#define CAM_TRG_EDGE   L"External" 
#define CAM_TRG_WIDTH  L"External Exposure"
#define CAM_TRG_SW     L"Software" 


// Image 
#define IMG_WIDTH      0     /// Array indices
#define IMG_HEIGHT     1     //
#define IMG_DIMENSIONS 2     /// sCMOS dimensions
#define IMG_CYCLE      16    /// Default images per revolution 
#define IMG_TOTAL      IMG_CYCLE * ROT_REVS     
#define MAX_CYCLE      16    /// Max. images per cycle
#define MAX_REVS       100   /// Max. PI stage rotations
#define MAX_IMAGES     MAX_CYCLE * MAX_REVS

#define TEL_UNSET      999.0 /// Unset/invalid telescope FITS parameter

// @brief Common structures _s and templates _t 
typedef struct cam_spec_s
{
    double gain [3]; /// For 3 possible amplifier settings
    double noise[3];
} cam_spec_t;

typedef struct cam_info_s
{
    int     FtsIndex;     /// FITS file  
    char   *Filter;       /// Filter name
    char   *FilterID;     /// Filter ID 
    double  PolAngle;     /// Polarisation angle WRT rotator
    AT_WC  *SerialNumber; /// Camera internal SN
    AT_WC  *Model;        /// Camera internal model name
    int    WellDepth;     /// [e] Pixel well depth
    double DarkCurrent;   /// Median dark current
    struct cam_spec_s mhz[2]; /// For 100 & 270 MHz read rates
} cam_info_t;


// Camera structure. Member name match Andor SDK V3.12 (mostly)
typedef struct mop_cam_s
{
    int    USBIndex;    /// Zyla camera USB index
    
    AT_H   Handle;
    AT_WC  SerialNumber[256];
    AT_WC  FirmwareVersion[256];
    AT_WC *Model;
    int    WellDepth;
    double DarkCurrent;     
    double Gain;            
    double Noise;  
    char  *Filter;
    char  *FilterID;
    AT_U8 *UserBuffer;
    AT_U8 *ImageBuffer[MAX_IMAGES];
    AT_64  ImageSizeBytes;  
    AT_WC  TemperatureStatus[256];
    AT_64  SensorWidth;       /// [px] Sensor width 
    AT_64  SensorHeight;      /// [px] Sensor height 
    double SensorTemperature; /// [C]  Sensor temp 
    double BytesPerPixel; 
    double PixelHeight;
    double PixelWidth;
    long   Dimension[IMG_DIMENSIONS];
    double ReadoutTime; 
    AT_64  TimestampClockFrequency;   /// Detector timestamp frequency [Hz]
    AT_64  TimestampClock[MAX_IMAGES]; 
    unsigned long Ticks[MAX_IMAGES];
    AT_BOOL FullAOIControl;

    unsigned char *ReturnBuffer[MAX_IMAGES];
    int            ReturnSize  [MAX_IMAGES];
    double ExpReq;             /// [s] Requested exposure time 
    double ExpVal;             /// [s] Reported exposure time 
    double ExpMax;             /// [s] Max Exposure time limit
    double ExpMin;             /// [s] Min
    double ExpDif;             /// [s] Total time to acquire image
    double RotReq[MAX_IMAGES]; /// [deg] Requested rotator position (absolute)
    double RotAng[MAX_IMAGES]; /// [deg] Actual rotator angle
    double RotEnd[MAX_IMAGES]; /// [deg] End rotator position for this exposure
    double RotDif[MAX_IMAGES]; /// [deg] Length of arc for this exposure 
    int    RotN[MAX_IMAGES];   /// Rotation number 1-100
    int    SeqN[MAX_IMAGES];   /// Position within rotation 1-8 or 1-16 
    int    Alloc;	       /// Memory allocation method

    struct timeval ObsStart;
    struct timeval ObsEnd;

//  FITS file name variables    
    char   id; /// FITS file prefix
    int   seq; /// Sequence 
    char *dir; /// Destination directory
//    bool  first;
} mop_cam_t;

// Macros
#define btoa(x) ((x)?"true":"false")  /// Boolean to ascii string 

// Function prototypes
// MOPTOP process functions
bool mop_init( void );
bool mop_opts( int argc, char *argv[] );
void mop_exit( int exit_code );

// PI rotator function
bool rot_init( char *usb, int baud, int timeout, char *trigger );
bool rot_cmd ( char *cmd, char *msg );
bool rot_move( double  angle );              // Start motion 
bool rot_get ( double *angle );              // Read current position 
bool rot_set ( double  angle, int timeout ); // Go to position within timeout 
bool rot_goto( double  angle, int timeout, double *actual ); // Wait for position within timeout 
bool rot_wait( double  angle, int timeout, bool cw );        // Wait for position within timeout 
bool rot_trg_ena( bool enable );             // Trigger enable/disable 
bool rot_ont ( int delay );                  // Wait for on target state
double rot_dbg( char *dbg );                 // Debug: Print & return rotator angle

// Andor camera functions
//bool  at_chk      ( int ret, char *fn );
bool  at_chk      ( int ret, char *fn, AT_WC *cmd );
bool  at_try      ( mop_cam_t *cam, void *fn, AT_WC *cmd, ...  );
bool  cam_init    ( int i );
bool  cam_chk     ( mop_cam_t *cam );
bool  cam_open    ( mop_cam_t *cam, double exp );
bool  cam_close   ( mop_cam_t *cam );
bool  cam_stable  ( mop_cam_t *cam, double TargetTemperature, int timeout, bool fast );
bool  cam_queue   ( mop_cam_t *cam );
bool  cam_param   ( mop_cam_t *cam );
bool  cam_alloc   ( mop_cam_t *cam );                // Allocate memory buffers 
void  cam_feature ( mop_cam_t *cam, AT_WC *Feature );// Get feature options
bool  cam_acq_circ( mop_cam_t *cam, char pfx);       // Acquire images - circular buffer
bool  cam_acq_stat( mop_cam_t *cam, char pfx);       // Acquire images -static 
bool  cam_trg_set ( mop_cam_t *cam, AT_WC *trg );    // Set trigger mode 
bool  cam_acq_ena ( mop_cam_t *cam, AT_BOOL );       // Acquisition enable/disable
bool  cam_clk_rst ( mop_cam_t *cam );                // Reset camera clock to zero

// FITS file functions
char *fts_mkname( mop_cam_t *cam, char pfx );
bool  fts_write ( char *filename, mop_cam_t *cam, int seq, int buf );

// Error & logging functions
bool mop_log( bool ret, int level, int fac, char *fmt, ... );
unsigned long cam_ticks( mop_cam_t *cam, int img );

// Utility functions
bool            utl_mksem     ( void        );
bool            utl_sem_stable( int timeout );
bool            utl_sem_rotate( int timeout );
bool            utl_mkshm   ( void );
bool            utl_shm_post( int state, int which );
bool            utl_shm_wait( int state, int which, int timeout, bool reset );
struct timespec utl_dbl2ts( double dbl );
struct timespec utl_ts_add( struct timespec *t1, struct timespec *t2 );
struct timespec utl_ts_sub( struct timespec *t1, struct timespec *t2 );
int             utl_ts_cmp( struct timespec *t1, struct timespec *t2 );
char  *strtoupper( char *str );

// Global data and external definitions
#include "mop_dat.h"
