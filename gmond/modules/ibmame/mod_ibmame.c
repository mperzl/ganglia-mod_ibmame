/******************************************************************************
 *
 *  This module implements IBM AIX Memory Expansion (AME) extensions.
 *
 *  The libperfstat API is used and it can deal with a 32-bit and a 64-bit
 *  kernel and does not require root authority.
 *
 *  The code has been tested with AIX 6.1 and AIX 7.1
 *  on different systems.
 *
 *  Written by Michael Perzl (michael@perzl.org)
 *
 *  Version 1.1, Oct 22, 2012
 *
 *  Version 1.1:  Oct 22, 2012
 *                - fixed some logical errors in the module
 *                - implemented some work-arounds for libperfstat errors
 *                - changed /etc/ganglia/conf.d/ibmame.conf settings
 *
 *  Version 1.0:  Oct 13, 2010
 *                - initial version
 *
 ******************************************************************************/

/*
 * The ganglia metric "C" interface, required for building DSO modules.
 */

#include <gm_metric.h>


#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include <ctype.h>
#include <utmp.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <sys/vminfo.h>
#include <libperfstat.h>

#include "libmetrics.h"


static time_t boottime;



g_val_t
ame_enabled_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      strcpy( val.str, "libperfstat returned an error" );
   else
      strcpy ( val.str, p.type.b.ame_enabled ? "yes" : "no" );

   return( val );
}



g_val_t
ame_version_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      strcpy( val.str, "libperfstat returned an error" );
   else
      snprintf( val.str, MAX_G_STRING_SIZE, "%d", p.ame_version );

   return( val );
}



g_val_t
true_memory_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.true_memory * 4096.0;

   return( val );
}



g_val_t
expanded_memory_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.expanded_memory * 4096.0;

   return( val );
}



/**************************************************************************/
/*                                                                        */
/*
g_val_t
target_memexp_factr_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.f = -1.0;
   else
      val.f = (float) p.target_memexp_factr / 100.0;

   return( val );
}
*/

g_val_t
target_memexp_factr_func( void )
{
   g_val_t val;
   struct vminfo vmi;


   if (vmgetinfo( &vmi, VMINFO, sizeof( vmi ) ) == -1)
      val.f = -1.0;
   else
      val.f = (float) vmi.ame_factor_tgt / 100.0;

   return( val );
}



/**************************************************************************/
/*                                                                        */
/*
g_val_t
current_memexp_factr_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.f = -1.0;
   else
      val.f = (float) p.current_memexp_factr / 100.0;

   return( val );
}
*/
g_val_t
current_memexp_factr_func( void )
{
   g_val_t val;
   struct vminfo vmi;


   if (vmgetinfo( &vmi, VMINFO, sizeof( vmi ) ) == -1)
      val.f = -1.0;
   else
      val.f = (float) vmi.ame_factor_actual / 100.0;

   return( val );
}



g_val_t
target_cpool_size_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.target_cpool_size;

   return( val );
}



g_val_t
max_cpool_size_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.max_cpool_size;

   return( val );
}



g_val_t
min_ucpool_size_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.min_ucpool_size;

   return( val );
}



g_val_t
ame_deficit_size_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;


   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.d = -1.0;
   else
      val.d = (double) p.ame_deficit_size;

   return( val );
}



g_val_t
ame_cores_used_func( void )
{
   g_val_t val;
   perfstat_partition_total_t p;
   static uint64_t saved_cmcs_total_time = 0LL;
   longlong_t diff;
   static double last_time = 0.0;
   static float last_val = 0.0;
   double now, delta_t;
   struct timeval timeValue;
   struct timezone timeZone;


   gettimeofday( &timeValue, &timeZone );

   now = (double) (timeValue.tv_sec - boottime) + (timeValue.tv_usec / 1000000.0);

   if (perfstat_partition_total( NULL, &p, sizeof( perfstat_partition_total_t ), 1 ) == -1)
      val.f = -1.0;
   else
   {
      delta_t = now - last_time;

      if ( delta_t > 0.0 )
      {
         diff = p.cmcs_total_time - saved_cmcs_total_time;

         if (diff >= 0LL)
            val.f = (double) diff / delta_t / 1000.0 / 1000.0 / 1000.0;
         else
            val.f = last_val;
      }
      else
         val.f = 0.0;

      saved_cmcs_total_time = p.cmcs_total_time;
   }

   last_time = now;
   last_val = val.f;

   return( val );
}



