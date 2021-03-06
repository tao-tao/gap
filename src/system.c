/****************************************************************************
**
*W  system.c                    GAP source                       Frank Celler
*W                                                         & Martin Schönert
*W                                                         & Dave Bayer (MAC)
*W                                                  & Harald Boegeholz (OS/2)
*W                                                         & Paul Doyle (VMS)
*W                                                  & Burkhard Höfling (MAC)
*W                                                    & Steve Linton (MS/DOS)
**
**
*Y  Copyright (C)  1996,  Lehrstuhl D für Mathematik,  RWTH Aachen,  Germany
*Y  (C) 1998 School Math and Comp. Sci., University of St Andrews, Scotland
*Y  Copyright (C) 2002 The GAP Group
**
**  The  files   "system.c" and  "sysfiles.c"  contains all  operating system
**  dependent  functions.  This file contains  all system dependent functions
**  except file and stream operations, which are implemented in "sysfiles.c".
**  The following labels determine which operating system is actually used.
*/

#include "system.h"

#include "gaputils.h"
#ifdef GAP_MEM_CHECK
#include "gasman.h"
#endif
#include "sysfiles.h"
#include "sysmem.h"

#ifdef HPCGAP
#include "hpc/misc.h"
#endif

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>


#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#include <sys/time.h>                   /* definition of 'struct timeval' */
#include <sys/types.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>               /* definition of 'struct rusage' */
#endif

#ifdef SYS_IS_DARWIN
#include <mach/mach_time.h>
#endif

/****************************************************************************
**  The following function is from profile.c. We put a prototype here
**  Rather than #include "profile.h" to avoid pulling in large chunks
**  of the GAP type system
*/    
Int enableProfilingAtStartup( Char **argv, void * dummy);
Int enableMemoryProfilingAtStartup( Char **argv, void * dummy );
Int enableCodeCoverageAtStartup( Char **argv, void * dummy);

/****************************************************************************
**
*V  SyKernelVersion  . . . . . . . . . . . . . . . hard coded kernel version
** do not edit the following line. Occurrences of `4.dev' and `today'
** will be replaced by string matching by distribution wrapping scripts.
*/
const Char * SyKernelVersion = "4.dev";

/****************************************************************************
**
*F * * * * * * * * * * * command line settable options  * * * * * * * * * * *
*/

/****************************************************************************
**
*V  SyArchitecture  . . . . . . . . . . . . . . . .  name of the architecture
*/
const Char * SyArchitecture = SYS_ARCH;


/****************************************************************************
**
*V  SyCTRD  . . . . . . . . . . . . . . . . . . .  true if '<ctr>-D' is <eof>
*/
UInt SyCTRD;


/****************************************************************************
**
*V  SyCompileInput  . . . . . . . . . . . . . . . . . .  from this input file
*/
Char SyCompileInput[GAP_PATH_MAX];


/****************************************************************************
**
*V  SyCompileMagic1 . . . . . . . . . . . . . . . . . . and this magic string
*/
Char * SyCompileMagic1;


/****************************************************************************
**
*V  SyCompileName . . . . . . . . . . . . . . . . . . . . . .  with this name
*/
Char SyCompileName[256];


/****************************************************************************
**
*V  SyCompileOutput . . . . . . . . . . . . . . . . . . into this output file
*/
Char SyCompileOutput[GAP_PATH_MAX];


/****************************************************************************
**
*V  SyCompilePlease . . . . . . . . . . . . . . .  tell GAP to compile a file
*/
Int SyCompilePlease;


/****************************************************************************
**
*V  SyDebugLoading  . . . . . . . . .  output messages about loading of files
*/
Int SyDebugLoading;


/****************************************************************************
**
*V  SyGapRootPaths  . . . . . . . . . . . . . . . . . . . array of root paths
**
*/
Char SyGapRootPaths[MAX_GAP_DIRS][GAP_PATH_MAX];
#ifdef HAVE_DOTGAPRC
Char DotGapPath[GAP_PATH_MAX];
#endif

/****************************************************************************
**
*V  IgnoreGapRC . . . . . . . . . . . . . . . . . . . -r option for kernel
**
*/
Int IgnoreGapRC;

/****************************************************************************
**
*V  SyLineEdit  . . . . . . . . . . . . . . . . . . . .  support line editing
**
**  0: no line editing
**  1: line editing if terminal
**  2: always line editing (EMACS)
*/
UInt SyLineEdit;

/****************************************************************************
**
*V  SyUseReadline   . . . . . . . . . . . . . . . . . .  support line editing
**
**  Switch for not using readline although GAP is compiled with libreadline
*/
UInt SyUseReadline;

/****************************************************************************
**
*V  SyMsgsFlagBags  . . . . . . . . . . . . . . . . .  enable gasman messages
**
**  'SyMsgsFlagBags' determines whether garbage collections are reported  or
**  not.
**
**  Per default it is false, i.e. Gasman is silent about garbage collections.
**  It can be changed by using the  '-g'  option  on the  GAP  command  line.
**
**  This is used in the function 'SyMsgsBags' below.
**
**  Put in this package because the command line processing takes place here.
*/
UInt SyMsgsFlagBags;


/****************************************************************************
**
*V  SyNrCols  . . . . . . . . . . . . . . . . . .  length of the output lines
**
**  'SyNrCols' is the length of the lines on the standard output  device.
**
**  Per default this is 80 characters which is the usual width of  terminals.
**  It can be changed by the '-x' options for larger terminals  or  printers.
**
**  'Pr' uses this to decide where to insert a <newline> on the output lines.
**  'SyRead' uses it to decide when to start scrolling the echoed input line.
**
**  See also getwindowsize() below.
**
**  Put in this package because the command line processing takes place here.
*/
UInt SyNrCols;
UInt SyNrColsLocked;

