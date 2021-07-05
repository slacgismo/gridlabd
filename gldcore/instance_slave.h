/* File: instance_slave.h
 * Copyright (C) 2008, Battelle Memorial Institute
 */

#ifndef _INSTANCE_CNX_H
#define _INSTANCE_CNX_H

#if ! defined _GLDCORE_H && ! defined _GRIDLABD_H
#error "this header may only be included from gldcore.h or gridlabd.h"
#endif

#include <math.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <winsock2.h>
#include <windows.h>

#endif


#include <pthread.h>


#include "property.h"
#include "timestamp.h"
#include "pthread.h"
#include "linkage.h"
#include "output.h"
#include "globals.h"
#include "randomvar.h"
#include "exec.h"
#include "instance.h"

#include "gld_sock.h"

#endif

// EOF
