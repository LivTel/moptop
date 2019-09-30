/** @file   mop_utl.c
  *
  * @brief  Utility functions.
  *
  * @author asp 
  *
  * @date   2019-05-01 
  */

#include "moptop.h"

// Module local definintion and data


// Dual process version uses shared memory to synchronise 
#define SHM_NAME "/MOPTOP"
#define SHM_OPTS (O_RDWR | O_CREAT)
#define SHM_PROT (PROT_READ | PROT_WRITE)
#define SHM_MODE (00666 )
int   shm_fd;   // File descriptor for shared memeory
char *shm_ptr;  // Pointer to start of shared data
//char *this_ptr; // Pointer to this process shared data


// Thread version uses semphore to synchronise
#define SEM_ROTATE "\\ROTATE"
#define SEM_STABLE "\\STABLE"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
sem_t  *sem_stable;
sem_t  *sem_rotate;


/** @brief     Create inter-process semaphore 
  *
  * @return    true|false = success|failure
  */
bool utl_mksem( void )
{
    if ( SEM_FAILED == (sem_rotate = sem_open(SEM_ROTATE, O_CREAT, SEM_PERMS, 0)))
            return mop_log( false, MOP_SYS, FAC_UTL, "sem_open("SEM_ROTATE")");
    if ( SEM_FAILED == (sem_stable = sem_open(SEM_STABLE, O_CREAT, SEM_PERMS, 0)))
            return mop_log( false, MOP_DBG, FAC_UTL, "sem_open("SEM_STABLE")");

    return true;
}

/** @brief     Wait for other camera to reach stable temperature 
  *
  * @param[in] timeout [s] 
  *
  * @return    true|false = stable|timeout
  */
bool utl_sem_stable( int timeout )
{
    struct timespec delta;
    struct timespec expire;

//  Set timeout   
    delta.tv_sec  = timeout;
    delta.tv_nsec = 0; 
   
    timespec_get( &expire, TIME_UTC );   
    expire = utl_ts_add( &expire, &delta ); 

    if ( mop_master )
    {
        mop_log( true, MOP_DBG, FAC_UTL, "Master awaiting SEM_STABLE");
        if ( sem_timedwait( sem_stable, &expire ) )
            return mop_log( false, MOP_ERR, FAC_UTL, "Recv SEM_STABLE");
        else
            return mop_log( true,  MOP_DBG, FAC_UTL, "Recv SEM_STABLE");
    }
    else    
    {
        mop_log( true, MOP_DBG, FAC_UTL, "Slave posting SEM_STABLE");
        if ( sem_post( sem_stable ))
            return mop_log( false, MOP_ERR, FAC_UTL, "Post SEM_STABLE");
        else
            return mop_log( true,  MOP_DBG, FAC_UTL, "Post SEM_STABLE"  );
    }
}

/** @brief     Wait for master process to start rotation  
  *
  * @param[in] timeout [s] 
  *
  * @return    true|false = rotating|timeout
  */
bool utl_sem_rotate( int timeout )
{
    struct timespec delta;
    struct timespec expire;

//  Set timeout   
    delta.tv_sec  = timeout;
    delta.tv_nsec = 0; 

    timespec_get( &expire, TIME_UTC );   
    expire = utl_ts_add( &expire, &delta ); 
  
    if ( mop_master )
    {
        mop_log( true, MOP_DBG, FAC_UTL, "Master posting SEM_ROTATE");
        if ( sem_post( sem_rotate ))
            return mop_log( false, MOP_ERR, FAC_UTL, "Post SEM_ROTATE");
        else
            return mop_log( true,  MOP_DBG, FAC_UTL, "Post SEM_ROTATE");
    }
    else    
    {
        mop_log( true, MOP_DBG, FAC_UTL, "Slave awaiting SEM_ROTATE");
        if ( sem_timedwait( sem_rotate, &expire ) )
            return mop_log( false, MOP_ERR, FAC_UTL, "Slave receive SEM_ROTATE");
        else
            return mop_log( true,  MOP_DBG, FAC_UTL, "Slave receive SEM_ROTATE");
    }
}


/** @brief Create shared memory for synchronising master/slave process
  *
  * @return    true|false = success/failure
  */
bool utl_mkshm( void )
{
    if (((shm_fd  = shm_open( SHM_NAME, SHM_OPTS, SHM_MODE )              ) > 0)&& // Get shared memory file desc.
        ( ftruncate( shm_fd, CAM_COUNT )                                   == 0)&& // Force size
        ((shm_ptr = mmap(NULL, CAM_COUNT, SHM_PROT, MAP_SHARED, shm_fd, 0)) > 0)  )// Map shared memory
    {
//      Zero this process shared memory sync. variable   
//        this_ptr = shm_ptr + this;
       *(shm_ptr + CAM1 ) = SHM_RESET;
       *(shm_ptr + CAM2 ) = SHM_RESET;
        return true; 
    }

    return mop_log( false, MOP_SYS, FAC_UTL, "utl_mkshm()" );
}