/****************************************************************************
**
*V  SyNrRows  . . . . . . . . . . . . . . . . . number of lines on the screen
**
**  'SyNrRows' is the number of lines on the standard output device.
**
**  Per default this is 24, which is the  usual  size  of  terminal  screens.
**  It can be changed with the '-y' option for larger terminals or  printers.
**
**  'SyHelp' uses this to decide where to stop with '-- <space> for more --'.
**
**  See also getwindowsize() below.
*/
UInt SyNrRows;
UInt SyNrRowsLocked;

/****************************************************************************
**
*V  SyQuiet . . . . . . . . . . . . . . . . . . . . . . . . . suppress prompt
**
**  'SyQuiet' determines whether GAP should print the prompt and the  banner.
**
**  Per default its false, i.e. GAP prints the prompt and  the  nice  banner.
**  It can be changed by the '-q' option to have GAP operate in silent  mode.
**
**  It is used by the functions in 'gap.c' to suppress printing the  prompts.
**
**  Put in this package because the command line processing takes place here.
*/
UInt SyQuiet;

/****************************************************************************
**
*V  SyQuitOnBreak . . . . . . . . . . exit GAP instead of entering break loop
*/
UInt SyQuitOnBreak;

/****************************************************************************
**
*V  SyRestoring . . . . . . . . . . . . . . . . . . . . restoring a workspace
**
**  `SyRestoring' determines whether GAP is restoring a workspace or not.  If
**  it is zero no restoring should take place otherwise it holds the filename
**  of a workspace to restore.
**
*/
Char * SyRestoring;


/****************************************************************************
**
*V  SyInitializing                               set to 1 during library init
**
**  `SyInitializing' is set to 1 during the library initialization phase of
**  startup. It suppresses some behaviours that may not be possible so early
**  such as homogeneity tests in the plist code.
*/

UInt SyInitializing;


/****************************************************************************
**
*V  SyLoadSystemInitFile  . . . . . . should GAP load 'lib/init.g' at startup
*/
Int SyLoadSystemInitFile = 1;


/****************************************************************************
**
*V  SyUseModule . . . . . check for dynamic/static modules in 'READ_GAP_ROOT'
*/
int SyUseModule;


/****************************************************************************
**
*V  SyWindow  . . . . . . . . . . . . . . . .  running under a window handler
**
**  'SyWindow' is 1 if GAP  is running under  a window handler front end such
**  as 'xgap', and 0 otherwise.
**
**  If running under  a window handler front  end, GAP adds various  commands
**  starting with '@' to the output to let 'xgap' know what is going on.
*/
UInt SyWindow;


/****************************************************************************
**
*F * * * * * * * * * * * * * time related functions * * * * * * * * * * * * *
*/

/****************************************************************************
**
*F  SyTime()  . . . . . . . . . . . . . . . return time spent in milliseconds
**
**  'SyTime' returns the number of milliseconds spent by GAP so far.
**
**  Should be as accurate as possible,  because it  is  used  for  profiling.
*/
UInt SyTime ( void )
{
    struct rusage       buf;

    if ( getrusage( RUSAGE_SELF, &buf ) ) {
        fputs("gap: panic 'SyTime' cannot get time!\n",stderr);
        SyExit( 1 );
    }
    return buf.ru_utime.tv_sec*1000 + buf.ru_utime.tv_usec/1000;
}
UInt SyTimeSys ( void )
{
    struct rusage       buf;

    if ( getrusage( RUSAGE_SELF, &buf ) ) {
        fputs("gap: panic 'SyTimeSys' cannot get time!\n",stderr);
        SyExit( 1 );
    }
    return buf.ru_stime.tv_sec*1000 + buf.ru_stime.tv_usec/1000;
}
UInt SyTimeChildren ( void )
{
    struct rusage       buf;

    if ( getrusage( RUSAGE_CHILDREN, &buf ) ) {
        fputs("gap: panic 'SyTimeChildren' cannot get time!\n",stderr);
        SyExit( 1 );
    }
    return buf.ru_utime.tv_sec*1000 + buf.ru_utime.tv_usec/1000;
}
UInt SyTimeChildrenSys ( void )
{
    struct rusage       buf;

    if ( getrusage( RUSAGE_CHILDREN, &buf ) ) {
        fputs("gap: panic 'SyTimeChildrenSys' cannot get time!\n",stderr);
        SyExit( 1 );
    }
    return buf.ru_stime.tv_sec*1000 + buf.ru_stime.tv_usec/1000;
}


/****************************************************************************
**
*F * * * * * * * * * * * * * * * string functions * * * * * * * * * * * * * *
*/


#ifndef HAVE_STRLCPY

size_t strlcpy (
    char *dst,
    const char *src,
    size_t len)
{
    /* Keep a copy of the original src. */
    const char * const orig_src = src;

    /* If a non-empty len was specified, we can actually copy some data. */
    if (len > 0) {
        /* Copy up to len-1 bytes (reserve one for the terminating zero). */
        while (--len > 0) {
            /* Copy from src to dst; if we reach the string end, we are
               done and can simply return the total source string length */
            if ((*dst++ = *src++) == 0) {
                /* return length of source string without the zero byte */
                return src - orig_src - 1;
            }
        }
        
        /* If we got here, then we used up the whole buffer and len is zero.
           We must make sure to terminate the destination string. */
        *dst = 0;
    }

    /* in the end, we must return the length of the source string, no
       matter whether we completely copied or not; so advance src
       till its terminator is reached */
    while (*src++)
        ;

    /* return length of source string without the zero byte */
    return src - orig_src - 1;
}