static time_t
boottime_func_CALLED_ONCE( void )
{
   time_t boottime;
   struct utmp buf;
   FILE *utmp;


   utmp = fopen( UTMP_FILE, "r" );

   if (utmp == NULL)
   {
      /* Can't open utmp, use current time as boottime */
      boottime = time( NULL );
   }
   else
   {
      while (fread( (char *) &buf, sizeof( buf ), 1, utmp ) == 1)
      {
         if (buf.ut_type == BOOT_TIME)
         {
            boottime = buf.ut_time;
            break;
        }
      }

      fclose( utmp );
   }

   return( boottime );
}



/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */
extern mmodule ibmame_module;


static int ibmame_metric_init ( apr_pool_t *p )
{
   int i;
   g_val_t val;


   for (i = 0;  ibmame_module.metrics_info[i].name != NULL;  i++)
   {
      /* Initialize the metadata storage for each of the metrics and then
       *  store one or more key/value pairs.  The define MGROUPS defines
       *  the key for the grouping attribute. */
      MMETRIC_INIT_METADATA( &(ibmame_module.metrics_info[i]), p );
      MMETRIC_ADD_METADATA( &(ibmame_module.metrics_info[i]), MGROUP, "ibmame" );
   }


/* initialize the routines which require a time interval */

   boottime = boottime_func_CALLED_ONCE();
   val = ame_cores_used_func();

   return( 0 );
}



static void ibmame_metric_cleanup ( void )
{
}



static g_val_t ibmame_metric_handler ( int metric_index )
{
   g_val_t val;

/* The metric_index corresponds to the order in which
   the metrics appear in the metric_info array
*/
   switch (metric_index)
   {
      case  0:  return( ame_enabled_func() );
      case  1:  return( ame_version_func() );
      case  2:  return( true_memory_func() );
      case  3:  return( expanded_memory_func() );
      case  4:  return( target_memexp_factr_func() );
      case  5:  return( current_memexp_factr_func() );
      case  6:  return( target_cpool_size_func() );
      case  7:  return( max_cpool_size_func() );
      case  8:  return( min_ucpool_size_func() );
      case  9:  return( ame_deficit_size_func() );
      case 10:  return( ame_cores_used_func() );
      default: val.uint32 = 0; /* default fallback */
   }

   return( val );
}



static Ganglia_25metric ibmame_metric_info[] = 
{
   {0, "ame_enabled",          1200, GANGLIA_VALUE_STRING, "",      "both", "%s",   UDP_HEADER_SIZE+32, "Is AME enabled?"},
   {0, "ame_version",          1200, GANGLIA_VALUE_STRING, "",      "both", "%s",   UDP_HEADER_SIZE+32, "AME Version"},
   {0, "true_memory",            15, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "True Memory Size"},
   {0, "expanded_memory",        15, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "Expanded Memory Size"},
   {0, "target_memexp_factr",   180, GANGLIA_VALUE_FLOAT,  "",      "both", "%.2f", UDP_HEADER_SIZE+8,  "Target Memory Expansion Factor"},
   {0, "current_memexp_factr",   15, GANGLIA_VALUE_FLOAT,  "",      "both", "%.2f", UDP_HEADER_SIZE+8,  "Current Memory Expansion Factor"},
   {0, "target_cpool_size",     180, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "Target Compressed Pool Size"},
   {0, "max_cpool_size",        180, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "Max Size of Compressed Pool"},
   {0, "min_ucpool_size",       180, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "Min Size of Uncompressed Pool"},
   {0, "ame_deficit_size",      180, GANGLIA_VALUE_DOUBLE, "bytes", "both", "%.0f", UDP_HEADER_SIZE+16, "Deficit Memory Size"},
   {0, "ame_cores_used",         15, GANGLIA_VALUE_FLOAT,  "CPUs",  "both", "%.4f", UDP_HEADER_SIZE+8,  "Amount of Cores used for AME"},
   {0, NULL}
};



mmodule ibmame_module =
{
   STD_MMODULE_STUFF,
   ibmame_metric_init,
   ibmame_metric_cleanup,
   ibmame_metric_info,
   ibmame_metric_handler
};