/** @brief     Set this process' sync. state variable
  *
  * @param[in] state = state to be set 
  *
  * @return    void 
  */
bool utl_shm_post( int state, int which )
{
    *(shm_ptr + which) = state;
     return true;
}


/** @brief Wait for a process synchronisation state 
  *
  * @param[in]  which   = which flag to synchronise on 
  * @param[in]  value   = flag state to wait for
  * @param[in]  reset   = reset flag if state detected 
  * @param[in]  timeout = timeout [us]
  *
  * @return    true = state reached, false = timeout  
  */
bool utl_shm_wait( int state, int which, int timeout, bool reset )
{
    int tick  = TIM_TICK;                          // Timer ticks [us] 
    int count = TIM_MICROSECOND * timeout / tick;  // Timer count [us] 
    char *ptr;

    ptr = shm_ptr + which;   
    do
    { 
        if ( *ptr == state )
        {
           if ( reset )
              *ptr = SHM_RESET;
           return true;
        }
        usleep(tick);
    } while( count-- );
    return mop_log( false, MOP_ERR, FAC_UTL, "utl_shm_wait() timeout");
}


/** @brief     Convert a decimal time [sec] to timespec
  *
  * @param[in] dbl = input time as decimal seconds
  *
  * @return    ts  = timespec struct   
  */
struct timespec utl_dbl2ts( double dbl )
{
    struct timespec ts;
    double sec;

    ts.tv_nsec = TIM_NANOSECOND * modf( dbl, &sec );  
    ts.tv_sec  = (int)sec;

    return ts;
}


/** @brief     Add two timespec values t1 + t2
  *
  * @param[in] *t1 = time 1
  * @param[in] *t2 = time 2
  *
  * @return    sum = sum of t1_t2 in a timespec struct   
  */
struct timespec utl_ts_add( struct timespec *t1, struct timespec *t2 )
{
    struct timespec sum;

    sum.tv_sec  = t1->tv_sec  + t2->tv_sec;
    sum.tv_nsec = t1->tv_nsec + t2->tv_nsec;

//  If tv_nsec overflowed apply correction
    if ( sum.tv_nsec >= TIM_NANOSECOND )
    {
       sum.tv_sec++;
       sum.tv_nsec -= TIM_NANOSECOND; 
    }  

    return sum;
} 


/** @brief  Subtract two timespec values t1 - t2
  *
  * @param[in] *t1 = time 1
  * @param[in] *t2 = time 2
  *
  * @return    diff = difference in a  timespec struct   
  */
struct timespec utl_ts_sub( struct timespec *t1, struct timespec *t2 )
{
    struct timespec diff = {0, 0};

    if (( t1->tv_sec  < t2->tv_sec                                 )||
        ((t1->tv_sec == t2->tv_sec) && (t1->tv_nsec <= t2->tv_nsec))  )
    {
//      Invalid or identical subtraction. Return {0,0};
        return diff;
    }
    else
    {
        diff.tv_sec = t1->tv_sec - t2->tv_sec;
        if ( t1->tv_nsec <  t2->tv_nsec )
        {
            diff.tv_nsec = TIM_NANOSECOND + t1->tv_nsec - t2->tv_nsec;
            diff.tv_sec--;
        }
        else
        {
            diff.tv_nsec = t1->tv_nsec - t2->tv_nsec;
        }
    }

    return diff;
}


/** @brief Compare timespec times
  *
  * @param[in] *t1 = time 1
  * @param[in] *t2 = time 2
  *
  * @return    -1 = less than, 0 = same, +1 = greater than 
  */
int utl_ts_cmp( struct timespec *t1, struct timespec *t2 )
{
    if      ( t1->tv_sec  < t2->tv_sec )  
        return -1;                     // Less than
    else if ( t1->tv_sec  > t2->tv_sec )
        return  1;                     // Greater than
    else if ( t1->tv_nsec < t2->tv_nsec)
        return -1;                     // Less than
    else if ( t1->tv_nsec > t2->tv_nsec)
        return  1;                     // Greater than
    else
        return  0;                     // Equal to 
}


/** @brief     Uppercase a string
  *
  * @param[in] str = pointer to input string 
  *
  * @return    str = pointer to converted string (same as input)
  */
char *strtoupper( char *str )
{
    char *ret;

    for( ret = str ; *str; str++ )
        *str = toupper(*str);
    return ret;
}