#endif /* !HAVE_STRLCPY */


#ifndef HAVE_STRLCAT

size_t strlcat (
    char *dst,
    const char *src,
    size_t len)
{
    /* Keep a copy of the original dst. */
    const char * const orig_dst = dst;

    /* Find the end of the dst string, so that we can append after it. */
    while (*dst != 0 && len > 0) {
        dst++;
        len--;
    }

    /* We can only append anything if there is free space left in the
       destination buffer. */
    if (len > 0) {
        /* One byte goes away for the terminating zero. */
        len--;

        /* Do the actual work and append from src to dst, until we either
           appended everything, or reached the dst buffer's end. */
        while (*src != 0 && len > 0) {
            *dst++ = *src++;
            len--;
        }

        /* Terminate, terminate, terminate! */
        *dst = 0;
    }

    /* Compute the final result. */
    return (dst - orig_dst) + strlen(src);
}

#endif /* !HAVE_STRLCAT */

size_t strlncat (
    char *dst,
    const char *src,
    size_t len,
    size_t n)
{
    /* Keep a copy of the original dst. */
    const char * const orig_dst = dst;

    /* Find the end of the dst string, so that we can append after it. */
    while (*dst != 0 && len > 0) {
        dst++;
        len--;
    }

    /* We can only append anything if there is free space left in the
       destination buffer. */
    if (len > 0) {
        /* One byte goes away for the terminating zero. */
        len--;

        /* Do the actual work and append from src to dst, until we either
           appended everything, or reached the dst buffer's end. */
        while (*src != 0 && len > 0 && n > 0) {
            *dst++ = *src++;
            len--;
            n--;
        }

        /* Terminate, terminate, terminate! */
        *dst = 0;
    }

    /* Compute the final result. */
    len = strlen(src);
    if (n < len)
        len = n;
    return (dst - orig_dst) + len;
}

size_t strxcpy (
    char *dst,
    const char *src,
    size_t len)
{
    size_t res = strlcpy(dst, src, len);
    assert(res < len);
    return res;
}

size_t strxcat (
    char *dst,
    const char *src,
    size_t len)
{
    size_t res = strlcat(dst, src, len);
    assert(res < len);
    return res;
}

size_t strxncat (
    char *dst,
    const char *src,
    size_t len,
    size_t n)
{
    size_t res = strlncat(dst, src, len, n);
    assert(res < len);
    return res;
}

/****************************************************************************
**
*F  SySleep( <secs> ) . . . . . . . . . . . . . .sleep GAP for <secs> seconds
**
**  NB Various OS events (like signals) might wake us up
**
*/
void SySleep ( UInt secs )
{
  sleep( (unsigned int) secs );
}

/****************************************************************************
**
*F  SyUSleep( <msecs> ) . . . . . . . . . .sleep GAP for <msecs> microseconds
**
**  NB Various OS events (like signals) might wake us up
**
*/
void SyUSleep ( UInt msecs )
{
  usleep( (unsigned int) msecs );
}

/****************************************************************************
**
*F * * * * * * * * * * * * * initialize module * * * * * * * * * * * * * * *
*/


/****************************************************************************
**
*F  SyExit( <ret> ) . . . . . . . . . . . . . exit GAP with return code <ret>
**
**  'SyExit' is the official way  to exit GAP, bus errors are the unofficial.
**  The function 'SyExit' must perform all the necessary cleanup operations.
**  If ret is 0 'SyExit' should signal to a calling process that all is  ok.
**  If ret is 1 'SyExit' should signal a  failure  to  the  calling process.
**
**  If the user calls 'QUIT_GAP' with a value, then the global variable
**  'UserHasQUIT' will be set, and their requested return value will be
**  in 'SystemErrorCode'. If the return value would be 0, we check
**  this value and use it instead.
*/
void SyExit (
    UInt                ret )
{
        exit( (int)ret );
}

/****************************************************************************
**
*F  SyNanosecondsSinceEpoch()
**
**  'SyNanosecondsSinceEpoch' returns a 64-bit integer which represents the
**  number of nanoseconds since some unspecified starting point. This means
**  that the number returned by this function is not in itself meaningful,
**  but the difference between the values returned by two consecutive calls
**  can be used to measure wallclock time.
**
**  The accuracy of this is system dependent. For systems that implement
**  clock_getres, we could get the promised accuracy.
**
**  Note that gettimeofday has been marked obsolete in the POSIX standard.
**  We are using it because it is implemented in most systems still.
**
**  If we are using gettimeofday we cannot guarantee the values that
**  are returned by SyNanosecondsSinceEpoch to be monotonic.
**
**  Returns -1 to represent failure
**
*/
Int8 SyNanosecondsSinceEpoch(void)
{
  Int8 res;

#if defined(SYS_IS_DARWIN)
  static mach_timebase_info_data_t timeinfo;
  if ( timeinfo.denom == 0 ) {
    (void) mach_timebase_info(&timeinfo);
  }
  res = mach_absolute_time();

  res *= timeinfo.numer;
  res /= timeinfo.denom;
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    res = ts.tv_sec;
    res *= 1000000000L;
    res += ts.tv_nsec;
  } else {
    res = -1;
  }
