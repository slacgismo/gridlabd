/* File: gldcore.h 
   Copyright (C) 2019, Regents of the Leland Stanford Junior University

   The header <gldcore.h> is included only by core code. This header includes all
   the headers needed by the code.
 */

#ifndef _GLDCORE_H
#define _GLDCORE_H

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <sys/errno.h>

#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>

#include "gld_sock.h"


#if HAVE_SCHED_H
#include <sched.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef FALSE

/*	Define: FALSE
		The value of false (0)
 */
#define FALSE (0)

/*	Define: TRUE
		The value of true (!0)
 */
#define TRUE (!FALSE)

#endif

/*	Typedef: STATUS
		See <e_status>

	Enum: e_status
		General status value

	Member: 
		FAILED - Equivalent to <FALSE>
		SUCCESS - Equivalent to <TRUE>
 */
typedef enum e_status {FAILED=FALSE, SUCCESS=TRUE} STATUS;

#include "aggregate.h"
#include "build.h"
#include "class.h"
#include "cmdarg.h"
#include "compare.h"
#include "complex.h"
#include "console.h"
#include "convert.h"
#include "curl.h"
#include "daemon.h"
#include "debug.h"
#include "deltamode.h"
#include "enduse.h"
#include "environment.h"
#include "exception.h"
#include "exec.h"
#include "find.h"
#include "globals.h"
#include "gui.h"
#include "http_client.h"
#include "index.h"
#include "instance.h"
#include "instance_cnx.h"
#include "instance_slave.h"
#include "interpolate.h"
#include "job.h"
#include "json.h"
#include "kill.h"
#include "kml.h"
#include "legal.h"
#include "link.h"
#include "linkage.h"
#include "list.h"
#include "load.h"
#include "loadshape.h"
#include "local.h"
#include "lock.h"
#include "main.h"
#include "match.h"
#include "matlab.h"
#include "module.h"
#include "object.h"
#include "output.h"
#include "platform.h"
#include "property.h"
#include "python_embed.h"
#include "random.h"
#include "python_property.h"
#include "random.h"
#include "realtime.h"
#include "sanitize.h"
#include "save.h"
#include "schedule.h"
#include "server.h"
#include "setup.h"
#include "stream.h"
#include "test.h"
#include "threadpool.h"
#include "timestamp.h"
#include "transform.h"
#include "ufile.h"
#include "unit.h"
#include "validate.h"
#include "version.h"

#endif