#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval tv;

  if (gettimeofday(&tv, NULL) == 0) {
    res = tv.tv_sec;
    res *= 1000000L;
    res += tv.tv_usec;
    res *= 1000;
  } else {
    res = -1;
  };
#else
  res = -1;
#endif

  return res;
}


/****************************************************************************
**
*V  SyNanosecondsSinceEpochMethod
*V  SyNanosecondsSinceEpochMonotonic
**  
**  These constants give information about the method used to obtain
**  NanosecondsSinceEpoch, and whether the values returned are guaranteed
**  to be monotonic.
*/
#if defined(SYS_IS_DARWIN)
  const char * const SyNanosecondsSinceEpochMethod = "mach_absolute_time";
  const Int SyNanosecondsSinceEpochMonotonic = 1;
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
  const char * const SyNanosecondsSinceEpochMethod = "clock_gettime";
  const Int SyNanosecondsSinceEpochMonotonic = 1;
#elif defined(HAVE_GETTIMEOFDAY)
  const char * const SyNanosecondsSinceEpochMethod = "gettimeofday";
  const Int SyNanosecondsSinceEpochMonotonic = 0;
#else
  const char * const SyNanosecondsSinceEpochMethod = "unsupported";
  const Int SyNanosecondsSinceEpochMonotonic = 0;
#endif


/****************************************************************************
**
*F  SyNanosecondsSinceEpochResolution()
**
**  'SyNanosecondsSinceEpochResolution' returns a 64-bit integer which
**  represents the resolution in nanoseconds of the timer used for
**  SyNanosecondsSinceEpoch. 
**
**  If the return value is positive then the value has been returned
**  by the operating system can can probably be relied on. If the 
**  return value is negative it is just an estimate (as in the case
**  of gettimeofday we have no way to get the exact resolution so we
**  just pretend that the resolution is 1000 nanoseconds).
**
**  A result of 0 signifies inability to obtain any sensible value.
*/
Int8 SyNanosecondsSinceEpochResolution(void)
{
  Int8 res;

#if defined(SYS_IS_DARWIN)
  static mach_timebase_info_data_t timeinfo;
  if ( timeinfo.denom == 0 ) {
    (void) mach_timebase_info(&timeinfo);
  }
  res = timeinfo.numer;
  res /= timeinfo.denom;
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
  struct timespec ts;

  if (clock_getres(CLOCK_MONOTONIC, &ts) == 0) {
    res = ts.tv_sec;
    res *= 1000000000L;
    res += ts.tv_nsec;
  } else {
    res = 0;
  }
#elif defined(HAVE_GETTIMEOFDAY)
  res = -1000;
#else
  res = 0;
#endif

  return res;
}

/****************************************************************************
**
*F  SySetGapRootPath( <string> )  . . . . . . . . .  set the root directories
**
**  'SySetGapRootPath' takes a string and modifies a list of root directories
**  in 'SyGapRootPaths'.
**
**  A  leading semicolon in  <string> means  that the list  of directories in
**  <string> is  appended  to the  existing list  of  root paths.  A trailing
**  semicolon means they are prepended.   If there is  no leading or trailing
**  semicolon, then the root paths are overwritten.
*/


/****************************************************************************
**
*f  SySetGapRootPath( <string> )
**
** This function assumes that the system uses '/' as path separator.
** Currently, we support nothing else. For Windows (or rather: Cygwin), we
** rely on a small hack which converts the path separator '\' used there
** on '/' on the fly. Put differently: Systems that use completely different
**  path separators, or none at all, are currently not supported.
*/

static void SySetGapRootPath( const Char * string )
{
    const Char *          p;
    Char *          q;
    Int             i;
    Int             n;

    /* set string to a default value if unset                              */
    if ( string == 0 || *string == 0 ) {
        string = "./";
    }
 
    /* 
    ** check if we append, prepend or overwrite. 
    */ 
    if( string[0] == ';' ) {
        /* Count the number of root directories already present.           */
         n = 0; while( SyGapRootPaths[n][0] != '\0' ) n++;

         /* Skip leading semicolon.                                        */
         string++;

    }
    else if( string[ strlen(string) - 1 ] == ';' ) {
        /* Count the number of directories in 'string'.                    */
        n = 0; p = string; while( *p ) if( *p++ == ';' ) n++;

        /* Find last root path.                                            */
        for( i = 0; i < MAX_GAP_DIRS; i++ ) 
            if( SyGapRootPaths[i][0] == '\0' ) break;
        i--;

#ifdef HPCGAP
        n *= 2; // for each root <ROOT> we also add <ROOT/hpcgap> as a root
#endif

        /* Move existing root paths to the back                            */
        if( i + n >= MAX_GAP_DIRS ) return;
        while( i >= 0 ) {
            memcpy( SyGapRootPaths[i+n], SyGapRootPaths[i], sizeof(SyGapRootPaths[i+n]) );
            i--;
        }

        n = 0;

    }
    else {
        /* Make sure to wipe out all possibly existing root paths          */
        for( i = 0; i < MAX_GAP_DIRS; i++ ) SyGapRootPaths[i][0] = '\0';
        n = 0;
    }

    /* unpack the argument                                                 */
    p = string;
    while ( *p ) {
        if( n >= MAX_GAP_DIRS ) return;

        q = SyGapRootPaths[n];
        while ( *p && *p != ';' ) {
            *q = *p++;

#ifdef SYS_IS_CYGWIN32
            /* fix up for DOS */
            if (*q == '\\')
              *q = '/';
#endif
            
            q++;
        }
        if ( q == SyGapRootPaths[n] ) {
            strxcpy( SyGapRootPaths[n], "./", sizeof(SyGapRootPaths[n]) );
        }
        else if ( q[-1] != '/' ) {
            *q++ = '/';
            *q   = '\0';
        }
        else {
            *q   = '\0';
        }
        if ( *p ) {
            p++;
        }
        n++;
#ifdef HPCGAP
        // or each root <ROOT> we also add <ROOT/hpcgap> as a root (and first)
        if( n < MAX_GAP_DIRS ) {
            strlcpy( SyGapRootPaths[n], SyGapRootPaths[n-1], sizeof(SyGapRootPaths[n]) );
        }
        strxcat( SyGapRootPaths[n-1], "hpcgap/", sizeof(SyGapRootPaths[n-1]) );
        n++;
#endif
    }
}

/****************************************************************************
**
*F  SySetInitialGapRootPaths( <string> )  . . . . .  set the root directories
**
**  Set up GAP's initial root paths, based on the location of the
**  GAP executable.
*/
static void SySetInitialGapRootPaths(void)
{
    if (GAPExecLocation[0] != 0) {
        // GAPExecLocation might be a subdirectory of GAP root,
        // so we will go and search for the true GAP root.
        // We try stepping back up to two levels.
        char pathbuf[GAP_PATH_MAX];
        char initgbuf[GAP_PATH_MAX];
        strxcpy(pathbuf, GAPExecLocation, sizeof(pathbuf));
        for (Int i = 0; i < 3; ++i) {
            strxcpy(initgbuf, pathbuf, sizeof(initgbuf));
            strxcat(initgbuf, "lib/init.g", sizeof(initgbuf));

            if (SyIsReadableFile(initgbuf) == 0) {
                SySetGapRootPath(pathbuf);
                // escape from loop
                return;
            }
            // try up a directory level
            strxcat(pathbuf, "../", sizeof(pathbuf));
        }
    }

    // Set GAP root path to current directory, if we have no other
    // idea, and for backwards compatibility.
    // Note that GAPExecLocation must always end with a slash.
    SySetGapRootPath("./");
}

/****************************************************************************
**
*F syLongjmp( <jump buffer>, <value>)
** Perform a long jump
**
*F RegisterSyLongjmpObserver( <func> )
** Register a function to be called before longjmp is called.
** returns 1 on success, 0 if the table of functions is already full.
** This function is idempotent -- if a function is passed multiple times
** it is still only registered once.
*/

enum { signalSyLongjmpFuncsLen = 16 };

static voidfunc signalSyLongjmpFuncs[signalSyLongjmpFuncsLen];

Int RegisterSyLongjmpObserver(voidfunc func)
{
    Int i;
    for (i = 0; i < signalSyLongjmpFuncsLen; ++i) {
        if (signalSyLongjmpFuncs[i] == func) {
            return 1;
        }
        if (signalSyLongjmpFuncs[i] == 0) {
            signalSyLongjmpFuncs[i] = func;
            return 1;
        }
    }
    return 0;
}

void syLongjmp(syJmp_buf* buf, int val)
{
    Int i;
    for (i = 0; i < signalSyLongjmpFuncsLen && signalSyLongjmpFuncs[i]; ++i)
        (signalSyLongjmpFuncs[i])();
    syLongjmpInternal(*buf, val);
}

/****************************************************************************
**
*F  InitSystem( <argc>, <argv> )  . . . . . . . . . initialize system package
**
**  'InitSystem' is called very early during the initialization from  'main'.
**  It is passed the command line array  <argc>, <argv>  to look for options.
**
**  For UNIX it initializes the default files 'stdin', 'stdout' and 'stderr',
**  installs the handler 'syAnswerIntr' to answer the user interrupts
**  '<ctr>-C', scans the command line for options, sets up the GAP root paths,
**  locates the '.gaprc' file (if any), and more.
*/

typedef struct { Char symbol; UInt value; } sizeMultiplier;

sizeMultiplier memoryUnits[]= {
  {'k', 1024},
  {'K', 1024},
  {'m', 1024*1024},
  {'M', 1024*1024},
  {'g', 1024*1024*1024},
  {'G', 1024*1024*1024},
#ifdef SYS_IS_64_BIT
  {'t', 1024UL*1024*1024*1024},
  {'T', 1024UL*1024*1024*1024},
  {'p', 1024UL*1024*1024*1024*1024}, /* you never know */
  {'P', 1024UL*1024*1024*1024*1024},
#endif
};

static UInt ParseMemory( Char * s)
{
  UInt size  = atol(s);
  Char symbol =  s[strlen(s)-1];
  UInt i;
  UInt maxmem;
#ifdef SYS_IS_64_BIT
  maxmem = 15000000000000000000UL;
#else
  maxmem = 4000000000UL;
#endif
  
  for (i = 0; i < ARRAY_SIZE(memoryUnits); i++) {
    if (symbol == memoryUnits[i].symbol) {
      UInt value = memoryUnits[i].value;
      if (size > maxmem/value)
        size = maxmem;
      else
        size *= value;
      return size;
    }      
  }
  if (!IsDigit(symbol))
    FPUTS_TO_STDERR("Unrecognised memory unit ignored");
  return size;
}


struct optInfo {
  Char shortkey;
  Char longkey[50];
  Int (*handler)(Char **, void *);
  void *otherArg;
  UInt minargs;
};


static Int toggle( Char ** argv, void *Variable )
{
  UInt * variable = (UInt *) Variable;
  *variable = !*variable;
  return 0;
}

#ifdef HPCGAP
static Int storePosInteger( Char **argv, void *Where )
{
  UInt *where = (UInt *)Where;
  UInt n;
  Char *p = argv[0];
  n = 0;
  while (isdigit(*p)) {
    n = n * 10 + (*p-'0');
    p++;
  }
  if (p == argv[0] || *p || n == 0)
    FPUTS_TO_STDERR("Argument not a positive integer");
  *where = n;
  return 1;
}
#endif

static Int storeString( Char **argv, void *Where )
{
  Char **where = (Char **)Where;
  *where = argv[0];
  return 1;
}

static Int storeMemory( Char **argv, void *Where )
{
  UInt *where = (UInt *)Where;
  *where = ParseMemory(argv[0]);
  return 1;
}

static Int storeMemory2( Char **argv, void *Where )
{
  UInt *where = (UInt *)Where;
  *where = ParseMemory(argv[0])/1024;
  return 1;
}

static Int processCompilerArgs( Char **argv, void * dummy)
{
  SyCompilePlease = 1;
  strxcat( SyCompileOutput, argv[0], sizeof(SyCompileOutput) );
  strxcat( SyCompileInput, argv[1], sizeof(SyCompileInput) );
  strxcat( SyCompileName, argv[2], sizeof(SyCompileName) );
  SyCompileMagic1 = argv[3];
  return 4;
}

static Int unsetString( Char **argv, void *Where)
{
  *(Char **)Where = (Char *)0;
  return 0;
}

static Int forceLineEditing( Char **argv,void *Level)
{
  UInt level = (UInt)Level;
  SyLineEdit = level;
  return 0;
}

static Int setGapRootPath( Char **argv, void *Dummy)
{
  SySetGapRootPath( argv[0] );
  return 1;
}


#ifndef GAP_MEM_CHECK
// Provide stub with helpful error message

static Int enableMemCheck(Char ** argv, void * dummy)
{
    SyFputs( "# Error: --enableMemCheck not supported by this copy of GAP\n", 3);
    SyFputs( "  pass --enable-memory-checking to ./configure\n", 3 );
    SyExit(2);
}
#endif


static Int preAllocAmount;

/* These are just the options that need kernel processing. Additional options will be 
   recognised and handled in the library */

/* These options must be kept in sync with those in system.g, so the help output
   is correct */
struct optInfo options[] = {
  { 'B',  "architecture", storeString, &SyArchitecture, 1}, /* default architecture needs to be passed from kernel 
                                                                  to library. Might be needed for autoload of compiled files */
  { 'C',  "", processCompilerArgs, 0, 4}, /* must handle in kernel */
  { 'D',  "debug-loading", toggle, &SyDebugLoading, 0}, /* must handle in kernel */
  { 'K',  "maximal-workspace", storeMemory2, &SyStorKill, 1}, /* could handle from library with new interface */
  { 'L', "", storeString, &SyRestoring, 1}, /* must be handled in kernel  */
  { 'M', "", toggle, &SyUseModule, 0}, /* must be handled in kernel */
  { 'R', "", unsetString, &SyRestoring, 0}, /* kernel */
  { 'a', "", storeMemory, &preAllocAmount, 1 }, /* kernel -- is this still useful */
  { 'e', "", toggle, &SyCTRD, 0 }, /* kernel */
  { 'f', "", forceLineEditing, (void *)2, 0 }, /* probably library now */
  { 'E', "", toggle, &SyUseReadline, 0 }, /* kernel */
  { 'l', "roots", setGapRootPath, 0, 1}, /* kernel */
  { 'm', "", storeMemory2, &SyStorMin, 1 }, /* kernel */
  { 'r', "", toggle, &IgnoreGapRC, 0 }, /* kernel */
  { 's', "", storeMemory, &SyAllocPool, 1 }, /* kernel */
  { 'n', "", forceLineEditing, 0, 0}, /* prob library */
  { 'o', "", storeMemory2, &SyStorMax, 1 }, /* library with new interface */
  { 'p', "", toggle, &SyWindow, 0 }, /* ?? */
  { 'q', "", toggle, &SyQuiet, 0 }, /* ?? */
#ifdef HPCGAP
  { 'S', "", toggle, &ThreadUI, 0 }, /* Thread UI */
  { 'Z', "", toggle, &DeadlockCheck, 0 }, /* Thread UI */
  { 'P', "", storePosInteger, &SyNumProcessors, 1 }, /* Thread UI */
  { 'G', "", storePosInteger, &SyNumGCThreads, 1 }, /* Thread UI */
#endif
  /* The following three options must be handled in the kernel so they happen early enough */
  { 0  , "prof", enableProfilingAtStartup, 0, 1},    /* enable profiling at startup */
  { 0  , "memprof", enableMemoryProfilingAtStartup, 0, 1 }, /* enable memory profiling at startup */
  { 0  , "cover", enableCodeCoverageAtStartup, 0, 1}, /* enable code coverage at startup */
  { 0  , "quitonbreak", toggle, &SyQuitOnBreak, 0}, /* Quit GAP if we enter the break loop */
  { 0  , "enableMemCheck", enableMemCheck, 0, 0 },
  { 0, "", 0, 0, 0}};


Char ** SyOriginalArgv;
UInt SyOriginalArgc;

 

void InitSystem (
    Int                 argc,
    Char *              argv [] )
{
    Char *              *ptrlist;
    UInt                i;             /* loop variable                   */
    Int res;                       /* return from option processing function */

    /* Initialize global and static variables */
    SyCTRD = 1;             
    SyCompilePlease = 0;
    SyDebugLoading = 0;
    SyLineEdit = 1;
#ifdef HPCGAP
    SyUseReadline = 0;
#else
    SyUseReadline = 1;
#endif
    SyMsgsFlagBags = 0;
    SyNrCols = 0;
    SyNrColsLocked = 0;
    SyNrRows = 0;
    SyNrRowsLocked = 0;
    SyQuiet = 0;
    SyInitializing = 0;
#ifdef SYS_IS_64_BIT
    SyStorMax = 2048*1024L;          /* This is in kB! */
    SyAllocPool = 4096L*1024*1024;   /* Note this is in bytes! */
#else
    SyStorMax = 1024*1024L;          /* This is in kB! */
#ifdef SYS_IS_CYGWIN32
    SyAllocPool = 0;                 /* works better on cygwin */
#else
    SyAllocPool = 1536L*1024*1024;   /* Note this is in bytes! */
#endif
#endif
    SyStorOverrun = 0;
    SyStorKill = 0;
    SyStorMin = SY_STOR_MIN;         /* see system.h */
    SyUseModule = 1;
    SyWindow = 0;

    for (i = 0; i < 2; i++) {
      UInt j;
      for (j = 0; j < 7; j++) {
        SyGasmanNumbers[i][j] = 0;
      }
    }

    preAllocAmount = 4*1024*1024;
    
    /* open the standard files                                             */
    struct stat stat_in, stat_out, stat_err;
    fstat(fileno(stdin), &stat_in);
    fstat(fileno(stdout), &stat_out);
    fstat(fileno(stderr), &stat_err);

    // set up stdin
    syBuf[0].fp = fileno(stdin);
    syBuf[0].echo = fileno(stdout);
    syBuf[0].bufno = -1;
    syBuf[0].isTTY = isatty(fileno(stdin));
    if (syBuf[0].isTTY) {
        // if stdin is on a terminal, make sure stdout in on the same terminal
        if (stat_in.st_dev != stat_out.st_dev ||
            stat_in.st_ino != stat_out.st_ino)
            syBuf[0].echo = open( ttyname(fileno(stdin)), O_WRONLY );
    }

    // set up stdout
    syBuf[1].echo = syBuf[1].fp = fileno(stdout); 
    syBuf[1].bufno = -1;
    syBuf[1].isTTY = isatty(fileno(stdout));

    // set up errin (defaults to stdin, unless stderr is on a terminal)
    syBuf[2].fp = fileno(stdin);
    syBuf[2].echo = fileno(stderr);
    syBuf[2].bufno = -1;
    syBuf[2].isTTY = isatty(fileno(stderr));
    if (syBuf[2].isTTY) {
        // if stderr is on a terminal, make sure errin in on the same terminal
        if (stat_in.st_dev != stat_err.st_dev ||
            stat_in.st_ino != stat_err.st_ino)
            syBuf[2].fp = open( ttyname(fileno(stderr)), O_RDONLY );
    }

    // set up errout
    syBuf[3].echo = syBuf[3].fp = fileno(stderr);
    syBuf[3].bufno = -1;

    // turn off buffering
    setbuf(stdin, (char *)0);
    setbuf(stdout, (char *)0);
    setbuf(stderr, (char *)0);

    for (i = 4; i < ARRAY_SIZE(syBuf); i++)
        SyMarkBufUnused(i);

    for (i = 0; i < ARRAY_SIZE(syBuffers); i++)
        syBuffers[i].inuse = 0;

#ifdef HAVE_LIBREADLINE
    rl_initialize ();
#endif
    
    SyInstallAnswerIntr();

#if defined(SYS_DEFAULT_PATHS)
    SySetGapRootPath( SYS_DEFAULT_PATHS );
#else
    SySetInitialGapRootPaths();
#endif

    /* save the original command line for export to GAP */
    SyOriginalArgc = argc;
    SyOriginalArgv = argv;

    /* scan the command line for options that we have to process in the kernel */
    /* we just scan the whole command line looking for the keys for the options we recognise */
    /* anything else will presumably be dealt with in the library */
    while ( argc > 1 )
      {
        if (argv[1][0] == '-' ) {

          if ( strlen(argv[1]) != 2 && argv[1][1] != '-') {
            FPUTS_TO_STDERR("gap: sorry, options must not be grouped '");
            FPUTS_TO_STDERR(argv[1]);  FPUTS_TO_STDERR("'.\n");
            goto usage;
          }


          for (i = 0;  options[i].shortkey != argv[1][1] &&
                       (argv[1][1] != '-' || argv[1][2] == 0 || strcmp(options[i].longkey, argv[1] + 2)) &&
                       (options[i].shortkey != 0 || options[i].longkey[0] != 0); i++)
            ;

        


          if (argc < 2 + options[i].minargs)
            {
              Char buf[2];
              FPUTS_TO_STDERR("gap: option "); FPUTS_TO_STDERR(argv[1]);
              FPUTS_TO_STDERR(" requires at least ");
              buf[0] = options[i].minargs + '0';
              buf[1] = '\0';
              FPUTS_TO_STDERR(buf); FPUTS_TO_STDERR(" arguments\n");
              goto usage;
            }
          if (options[i].handler) {
            res = (*options[i].handler)(argv+2, options[i].otherArg);
            
            switch (res)
              {
              case -1: goto usage;
                /*            case -2: goto fullusage; */
              default: ;     /* fall through and continue */
              }
          }
          else
            res = options[i].minargs;
          /*    recordOption(argv[1][1], res,  argv+2); */
          argv += 1 + res;
          argc -= 1 + res;
          
        }
        else {
          argv++;
          argc--;
        }
          
      }
    /* adjust SyUseReadline if no readline support available or for XGAP  */
#if !defined(HAVE_LIBREADLINE)
    SyUseReadline = 0;
#endif
    if (SyWindow) SyUseReadline = 0;

    /* now that the user has had a chance to give -x and -y,
       we determine the size of the screen ourselves */
    getwindowsize();

    /* fix max if it is lower than min                                     */
    if ( SyStorMax != 0 && SyStorMax < SyStorMin ) {
        SyStorMax = SyStorMin;
    }

    /* fix pool size if larger than SyStorKill */
    if ( SyStorKill != 0 && SyAllocPool != 0 &&
                            SyAllocPool > 1024 * SyStorKill ) {
        SyAllocPool = SyStorKill * 1024;
    }
    /* fix pool size if it is given and lower than SyStorMax */
    if ( SyAllocPool != 0 && SyAllocPool < SyStorMax * 1024) {
        SyAllocPool = SyStorMax * 1024;
    }

    /* when running in package mode set ctrl-d and line editing            */
    if ( SyWindow ) {
      /*         SyLineEdit   = 1;
                 SyCTRD       = 1; */
        syBuf[2].fp = syBuf[0].fp;  syBuf[2].echo = syBuf[0].echo;
        syBuf[2].isTTY = syBuf[0].isTTY;
        syBuf[3].fp = syBuf[1].fp;
        syWinPut( 0, "@p", "1." );
    }
   

    if (SyAllocPool == 0) {
      /* premalloc stuff                                                     */
      /* allocate in small chunks, and write something to them
       * (the GNU clib uses mmap for large chunks and give it back to the
       * system after free'ing; also it seems that memory is only really 
       * allocated (pagewise) when it is first used)                     */
      ptrlist = (Char **)malloc((1+preAllocAmount/1000)*sizeof(Char*));
      for (i = 1; i*1000 < preAllocAmount; i++) {
        ptrlist[i-1] = (Char *)malloc( 1000 );
        if (ptrlist[i-1] != NULL) ptrlist[i-1][900] = 13;
      }
      for (i = 1; (i+1)*1000 < preAllocAmount; i++) 
        if (ptrlist[i-1] != NULL) free(ptrlist[i-1]);
      free(ptrlist);
       
     /* ptr = (Char *)malloc( preAllocAmount );
      ptr1 = (Char *)malloc(4);
      if ( ptr != 0 )  free( ptr ); */
    }

    /* should GAP load 'init/lib.g' on initialization */
    if ( SyCompilePlease || SyRestoring ) {
        SyLoadSystemInitFile = 0;
    }

    /* the compiler will *not* read in the .gaprc file                     
    if ( gaprc && ! ( SyCompilePlease || SyRestoring ) ) {
        sySetGapRCFile();
    }
    */

#ifdef HAVE_DOTGAPRC
    /* the users home directory                                            */
    if ( getenv("HOME") != 0 ) {
        strxcpy(DotGapPath, getenv("HOME"), sizeof(DotGapPath));
# if defined(SYS_IS_DARWIN) && SYS_IS_DARWIN
        /* On Darwin, add .gap to the sys roots, but leave */
        /* DotGapPath at $HOME/Library/Preferences/GAP     */
        strxcat(DotGapPath, "/.gap;", sizeof(DotGapPath));
        if (!IgnoreGapRC) {
          SySetGapRootPath(DotGapPath);
        }
		
        strxcpy(DotGapPath, getenv("HOME"), sizeof(DotGapPath));
        strxcat(DotGapPath, "/Library/Preferences/GAP;", sizeof(DotGapPath));
# elif defined(__CYGWIN__)
        strxcat(DotGapPath, "/_gap;", sizeof(DotGapPath));
# else
        strxcat(DotGapPath, "/.gap;", sizeof(DotGapPath));
# endif

        if (!IgnoreGapRC) {
          SySetGapRootPath(DotGapPath);
        }
        DotGapPath[strlen(DotGapPath)-1] = '\0';
        
        /* and in this case we can also expand paths which start
           with a tilde ~ */
        Char userhome[GAP_PATH_MAX];
        strxcpy(userhome, getenv("HOME"), sizeof(userhome));
        const UInt userhomelen = strlen(userhome);
        for (i = 0; i < MAX_GAP_DIRS && SyGapRootPaths[i][0]; i++) {
            const UInt pathlen = strlen(SyGapRootPaths[i]);
            if (SyGapRootPaths[i][0] == '~' &&
                userhomelen + pathlen < sizeof(SyGapRootPaths[i])) {
                SyMemmove(SyGapRootPaths[i] + userhomelen,
                        /* don't copy the ~ but the trailing '\0' */
                        SyGapRootPaths[i] + 1, pathlen);
                memcpy(SyGapRootPaths[i], userhome, userhomelen);
          }
        }
    }
#endif


    /* now we start                                                        */
    return;

    /* print a usage message                                               */
usage:
 FPUTS_TO_STDERR("usage: gap [OPTIONS] [FILES]\n");
 FPUTS_TO_STDERR("       run the Groups, Algorithms and Programming system, Version ");
 FPUTS_TO_STDERR(SyBuildVersion);
 FPUTS_TO_STDERR("\n");
 FPUTS_TO_STDERR("       use '-h' option to get help.\n");
 FPUTS_TO_STDERR("\n");
 SyExit( 1 );
}
