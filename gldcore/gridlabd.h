/* 	File: gridlabd.h 
 	Copyright (C) 2008, Battelle Memorial Institute

 	The header <gridlabd.h> is included only by modules. It includes all the needed
 	headers and defines the API macros, functions, and classes.

	The runtime module API links the GridLAB-D core to modules that are created to
	perform various modeling tasks.  The core interacts with each module according
	to a set script that determines which exposed module functions are called and
	when.  The general sequence of calls is as follows:
	
	Registration - A module registers the object classes it implements and
	registers the variables that each class publishes.
	
	Creation - The core calls object creation functions during the model
	load operation for each object that is created.  Basic initialization can be
	completed at this point.
	
	Definition - The core sets the values of all published variables that have
	been specified in the model being loaded.  After this is completed, all references
	to other objects have been resolved.
	
	Validation - The core gives the module an opportunity to check the model
	before initialization begins.  This gives the module an opportunity to validate
	the model and reject it or fix it if it fails to meet module-defined criteria.
	
	Initialization - The core calls the final initialization procedure with
	the object's full context now defined.  Properties that are derived based on the
	object's context should be initialized only at this point.
	
	Synchronization - This operation is performed repeatedly until every object
	reports that it no longer expects any state changes.  The return value from a
	synchronization operation is the amount time expected to elapse before the object's
	next state change.  The side effect of a synchronization is a change to one or
	more properties of the object, and possible an update to the property of another
	object.

	Note that object destruction is not supported at this time.

	 GridLAB-D modules usually require a number of functions to access data and interaction
	 with the core.  These include

	 - memory locking,
	 - memory exception handlers,
	 - variable publishers,
	 - output functions,
	 - management routines,
	 - module management,
	 - class registration,
	 - object management,
	 - property management,
	 - object search,
	 - random number generators, and
	 - time management.
 */

#ifndef _GRIDLABD_H
#define _GRIDLABD_H

// Define: MAJOR
// 	Core major version (see <REV_MAJOR>)
#define MAJOR REV_MAJOR

// Define: MINOR
//	Core minor version (see <REV_MINOR>)
#define MINOR REV_MINOR

#define STREAM_MODULE

#ifndef _GLDCORE_H

#ifndef FALSE
// Define: FALSE
// FALSE = (0)
#define FALSE (0)
#endif

#ifndef TRUE
// Define: TRUE
// TRUE = (!FALSE)
#define TRUE (!FALSE)
#endif

/*	Typedef: STATUS
		See <e_status>

	Enum: s_status
	FAILED = FALSE - status indicating failure 
	SUCCESS = TRUE - status indicating success
 */
typedef enum e_status {FAILED=FALSE, SUCCESS=TRUE} STATUS;

#endif

#include <arpa/inet.h>
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

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <direct.h>
#include <io.h>
#include <windows.h>
#include <winbase.h>
#include <winsock2.h>
#include <io.h>
#include <process.h>
#ifndef __MINGW__
#define snprintf _snprintf
#endif
#else
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include <vector>

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
#include "gld_sock.h"
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
#include "random.h"
#include "realtime.h"
#include "sanitize.h"
#include "save.h"
#include "schedule.h"
#include "server.h"
#include "setup.h"
#include "stream.h"
#include "threadpool.h"
#include "timestamp.h"
#include "transform.h"
#include "ufile.h"
#include "unit.h"
#include "validate.h"
#include "version.h"

#ifdef DLMAIN
#define EXTERN
#define INIT(X) = X
#else
#define EXTERN extern
#define INIT(X)
#endif

/*	Structure: callback

	Core function callback table
 */
EXTERN DEPRECATED CALLBACKS *callback INIT(NULL);

#ifndef MODULENAME

/*	Define: MODULENAME()

	Expands to the (const char*) name of the module that implements an object.

	This macro is obsolete.
 */
#define MODULENAME(obj) DEPRECATED (obj->oclass->module->name)

#endif

/* 	Section: Version information

 	Version information is stored in <version.h> and build.h, 
	which is generated by the makefile.
 */

/*	Define: gl_version_major

	Returns core major version number (see <global_version_major>)

	This macro is obsolete.
 */
#define gl_version_major DEPRECATED (*callback->version.major)

/*	Define: gl_version_minor 

	Returns core minor version number (see <global_version_minor>)

	This macro is obsolete.
 */
#define gl_version_minor DEPRECATED (*callback->version.minor)

/*	Define: gl_version_patch

	Returns core patch number (see <global_version_patch>)

	This macro is obsolete.
 */
#define gl_version_patch DEPRECATED (*callback->version.patch)

/*	Define: gl_version_build
	
	Returns core build number (see <global_version_build>)

	This macro is obsolete.
 */
#define gl_version_build DEPRECATED (*callback->version.build)

/*	Define: gl_version_branch

	Returns core branch name (see <global_version_branch>)

	This macro is obsolete.
 */
#define gl_version_branch DEPRECATED (*callback->version.branch)

/*	Section: Variable Publishing

	Modules must register public variables that are accessed by other modules, and the core by publishing them.

	The typical modules will register a class, and immediately publish the variables supported by the class:
	--- C++ Code ---
	EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
	{
		extern CLASS* node_class; // defined globally in the module
		if (set_callback(fntable)==NULL)
		{
			errno = EINVAL;
			return NULL;
		}

		node_class = gl_register_class(module,"node",sizeof(node),PC_BOTTOMUP);
		PUBLISH_CLASS(node,complex,V);
		PUBLISH_CLASS(node,complex,S);

		return node_class; // always return the *first* class registered
	}
	--- End Code ---
 */

/*	Define: PUBLISH_STRUCT(clsas,type,name)

	This macro is obsolete.
 */
#define PUBLISH_STRUCT(C,T,N) DEPRECATED {struct C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}

/*	Define: PUBLISH_CLASS(class,type,name)

	The PUBLISH_CLASS macro is used to publish a member of a class (C++ only).

	This macro is obsolete.

 */
#define PUBLISH_CLASS(C,T,N) DEPRECATED {class C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}

/*	Define: PUBLISH_CLASSX(class,type,name)

	The PUBLISH_CLASSX macro is used to publish a member of a class (C++ only) using a different name from the member name.

	This macro is obsolete.
*/
#define PUBLISH_CLASSX(C,T,N,V) DEPRECATED {class C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,V,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}

/*	Define: PUBLISH_DELEGATED(class,type,name)

	The PUBLISH_DELEGATED macro is used to publish a variable that uses a delegated type.

	This macro is obsolete.
**/

#ifdef __cplusplus
#define PADDR(X) PADDR_X(X,this)
#endif

/*	Section: Exception handling

	Module exception handling is provided for modules implemented in C to perform exception handling,
	as well to allow C++ code to throw exceptions to the core's main exception handler.

	Typical use is like this:

	--- C++ CODE ---
	#include <errno.h>
	#include <string.h>
	GL_TRY {

		// block of code

		// exception
		if (errno!=0)
			GL_THROW("Error condition %d: %s", errno, strerror(errno));

		// more code

	} GL_CATCH(char *msg) {

		// exception handler

	} GL_ENDCATCH;
	--- END CODE ---

	Note: it is ok to use GL_THROW inside a C++ catch statement.  This behavior is defined
	(unlike using C++ throw inside C++ catch) because GL_THROW is implemented using longjmp().

	See Also:
	throw_exception - "Exception handling" for detail on the message format conventions.
 */

/*	Define: GL_TRY

	You may create your own #GL_TRY block and throw exception using GL_THROW(Msg,...) within
	the block.  Declaring this block will change the behavior of GL_THROW(Msg,...) only
	within the block.  Calls to GL_THROW(Msg,...) within this try block will be transfer
	control to the GL_CATCH(Msg) block.
 */
#define GL_TRY DEPRECATED { EXCEPTIONHANDLER *_handler = (*callback->exception.create_exception_handler)(); if (_handler==NULL) (*callback->output_error)("%s(%d): module exception handler creation failed",__FILE__,__LINE__); else if (setjmp(_handler->buf)==0) {
/* TROUBLESHOOT
	This error is caused when the system is unable to implement an exception handler for a module.
	This is an internal error and should be reported to the module developer.
 */

/*	Define: GL_THROW(message,...)

	The behavior of GL_THROW(Msg,...) differs depending on the situation:
	
	- Inside a <GL_TRY> block, program flow is transfered to the <GL_CATCH(message)> block that follows.
	- Inside a <GL_CATCH(message)> block, <GL_THROW(message,...)> behavior is undefined.
	- Outside a <GL_TRY> or <GL_CATCH(message)> block, control is transfered to the main core exception handler.

	This function is obsolete. Use throw() instead.
 */
#ifdef __cplusplus
inline DEPRECATED void GL_THROW(const char *format, ...)
{
	static char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	throw (const char*) buffer;
}
#else
#define GL_THROW DEPRECATED (*callback->exception.throw_exception)

/*	Define: GL_CATCH(message)

	The argument \p msg provides access to the exception message thrown.
	Otherwise, GL_CATCH(Msg) blocks function like all other code blocks.

	The behavior of GL_THROW(Msg) is not defined inside GL_CATCH(Msg) blocks.

	GL_CATCH blocks must always be terminated by a #GL_ENDCATCH statement.

	This function is obsolete. Use catch() instead.
 */
#endif
#define GL_CATCH(Msg) DEPRECATED } else {Msg = (*callback->exception.exception_msg)();

/*	Define: GL_ENDCATCH

	Each GL_CATCH(Msg) block must be terminated by a <GL_ENDCATCH> statement.
 */
#define GL_ENDCATCH } DEPRECATED (*callback->exception.delete_exception_handler)(_handler);}

/*	Section: Output functions

	Output to the <gldcore> stream is handled by the <gl_verbose>, <gl_output>, 
	<gl_warning>, <gl_error>, and <gl_debug> functions

	Modules should use these function. Classes may use these functions, or use
	the output functions in the parent <GldObject> class.
 */

/*	Define: gl_verbose

	Outputs a message to the verbose stream when <global_verbose> is <TRUE>.
	See <output_verbose>.
 */
#define gl_verbose (*callback->output_verbose)

/*	Define: gl_output

	Outputs a message to the output stream when <global_output> is <TRUE>.
	See <output_message>.
 */
 #define gl_output (*callback->output_message)

/*	Define: gl_warning

	Outputs a message to the warning stream when <global_warning> is <TRUE>.
	See <output_warning>.
 */
 #define gl_warning (*callback->output_warning)

/*	Define: gl_error

	Outputs a message to the warning stream when <global_quiet> is <FALSE>.	
	See <output_error>.
 */
 #define gl_error (*callback->output_error)

/*	Define: gl_debug

	Outputs a message to the debug stream when <global_debug> is <TRUE>.
	See <output_debug>.	
 */
 #define gl_debug (*callback->output_debug)

/*	Define: gl_testmsg

	Outputs a message to the test stream. See <output_test>.
 */
 #define gl_testmsg (*callback->output_test)

/*	Section: Memory allocation

	The core is responsible for managing any memory that is shared.  Use these
	macros to manage the allocation of objects that are registered classes.
 */

/*	Define: gl_malloc

	Allocate a block of memory from the core's heap.
	This is necessary for any memory that the core will have to manage.

	See Also:
	- <module_malloc>
	- <module_free>
*/
#define gl_malloc DEPRECATED (*callback->malloc)

/*	Section: Core linkage

	Most module function use core library functions and variables.
	These macros give access the core library and other global module variables.

	This function is obsolete.
 */

/*	Define: set_callback

	Defines the callback table for the module.
	Callback function provide module with direct access to important core functions.
 */
#define set_callback(CT) DEPRECATED (callback=(CT))

/*	Define: gl_get_module_var

	Provides access to a global module variable.
	
	This function is obsolete.

	See Also:
	- <global_getvar>
	- <global_setvar>
 */
#define gl_get_module_var DEPRECATED (*callback->module.getvar)

/*	Define: gl_findfile

	Provides file search function. See <find_file>.
	
	This function is obsolete.
 */
#define gl_findfile DEPRECATED (*callback->file.find_file)

/*	Define: gl_find_module

	Find a module. See <module_find>.
	
	This function is obsolete.
 */
#define gl_find_module DEPRECATED (*callback->module_find)

/*	Define: gl_find_property

	Find a property. See <class_find_property>.
	
	This function is obsolete.
 */
#define gl_find_property DEPRECATED (*callback->find_property)

#ifdef __cplusplus

/*	Function: gl_module_depends

	Declare a module dependency.  This will automatically load
    the module if it is not already loaded. See <module_depends>.
	
	This function is obsolete.
 */
inline DEPRECATED int gl_module_depends(char *name, /**< module name */
							 unsigned char major=0, /**< major version, if any required (module must match exactly) */
							 unsigned char minor=0, /**< minor version, if any required (module must be greater or equal) */
							 unsigned short build=0) /**< build number, if any required (module must be greater or equal) */
{
	return (*callback->module.depends)(name,major,minor,build);
}
#else
#define gl_module_depends DEPRECATED (*callback->module.depends)
#endif

/*	Define: gl_module_getfirst

	See <module_get_first>.
	
	This function is obsolete.
 */
#define gl_module_getfirst DEPRECATED (*callback->module.getfirst)

/*	Section: Class access

	Classes are used to make sure the core knows how objects are implemented
	in modules.  Use the class access macros to create, use, and destroy classes.
	
	This function is obsolete.
 */

#ifdef __cplusplus

/*	Function: gl_register_class

	Allow an object class to be registered with the core.
	Note that C file may publish structures, even they are not implemented as classes.
	See <class_register>
	
	This function is obsolete.
 */
inline DEPRECATED CLASS *gl_register_class(MODULE *mod,const char *name,size_t size,unsigned int options)
{
	return (*callback->register_class)(mod,name,size,PASSCONFIG(options));
}

#else

#define gl_register_class DEPRECATED (*callback->register_class)

#endif

/*	Define: gl_class_get_first
	
	This function is obsolete.
 */
#define gl_class_get_first DEPRECATED (*callback->class_getfirst)

/*	Define: gl_class_get_by_name
	
	This function is obsolete.
 */
#define gl_class_get_by_name DEPRECATED (*callback->class_getname)

/*	Section: Object management

	Object management macros are create to allow modules to create, test,
	control ranks, and reveal members of objects and registered classes.

 */

/*	Define: gl_create_object

	Creates an object by allocating memory from core heap and setting default values.
	See <object_create_single>

	This function is obsolete.
 */
#define gl_create_object DEPRECATED (*callback->create.single)

/*	Define: gl_create_array

	Creates an array of objects on core heap.
	See <object_create_array>

	This function is obsolete.
 */
#define gl_create_array DEPRECATED (*callback->create.array)

/*	Define: gl_create_foreign

	This function is obsolete.
	See <object_create_array>
 */
#define gl_create_foreign DEPRECATED (*callback->create.foreign)

#ifdef __cplusplus

/*	Define: object_isa

	Checks the type (and supertypes) of an object.
	See <object_isa>

	This function is obsolete.
 */
inline DEPRECATED bool gl_object_isa(OBJECT *obj, /**< object to test */
						  const char *type,
						  const char *modname=NULL) /**< type to test */
{	
	return (*callback->object_isa)(obj,type,modname)!=0;
}

#else

#define gl_object_isa DEPRECATED (*callback->object_isa)

#endif

/*	Define: gl_publish_variable

	Declare an object property as publicly accessible. See <class_define_map>.

	This function is obsolete.
 */
#define gl_publish_variable DEPRECATED (*callback->define_map)

/*	Define: gl_publish_loadmethod

	Declare a loader method based on a property name.  See <class_add_loadmethod>.

	This function is obsolete.
 */
#define gl_publish_loadmethod DEPRECATED (*callback->loadmethod)

#ifdef __cplusplus

/*	Function: gl_publish_function

	Publishes an object function.  This is currently unused.
	See <object_define_function>

	This function is obsolete.
 */
inline DEPRECATED FUNCTION *gl_publish_function(CLASS *oclass, /**< class to which function belongs */
									 FUNCTIONNAME functionname, /**< name of function */
									 FUNCTIONADDR call) /**< address of function entry */
{ 
	return (*callback->function.define)(oclass, functionname, call);
}

/*	Define: gl_get_function

	Reference the function by the name given.

	This function is obsolete.
 */
inline DEPRECATED FUNCTIONADDR gl_get_function(OBJECT *obj, FUNCTIONNAME name)
{ 
	return obj?(*callback->function.get)(obj->oclass->name,name):NULL;
}

#else

#define gl_publish_function DEPRECATED (*callback->function.define)
#define gl_get_function DEPRECATED (*callback->function.get)

#endif

#ifdef __cplusplus

/*	Function: gl_set_dependent

	Changes the dependency rank of an object.
	Normally dependency rank is determined by the object parent,
	but an object's rank may be increased using this call.
	An object's rank may not be decreased.
	See <object_set_rank>, <object_set_parent>

	This function is obsolete.
 */
inline DEPRECATED int gl_set_dependent(OBJECT *obj, /**< object to set dependency */
							OBJECT *dep) /**< object dependent on */
{ 
	return (*callback->object.set_dependent)(obj,dep);
}

#else

#define gl_set_dependent DEPRECATED (*callback->object.set_dependent)

#endif

#ifdef __cplusplus

/*	Function: gl_set_parent

	Establishes the rank of an object relative to another object (it's parent).
	When an object is parent to another object, it's rank is always greater.
	Object of higher rank are processed first on top-down passes,
	and later on bottom-up passes.
	Objects of the same rank may be processed in parallel,
	if system resources make it possible.
	See <object_set_rank>, <object_set_parent>

	This function is obsolete.
 */
inline DEPRECATED int gl_set_parent(OBJECT *obj, /**< object to set parent of */
						 OBJECT *parent) /**< parent object */
{ 
	return (*callback->object.set_parent)(obj,parent);
}

#else

#define gl_set_parent DEPRECATED (*callback->object.set_parent)

#endif

#ifdef __cplusplus

/*	Function: gl_set_rank

	Adjusts the rank of an object relative to another object (it's parent).
	When an object is parent to another object, it's rank is always greater.
	Object of higher rank are processed first on top-down passes,
	and later on bottom-up passes.
	Objects of the same rank may be processed in parallel,
	if system resources make it possible.
	See <object_set_rank>, <object_set_parent>

	This function is obsolete.
 */
inline DEPRECATED int gl_set_rank(OBJECT *obj, /**< object to change rank */
					   int rank)	/**< new rank of object */
{ 
	return (*callback->object.set_rank)(obj,rank);
}

#else

#define gl_set_rank DEPRECATED (*callback->object.set_rank)

#endif

/*	Define: gl_object_get_first
	See <object_get_first>

	This function is obsolete.
 */
#define gl_object_get_first DEPRECATED (*callback->object.get_first)

/*	Define: gl_object_find_by_id
	See <objecct_find_by_id

	This function is obsolete.
 */
#define gl_object_find_by_id DEPRECATED (*callback->object_find_by_id)

/*	Section: Property management

	Use the property management functions to provide and gain access to published
	variables from other modules.  This include getting property information
	and unit conversion.

 */

/*	Define: gl_register_type

	Create an object. See <class_register_type>.

	This function is obsolete.
 */
#define gl_register_type DEPRECATED (*callback->register_type)

/*	Define: gl_class_add_extended_property

	See <class_add_extended_property>.

	This function is obsolete.
 */
#define gl_class_add_extended_property DEPRECATED (*callback->class_add_extended_property)

/*	Define: gl_publish_delegate

	Publish an delegate property type for a class.
	See <class_define_type>.

	This function is obsolete.
 */
#define gl_publish_delegate DEPRECATED (*callback->define_type)

#ifdef __cplusplus

/*	Function: gl_get_property

	Get a property of an object.
	See <object_get_property>.

	This function is obsolete.
 */
inline DEPRECATED PROPERTY *gl_get_property(OBJECT *obj, /**< a pointer to the object */
								 PROPERTYNAME name, /**< the name of the property */
								 PROPERTYSTRUCT *part=NULL) /**< part info */
{ 
	return (*callback->properties.get_property)(obj,name,part); 
}

#else

#define gl_get_property DEPRECATED (*callback->properties.get_property)

#endif

#ifdef __cplusplus

/*	Function: gl_get_value

	Get the value of a property in an object
	See <object_get_value_by_addr>.

	This function is obsolete.
 */
inline DEPRECATED int gl_get_value(OBJECT *obj, /**< the object from which to get the data */
						void *addr, /**< the addr of the data to get */
						char *value, /**< the buffer to which to write the result */
						int size, /**< the size of the buffer */
						PROPERTY *prop=NULL) /**< the property to use or NULL if unknown */
{ 
	return (*callback->properties.get_value_by_addr)(obj,addr,value,size,prop);
}

#else

#define gl_get_value DEPRECATED (*callback->properties.get_value_by_addr)

#endif

/*	Define: gl_set_value_by_type

	See <object_set_value_by_type>.

	This function is obsolete.
 */
#define gl_set_value_by_type DEPRECATED (*callback->properties.set_value_by_type)

#ifdef __cplusplus

/*	Function: gl_set_value
	Parameters:
	obj - object to alter
	addr - addr of property to set
	value - value to set
	prop - property hint to use

	Set the value of a property in an object.
	See <object_set_value_by_addr>.

	This function is obsolete.	
 */
inline DEPRECATED int gl_set_value(OBJECT *obj,
						void *addr,
						char *value,
						PROPERTY *prop)
{ 
	return (*callback->properties.set_value_by_addr)(obj,addr,value,prop);
}

#else

#define gl_set_value DEPRECATED (*callback->properties.set_value_by_addr)

#endif

DEPRECATED char* gl_name(OBJECT *my, char *buffer, size_t size);

#ifdef __cplusplus

/*	Function: gl_set_value
	Parameters:
	obj - the object whose property value is being obtained
	prop - the name of the property being obtained
	value - a reference to the local value where the property's value is being copied

	Set the typed value of a property. 

	This function is obsolete.
 */
template <class T> 
inline DEPRECATED int gl_set_value(OBJECT *obj, 
											PROPERTY *prop,
											T &value)
{
	char buffer[256];

	T *ptr = (T *)((char *)(obj + 1) + (int64)(prop->addr)); 
	// TODO: it would be a good idea to check the property type here
	if (ptr==NULL)
	{
		gl_error("property %s not found in object %s", prop->name, gl_name(obj, buffer, 255));
		return 0;
	}
	if ( obj->oclass->notify )
	{
		if ( obj->oclass->notify(obj,NM_PREUPDATE,prop) == 0 ) 
		{
			gl_error("preupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
			return 0;
		}
	}
	*ptr = value;
	if ( obj->oclass->notify ) 
	{
		if ( obj->oclass->notify(obj,NM_POSTUPDATE,prop) == 0 ) 
		{
			gl_error("postupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
			return 0;
		}
	}
	return 1;
}
#endif

/*	Define: gl_get_reference

	Get a reference to another object.
	See <object_get_reference>.
 */
#define gl_get_reference DEPRECATED (*callback->properties.get_reference)

#ifdef __cplusplus

/*	Function: gl_get_value_by_name

	Get the value of a property in an object.
	See <object_get_value_by_name>.

	This function is obsolete.
 */
inline DEPRECATED int gl_get_value_by_name(OBJECT *obj,
								PROPERTYNAME name,
								char *value,
								int size)
{ 
	return (*callback->properties.get_value_by_name)(obj,name,value,size);
}

#else

#define gl_get_value_by_name DEPRECATED (*callback->properties.get_value_by_name)

#endif

#ifdef __cplusplus

/*	Function: gl_getvalue
	
	See <gl_get_value_by_name>.

	This function is obsolete.
 */
inline DEPRECATED char *gl_getvalue(OBJECT *obj,
						 PROPERTYNAME name, char *buffer, int sz)
{
	return gl_get_value_by_name(obj,name,buffer,sz)>=0 ? buffer : NULL;
}
#endif

/** Set the value of a property in an object
	@see object_set_value_by_name()
 **/
#define gl_set_value_by_name DEPRECATED (*callback->properties.set_value_by_name)

/** Get unit of property
	@see object_get_unit()
 **/
#define gl_get_unit DEPRECATED (*callback->properties.get_unit)

/** Convert the units of a property using unit name
	@see unit_convert()
 **/
#define gl_convert DEPRECATED (*callback->unit_convert)

/** Convert the units of a property using unit data
	@see unit_convert_ex()
 **/
#define gl_convert_ex DEPRECATED (*callback->unit_convert_ex)

#define gl_find_unit DEPRECATED (*callback->unit_find)

#define gl_get_object DEPRECATED (*callback->get_object)

#define gl_name_object DEPRECATED (*callback->name_object)

#define gl_get_object_count DEPRECATED (*callback->object_count)

#ifdef __cplusplus
inline DEPRECATED OBJECT **gl_get_object_prop(OBJECT *obj, PROPERTY *prop){
    return (*callback->objvar.object_var)(obj, prop);
}
#else
#define gl_get_object_prop DEPRECATED (*callback->objvar.object_var)
#endif

#ifdef __cplusplus
inline DEPRECATED bool *gl_get_bool(OBJECT *obj, /**< object to set dependency */
							PROPERTY *prop) /**< object dependent on */
{
    return (*callback->objvar.bool_var)(obj,prop);
}
#else
#define gl_get_bool DEPRECATED (*callback->objvar.bool_var)
#endif

#ifdef __cplusplus
inline DEPRECATED bool *gl_get_bool(OBJECT *obj, /**< object to set dependency */
							char *propname) /**< object dependent on */
{
    return (*callback->objvarname.bool_var)(obj,propname);
}
#else
#define gl_get_bool_by_name DEPRECATED (*callback->objvarname.bool_var)
#endif

/** Retrieve the complex value associated with the property
	@see object_get_complex()
**/
#define gl_get_complex DEPRECATED (*callback->objvar.complex_var)

#define gl_get_complex_by_name DEPRECATED (*callback->objvarname.complex_var)

#define gl_get_enum DEPRECATED (*callback->objvar.enum_var)

#define gl_get_enum_by_name DEPRECATED (*callback->objvarname.enum_var)

#define gl_get_set DEPRECATED (*callback->objvar.set_var)

#define gl_get_set_by_name DEPRECATED (*callback->objvarname.set_var)

#define gl_get_int16 DEPRECATED (*callback->objvar.int16_var)

#define gl_get_int16_by_name DEPRECATED (*callback->objvarname.int16_var)

#define gl_get_int32_by_name DEPRECATED (*callback->objvarname.int32_var)

#define gl_get_int32 DEPRECATED (*callback->objvar.int32_var)

#define gl_get_int64_by_name DEPRECATED (*callback->objvarname.int64_var)

#define gl_get_int64 DEPRECATED (*callback->objvar.int64_var)

#define gl_get_double_by_name DEPRECATED (*callback->objvarname.double_var)

#define gl_get_double DEPRECATED (*callback->objvar.double_var)

#define gl_get_string_by_name DEPRECATED (*callback->objvarname.string_var)

#define gl_get_object_prop_by_name DEPRECATED (*callback->objvarname.object_var)

#define gl_get_string DEPRECATED (*callback->objvar.string_var)

#define gl_get_addr DEPRECATED (*callback->properties.get_addr)

/** @} **/

/******************************************************************************
 * Object search
 */
/**	@defgroup gridlabd_h_search Object search

	Searches and navigates object lists.

	@{
 **/
/** Find one or more object
	@see find_objects()
 **/
#define gl_find_objects DEPRECATED (*callback->find.objects)

/** Scan a list of found objects
	@see find_first(), find_next()
 **/
#define gl_find_next DEPRECATED (*callback->find.next)

/** Duplicate a list of found objects
	@see find_copylist()
 **/
#define gl_findlist_copy DEPRECATED (*callback->find.copy)
#define gl_findlist_add DEPRECATED (*callback->find.add)
#define gl_findlist_del DEPRECATED (*callback->find.del)
#define gl_findlist_clear DEPRECATED (*callback->find.clear)
/** Release memory used by a find list
	@see free()
 **/
#define gl_free DEPRECATED (*callback->free)

/** Create an aggregate property from a find list
	@see aggregate_mkgroup()
 **/
#define gl_create_aggregate DEPRECATED (*callback->aggregate.create)

/** Evaluate an aggregate property
	@see aggregate_value()
 **/
#define gl_run_aggregate DEPRECATED (*callback->aggregate.refresh)
/** @} **/

/******************************************************************************
 * Random number generation
 */
/**	@defgroup gridlabd_h_random Random numbers

	The random number library provides a variety of random number generations
	for different distributions.

	@{
 **/

#define RNGSTATE DEPRECATED (&(THISOBJECTHDR)->rng_state)

/** Determine the distribution type to be used from its name
	@see RANDOMTYPE, random_type()
 **/
#define gl_randomtype DEPRECATED (*callback->random.type)

/** Obtain an arbitrary random value using RANDOMTYPE
	@see RANDOMTYPE, random_value()
 **/
#define gl_randomvalue DEPRECATED (*callback->random.value)

/** Obtain an arbitrary random value using RANDOMTYPE
	@see RANDOMTYPE, pseudorandom_value()
 **/
#define gl_pseudorandomvalue DEPRECATED (*callback->random.pseudo)

/** Generate a uniformly distributed random number
	@see random_uniform()
 **/
#define gl_random_uniform DEPRECATED (*callback->random.uniform)

/** Generate a normal distributed random number
	@see random_normal()
 **/
#define gl_random_normal DEPRECATED (*callback->random.normal)

/** Generate a log normal distributed random number
	@see random_lognormal()
 **/
#define gl_random_lognormal DEPRECATED (*callback->random.lognormal)

/** Generate a Bernoulli distributed random number
	@see random_bernoulli()
 **/
#define gl_random_bernoulli DEPRECATED (*callback->random.bernoulli)

/** Generate a Pareto distributed random number
	@see random_pareto()
 **/
#define gl_random_pareto DEPRECATED (*callback->random.pareto)

/** Generate a random number drawn uniformly from a sample
	@see random_sampled()
 **/
#define gl_random_sampled DEPRECATED (*callback->random.sampled)

/** Generate an examponentially distributed random number
	@see random_exponential()
 **/
#define gl_random_exponential DEPRECATED (*callback->random.exponential)
#define gl_random_triangle DEPRECATED (*callback->random.triangle)
#define gl_random_gamma DEPRECATED (*callback->random.gamma)
#define gl_random_beta DEPRECATED (*callback->random.beta)
#define gl_random_weibull DEPRECATED (*callback->random.weibull)
#define gl_random_rayleigh DEPRECATED (*callback->random.rayleigh)
/** @} **/

/******************************************************************************
 * Timestamp handling
 */
/** @defgroup gridlabd_h_timestamp Time handling functions
 @{
 **/

#define gl_globalclock DEPRECATED (*(callback->global_clock))

/** Link to double precision deltamode clock (offset by global_clock) **/
#define gl_globaldeltaclock DEPRECATED (*(callback->global_delta_curr_clock))

/** Link to stop time of the simulation **/
#define gl_globalstoptime DEPRECATED (*(callback->global_stoptime))

/** Convert a string to a timestamp
	@see convert_to_timestamp()
 **/
#define gl_parsetime DEPRECATED (*callback->time.convert_to_timestamp)

#define gl_delta_parsetime DEPRECATED (*callback->time.convert_to_timestamp_delta)

#define gl_printtime DEPRECATED (*callback->time.convert_from_timestamp)

#define gl_printtimedelta DEPRECATED (*callback->time.convert_from_deltatime_timestamp)

/** Convert a timestamp to a date/time structure
	@see mkdatetime()
 **/
#define gl_mktime DEPRECATED (*callback->time.mkdatetime)

/** Convert a date/time structure to a string
	@see strdatetime()
 **/
#define gl_strtime DEPRECATED (*callback->time.strdatetime)

/** Convert a timestamp to days
	@see timestamp_to_days()
 **/
#define gl_todays DEPRECATED (*callback->time.timestamp_to_days)

/** Convert a timestamp to hours
	@see timestamp_to_hours()
 **/
#define gl_tohours DEPRECATED (*callback->time.timestamp_to_hours)

/** Convert a timestamp to minutes
	@see timestamp_to_minutes()
 **/
#define gl_tominutes DEPRECATED (*callback->time.timestamp_to_minutes)

/** Convert a timestamp to seconds
	@see timestamp_to_seconds()
 **/
#define gl_toseconds DEPRECATED (*callback->time.timestamp_to_seconds)

/** Convert a timestamp to a local date/time structure
	@see local_datetime()
 **/
#define gl_localtime DEPRECATED (*callback->time.local_datetime)

/** Convert a timestamp to a local date/time structure
	Use this if it is a double precision time (deltamode)
	@see local_datetime()
 **/
#define gl_localtime_delta DEPRECATED (*callback->time.local_datetime_delta)

#ifdef __cplusplus

inline DEPRECATED int gl_getweekday(TIMESTAMP t)
{
	DATETIME dt;
	gl_localtime(t, &dt);
	return dt.weekday;
}

inline DEPRECATED int gl_gethour(TIMESTAMP t)
{
	DATETIME dt;
	gl_localtime(t, &dt);
	return dt.hour;
}
#endif

/**@}*/
/******************************************************************************
 * Global variables
 */
/** @defgroup gridlabd_h_globals Global variables
 @{
 **/

/** Create a new global variable
	@see global_create()
 **/
#define gl_global_create DEPRECATED (*callback->global.create)

/** Set a global variable
	@see global_setvar()
 **/
#define gl_global_setvar DEPRECATED (*callback->global.setvar)

/** Get a global variable
	@see global_getvar()
 **/
#define gl_global_getvar DEPRECATED (*callback->global.getvar)

/** Find a global variable
	@see global_find()
 **/
#define gl_global_find DEPRECATED (*callback->global.find)

#define gl_get_oflags DEPRECATED (*callback->get_oflags)
/**@}*/

#ifdef __cplusplus

/******************************************************************************
 * Utilities
 */
/** @defgroup gridlabd_h_utility Utility functions
 @{
 **/

/** Clip a value \p x if outside the range (\p a, \p b)
	@return the clipped value of x
	@note \f clip() is only supported in C++
 **/
inline double clip(double x, /**< the value to clip **/
				   double a, /**< the lower limit of the range **/
				   double b) /**< the upper limit of the range **/
{
	if (x<a) return a;
	else if (x>b) return b;
	else return x;
}

/** Determine which bit is set in a bit pattern
	@return the bit number \e n; \p -7f is no bit found; \e -n if more than one bit found
 **/
inline char bitof(unsigned int64 x,/**< bit pattern to scan */
						   bool use_throw=false) /**< flag to use throw when more than one bit is set */
{
	char n=0;
	if (x==0)
	{
		if (use_throw)
			throw "bitof empty bit pattern";
		return -0x7f;
	}
	while ((x&1)==0)
	{
		x>>=1;
		n++;
	}
	if (x!=0)
	{
		if (use_throw)
			throw "bitof found more than one bit";
		else
			return -n;
	}
	return n;
}
/** Construct a proper object object
    @return a pointer to the struct buffer or NULL if failed
 **/
inline DEPRECATED char* gl_name(OBJECT *my, char *buffer, size_t size)
{
	char temp[256];
	if(my == NULL || buffer == NULL) return NULL;
	if (my->name==NULL)
		sprintf(temp,"%s:%d", my->oclass->name, my->id);
	else
		sprintf(temp,"%s", my->name);
	if(size < strlen(temp))
		return NULL;
	strcpy(buffer, temp);
	return buffer;
}

/** Find a schedule 
 **/
inline DEPRECATED SCHEDULE *gl_schedule_find(const char *name)
{
	return callback->schedule.find(name);
}
/** Create a schedule
 **/
inline DEPRECATED SCHEDULE *gl_schedule_create(const char *name, const char *definition)
{
	return callback->schedule.create(name,definition);
}
/** Find the time index in a schedule
 **/
inline DEPRECATED SCHEDULEINDEX gl_schedule_index(SCHEDULE *sch, TIMESTAMP ts)
{
	return callback->schedule.index(sch,ts);
}
/** Find the value at a time index in a schedule
 **/
inline DEPRECATED double gl_schedule_value(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.value(sch,index);
}
/** Find the elapsed time until the value at an index changes
 **/
inline DEPRECATED int32 gl_schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.dtnext(sch,index);
}
inline DEPRECATED SCHEDULE *gl_schedule_getfirst(void)
{
	return callback->schedule.getfirst();
}
/** Create an enduse
 **/
inline DEPRECATED int gl_enduse_create(enduse *e)
{
	return callback->enduse.create(e);
}
/** Synchronize an enduse
 **/
inline DEPRECATED TIMESTAMP gl_enduse_sync(enduse *e, TIMESTAMP t1)
{
	return callback->enduse.sync(e,PC_BOTTOMUP,t1);
}
/** Create a loadshape 
 **/
inline DEPRECATED loadshape *gl_loadshape_create(SCHEDULE *s)
{
	loadshape *ls = (loadshape*)malloc(sizeof(loadshape));
	memset(ls,0,sizeof(loadshape));
	if (0 == callback->loadshape.create(ls)){
		return NULL;
	}
	ls->schedule = s;
	return ls;
}
/** Get the current value of a loadshape
 **/
inline DEPRECATED double gl_get_loadshape_value(loadshape *shape)
{
	if (shape)
		return shape->load;
	else
		return 0;
}
/** Format a DATETIME into a string buffer
 **/
inline DEPRECATED char *gl_strftime(DATETIME *dt, char *buffer, int size) { return callback->time.strdatetime(dt,buffer,size)?buffer:NULL;};
/** Format a TIMESTAMP into a string buffer
 **/
inline DEPRECATED char *gl_strftime(TIMESTAMP ts, char *buffer, int size)
{
	//static char buffer[64];
	DATETIME dt;
	if(buffer == 0){
		gl_error("gl_strftime: buffer is a null pointer");
		return 0;
	}
	if(size < 15){
		gl_error("gl_strftime: buffer size is too small");
		return 0;
	}
	if(gl_localtime(ts,&dt)){
		return gl_strftime(&dt,buffer,size);
	} else {
		strncpy(buffer,"(invalid time)", size);
	}
	return buffer;
}
/** Calculate the next power of 2 greater than a number
 **/
inline size_t nextpow2(size_t x)
{
	if (x<0) return 0;
	x--;
	x|=x>>1;
	x|=x>>2;
	x|=x>>4;
	x|=x>>8;
	x|=x>>16;
	// won't work for anything over 2^30
	return x+1;
}

/**@}*/
#endif //__cplusplus


/******************************************************************************
 * Interpolation routines
 */
/** @defgroup gridlabd_h_interpolation Interpolation routines
 @{
 **/

/** Linearly interpolate a value between two points

 **/
#define gl_lerp DEPRECATED (*callback->interpolate.linear)

/** Quadratically interpolate a value between two points

 **/
#define gl_qerp DEPRECATED (*callback->interpolate.quadratic)
/**@}*/

/******************************************************************************
 * Forecasting routines
 */
/** @defgroup gridlabd_h_forecasting Forecasting routines
 @{
 **/

/** Create a forecast entity for an object
 **/
#define gl_forecast_create DEPRECATED (*callback->forecast.create)

/** Find a forecast entity for an object
 **/
#define gl_forecast_find DEPRECATED (*callback->forecast.find)

/** Read a forecast entity for an object
 **/
#define gl_forecast_read DEPRECATED (*callback->forecast.read)

/** Save a forecast entity for an object
 **/
#define gl_forecast_save DEPRECATED (*callback->forecast.save)
/**@}*/


/******************************************************************************
 * Init/Sync/Create catchall macros
 */
/** @defgroup gridlabd_h_catchall Init/Sync/Create catchall macros
 @{
 **/
///
/// Catchall for sync
///
#define SYNC_CATCHALL(C) catch (const char *msg) { gl_error("sync_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } \
	catch (std::exception &msg) { gl_error("sync_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg.what()); return TS_INVALID; } \
	catch (...) { gl_error("sync_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return TS_INVALID; }
///
/// Catchall for init
///
#define INIT_CATCHALL(C) catch (const char *msg) { gl_error("init_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return 0; } \
	catch (std::exception &msg) { gl_error("init_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg.what()); return TS_INVALID; } \
	catch (...) { gl_error("init_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return 0; }
///
/// Catchall for create
///
#define CREATE_CATCHALL(C) catch (const char *msg) { gl_error("create_" #C ": %s", msg); return 0; } \
	catch (std::exception &msg) { gl_error("create_" #C ": %s", msg.what()); return TS_INVALID; } \
	catch (...) { gl_error("create_" #C ": unhandled exception"); return 0; }
#define I_CATCHALL(T,C) catch (const char *msg) { gl_error(#T "_" #C ": %s", msg); return 0; } \
	catch (std::exception &msg) { gl_error(#T "_" #C ": %s", msg.what()); return TS_INVALID; } \
	catch (...) { gl_error(#T "_" #C ": unhandled exception"); return 0; }
#define T_CATCHALL(T,C) catch (const char *msg) { gl_error(#T "_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } \
	catch (std::exception &msg) { gl_error(#T "_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg.what()); return TS_INVALID; } \
	catch (...) { gl_error(#T "_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return TS_INVALID; }
/**@}*/

/****************************
 * Transform access
 */
/** @defgroup gridlabd_h_transform Transform access
 * @{
 */
#ifdef __cplusplus
inline DEPRECATED TRANSFORM *gl_transform_getfirst(void) { return callback->transform.getnext(NULL); };
inline DEPRECATED TRANSFORM *gl_transform_getnext(TRANSFORM *xform) { return callback->transform.getnext(xform); };
inline DEPRECATED int gl_transform_add_linear(TRANSFORMSOURCE stype,double *source,void *target,double scale,double bias,OBJECT *obj,PROPERTY *prop,SCHEDULE *sched) { return callback->transform.add_linear(stype,source,target,scale,bias,obj,prop,sched); };
inline DEPRECATED int gl_transform_add_external(OBJECT *target_obj, PROPERTY *target_prop,char *function,OBJECT *source_obj, PROPERTY* source_prop) { return callback->transform.add_external(target_obj,target_prop,function,source_obj,source_prop); };
inline DEPRECATED const char *gl_module_find_transform_function(TRANSFORMFUNCTION function) { return callback->module.find_transform_function(function); };
#else
#define gl_transform_getnext DEPRECATED (*callback->transform.getnext) /* TRANSFORM *(*transform.getnext)(TRANSFORM*); */
#define gl_transform_add_linear DEPRECATED (*callback->transfor.add_linear) /* int transform_add_linear(TRANSFORMSOURCE stype,double *source,void *target,double scale,double bias,OBJECT *obj,PROPERTY *prop,SCHEDULE *sched) */
#define gl_transform_add_external DEPRECATED (*callback->transform.add_external) /* int (*transform.add_external)(OBJECT*,PROPERTY*,char*,OBJECT*,PROPERTY*); */
#define gl_module_find_transform_function DEPRECATED (*callback->module.find_transform_function)
#endif
/**@}*/

#ifdef __cplusplus
inline DEPRECATED randomvar *gl_randomvar_getfirst(void) { return callback->randomvar.getnext(NULL); };
inline DEPRECATED randomvar *gl_randomvar_getnext(randomvar *var) { return callback->randomvar.getnext(var); };
inline DEPRECATED size_t gl_randomvar_getspec(char *str, size_t size, const randomvar *var) { return callback->randomvar.getspec(str,size,var); };
#else
#define gl_randomvar_getfirst DEPRECATED (*callback->randomvar.getnext)(NULL)
#define gl_randomvar_getnext DEPRECATED (*callback->randomvar.getnext) /* randomvar *(*randomvar.getnext)(randomvar*) */
#define gl_randomvar_getspec DEPRECATED (*callback->randomvar.getspec) /* size_t (*randomvar.getspec(char*,size_t,randomvar*) */
#endif

/******************************************************************************
 * Remote data access
 */
/** @defgroup gridlabd_h_remote Remote data access
 @{
 **/

#ifdef __cplusplus
/** read remote object data **/
inline DEPRECATED void *gl_read(void *local, /**< local memory for data (must be correct size for property) */
					 OBJECT *obj, /**< object from which to get data */
					 PROPERTY *prop) /**< property from which to get data */
{
	return callback->remote.readobj(local,obj,prop);
}
/** write remote object data **/
inline DEPRECATED void gl_write(void *local, /** local memory for data */
					 OBJECT *obj, /** object to which data is written */
					 PROPERTY *prop) /**< property to which data is written */
{
	/* @todo */
	return callback->remote.writeobj(local,obj,prop);
}
/** read remote global data **/
inline DEPRECATED void *gl_read(void *local, /** local memory for data (must be correct size for global */
					 GLOBALVAR *var) /** global variable from which to get data */
{
	/* @todo */
	return callback->remote.readvar(local,var);
}
/** write remote global data **/
inline DEPRECATED void gl_write(void *local, /** local memory for data */
					 GLOBALVAR *var) /** global variable to which data is written */
{
	/* @todo */
	return callback->remote.writevar(local,var);
}
#endif
/**@}*/

// locking functions 
#ifdef __cplusplus
#define READLOCK(X) DEPRECATED ::rlock(X); /**< Locks an item for reading (allows other reads but blocks write) */
#define WRITELOCK(X) DEPRECATED ::wlock(X); /**< Locks an item for writing (blocks all operations) */
#define READUNLOCK(X) DEPRECATED ::runlock(X); /**< Unlocks an read lock */
#define WRITEUNLOCK(X) DEPRECATED ::wunlock(X); /**< Unlocks a write lock */

inline DEPRECATED void rlock(LOCKVAR* lock) { callback->lock.read(lock); }
inline DEPRECATED void wlock(LOCKVAR* lock) { callback->lock.write(lock); }
inline DEPRECATED void runlock(LOCKVAR* lock) { callback->unlock.read(lock); }
inline DEPRECATED void wunlock(LOCKVAR* lock) { callback->unlock.write(lock); }

#else
#define READLOCK(X) DEPRECATED rlock(X); /**< Locks an item for reading (allows other reads but blocks write) */
#define WRITELOCK(X) DEPRECATED wlock(X); /**< Locks an item for writing (blocks all operations) */
#define READUNLOCK(X) DEPRECATED runlock(X); /**< Unlocks an read lock */
#define WRITEUNLOCK(X) DEPRECATED wunlock(X); /**< Unlocks a write lock */
#endif
#define LOCK(X) DEPRECATED WRITELOCK(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK(X) DEPRECATED WRITEUNLOCK(X); /**< @todo this is deprecated and should not be used anymore */

#define READLOCK_OBJECT(X) DEPRECATED READLOCK(&((X)->lock)) /**< Locks an object for reading */
#define WRITELOCK_OBJECT(X) DEPRECATED WRITELOCK(&((X)->lock)) /**< Locks an object for writing */
#define READUNLOCK_OBJECT(X) DEPRECATED READUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define WRITEUNLOCK_OBJECT(X) DEPRECATED WRITEUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define LOCK_OBJECT(X) DEPRECATED WRITELOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK_OBJECT(X) DEPRECATED WRITEUNLOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */

#define LOCKED(X,C) DEPRECATED {WRITELOCK_OBJECT(X);(C);WRITEUNLOCK_OBJECT(X);} /**< @todo this is deprecated and should not be used anymore */

#include <math.h>
#define NaN NAN

#ifdef __cplusplus

/*	Section: Module message flags

	The variable <module_message_flags> is a <set> that control the flow of messages to the output
	stream.

	The following message module flags are defined
	- <MMF_NONE>
	- <MMF_QUIET>
	- <MMF_DEBUG>
	- <MMF_VERBOSE>
	- <MMF_WARNING>
	- <MMF_ALL>
 */
extern set module_message_flags;

/*	Define: MMF_NONE

	Specify that no messages from the module are to be output.
 */
#define MMF_NONE 0

/*	Define: MMF_QUIET

	Specify that error messages from the module are to be suppressed. See <OF_QUIET>.
 */
#define MMF_QUIET OF_QUIET

/*	Define: MMF_DEBUG

	Specify that debug messages are to be output. See <OF_DEBUG>.
 */
#define MMF_DEBUG OF_DEBUG

/*	Define: MMF_VERBOSE

	Specify that verbose messages are to be output See <OF_VERBOSE>.
 */
#define MMF_VERBOSE OF_VERBOSE

/*	Define: MMF_WARNING

	Specify that warning messages are to be suppressed See <OF_WARNING>.
 */
#define MMF_WARNING OF_WARNING

/*	Define: MMF_ALL

	Specify that all messages from the module are to be output.	See <MMF_DEBUG> and <MMF_VERBOSE>
 */
#define MMF_ALL (OF_DEBUG|OF_VERBOSE)

/**************************************************************************************
 * GRIDLABD BASE CLASSES (Version 3.0 and later)
 * @defgroup gridlabd_h_classes Module API Classes
 * @{
 **************************************************************************************/

#include <ctype.h>
#include "module.h"
#include "class.h"
#include "property.h"

/*	Class: gld_string

	General string encapsulation
 */
class gld_string 
{
private: // data
	typedef struct strbuf {
		LOCKVAR lock; // TODO implement locking
		size_t len;
		unsigned int nrefs;
		char *str;
	} STRBUF;
	STRBUF *buf;
public: 

	// Constructor: gld_string(void)
	inline gld_string(void) : buf(NULL) { init(); };

	// Constructor: gld_string(gld_string &s)
	inline gld_string(gld_string&s) : buf(NULL) { init(); link(s); };

	// Constructor: gld_string(const char *s)
	inline gld_string(const char *s) : buf(NULL) { init(); copy(s); };

	// Constructor: gld_stdring(const char *, size_t n)
	inline gld_string(const char *s, size_t n) : buf(NULL) { init(); copy(s,n); };

	// Destructor: ~gld_string
	inline ~gld_string(void) { unlink(); };
public: 

	// Operator: = (const char *s)
	inline gld_string &operator=(const char *s) { copy(s); return *this; };

	// Operator: = (gld_string &s)
	inline gld_string &operator=(gld_string&s) { link(s); return *this; };
public: 

	// Operator: const char*
	inline operator const char*(void) { return buf->str; };

	// Operator: size_t
	inline operator size_t(void) { return buf->len; };

	// Operator: STRBUF
	inline operator STRBUF *(void) { return buf; };

private: // internals
	inline void init(void) { buf=(STRBUF*)malloc(sizeof(STRBUF)); memset(buf,0,sizeof(STRBUF)); }; 
	inline void lock(void) { if ( buf ) ::wlock(&buf->lock); };
	inline void unlock(void) { if ( buf ) ::wunlock(&buf->lock); };
	inline void fit(size_t n) { if ( buf==NULL || n>buf->len) alloc(n); };
	inline void alloc(size_t n) 
	{
		size_t len = nextpow2(n);
		if ( len<sizeof(NATIVE) ) 
			len=sizeof(NATIVE); 
		char *newstr=(char*)malloc(len);
		if ( buf->str!=NULL )
		{
			strcpy(newstr,buf->str);
			free(buf->str);
		}
		else
			buf->nrefs=1;
		buf->str = newstr;
		buf->len = len;
	};
	inline void copy(const char *s) { fit(strlen(s)+1); strcpy(buf->str,s); };
	inline void copy(const char *s, size_t n) { fit(n+1); strncpy(buf->str,s,n); };
	inline void link(gld_string&s) { unlink(); buf=(STRBUF*)s; buf->nrefs++;};
	inline void unlink() { if ( buf->nrefs<=1 ) {free(buf->str); free(buf);} else buf->nrefs--; };
public: 

	//	Method: is_valid
	inline bool is_valid(void) { return buf!=NULL; };

	//	Method: is_null
	inline bool is_null(void) { return is_valid() && buf->str==NULL; };

public: 

	// Method: get_buffer
	inline const char* get_buffer(void) { return buf ? buf->str : NULL; };

	// Method: get_size
	inline size_t get_size(void) { return buf ? buf->len : -1; };

	// Method: get_length
	inline size_t get_length(void) { return buf && buf->str ? strlen(buf->str) : -1; };

public: 

	// Method: set_string
	inline void set_string(const char *s) { copy(s); };

	// Method: set_size
	inline void set_size(size_t n) { fit(n); };

	// Method: format(const char *fmt,...)
	inline size_t format(const char *fmt,...) { va_list ptr; va_start(ptr,fmt); int len=vsnprintf(buf->str,buf->len,fmt,ptr); va_end(ptr); return len;};

	// Method: format(size_t len,const char *fmt,...)
	inline size_t format(size_t len,const char *fmt,...) { fit(len); va_list ptr; va_start(ptr,fmt); int rv=vsnprintf(buf->str,buf->len,fmt,ptr); va_end(ptr); return rv;};

public: 

	// Operator: <
	inline bool operator<(const char*s) { return strcmp(buf->str,s)<0; };

	// Operator: <=
	inline bool operator<=(const char*s) { return strcmp(buf->str,s)<=0; };

	// Operator: ==
	inline bool operator==(const char*s) { return strcmp(buf->str,s)==0; };

	// Operator: >=
	inline bool operator>=(const char*s) { return strcmp(buf->str,s)>=0; };

	// Operator: >
	inline bool operator>(const char*s) { return strcmp(buf->str,s)>0; };

	// Operator: !=
	inline bool operator!=(const char*s) { return strcmp(buf->str,s)!=0; };
public: // manipulation

	// Method: trimleft
	inline void trimleft(void) { if ( is_null() ) return; size_t n=0; while (buf->str[n]!='\0'&&isspace(*buf->str)) n++; strcpy(buf->str,buf->str+n); };

	// Method: trimright
	inline void trimright(void) { if ( is_null() ) return; size_t n=strlen(buf->str); while (n>0&&isspace(buf->str[n-1])) buf->str[--n]='\0'; };

	// Method: left
	inline gld_string left(size_t n) { if ( is_null() ) return gld_string(); return gld_string(buf->str,n); };

	// Method: right
	inline gld_string right(size_t n) { if ( is_null() ) return gld_string(); return gld_string(buf->str+buf->len-n); };

	// Method: mid
	inline gld_string mid(size_t n, size_t m) { if ( is_null() ) return gld_string(); return gld_string(buf->str+buf->len-n,m); };

	// Method: findstr
	inline size_t findstr(const char *s) { if ( is_null() ) return -1; char *p=strstr(buf->str,s); return p==NULL ? -1 : (p-buf->str); };

	// Method: findchr
	inline size_t findchr(char c) { if ( is_null() ) return -1; char *p=strchr(buf->str,c); return p==NULL ? -1 : (p-buf->str); };

	// Method: split
	// This function is not implemented yet
	inline size_t split(gld_string *&list, const char *delim=" ") 
	{
		// TODO
		if ( is_null() ) return 0;
		return 0;
	}

	// Method: merge
	// This function is not implemented yet
	inline gld_string merge(gld_string *&list, size_t n, const char *delim=" ")
	{
		// TODO
		return gld_string();
	}
};

/*	Class: gld_clock

 	Date/time encapsulation
 */
class gld_clock 
{
private: // data
	DATETIME dt;
public: 

	// Constructor: gld_clock
	gld_clock(void) { callback->time.local_datetime(*(callback->global_clock),&dt); }; 

	// Constructor: gld_clock
	gld_clock(TIMESTAMP ts) { if ( !callback->time.local_datetime(ts,&dt)) memset(&dt,0,sizeof(dt)); };

	// Constructor: gld_clock
	gld_clock(const char *str) { from_string(str); };

	// Constructor: gld_clock
	gld_clock(unsigned short y, unsigned short m=0, unsigned short d=0, unsigned short H=0, unsigned short M=0, unsigned short S=0, unsigned short int ms=0, char *tz=NULL, int dst=-1)
	{
		dt.year = y; dt.month=m; dt.day=d; dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=ms;
		if ( dst>=0 ) dt.is_dst=dst;
		if ( tz!=NULL ) set_tz(tz); else callback->time.mkdatetime(&dt);
	}
public: 

	// Operator: TIMESTAMP
	inline operator TIMESTAMP (void) { return dt.timestamp; };
public: 

	// Operator: operator>
	inline bool operator > (TIMESTAMP t) { return dt.timestamp>t; };

	// Operator: operator>=
	inline bool operator >= (TIMESTAMP t) { return dt.timestamp>=t; };

	// Operator: operator<
	inline bool operator < (TIMESTAMP t) { return dt.timestamp<t; };

	// Operator: operator<=
	inline bool operator <= (TIMESTAMP t) { return dt.timestamp<=t; };

	// Operator: operator==
	inline bool operator == (TIMESTAMP t) { return dt.timestamp==t; };

	// Operator: operator!=
	inline bool operator != (TIMESTAMP t) { return dt.timestamp!=t; };

	// Method: is_valid
	inline bool is_valid(void) { return dt.timestamp>0; };

	// Method: is_never
	inline bool is_never(void) { return dt.timestamp==TS_NEVER; };

public: 

	// Method: get_year
	// Get the year
	inline unsigned short get_year(void) { return dt.year; };

	// Method: get_month
	// Get the month (0-11)
	inline unsigned short get_month(void) { return dt.month; };

	// Method: get_day
	// Get the day (1-31)
	inline unsigned short get_day(void) { return dt.day; };

	// Method: get_hour
	// Get the hour (0-23)
	inline unsigned short get_hour(void) { return dt.hour; };

	// Method: get_minute
	// Get the minute (0-59)
	inline unsigned short get_minute(void) { return dt.minute; };

	// Method: get_second
	// Get the second (0-59)
	inline unsigned short get_second(void) { return dt.second; };

	// Method: get_nanosecond
	// Get the nanosecond (0-999999)
	inline unsigned int get_nanosecond(void) { return dt.nanosecond; };

	// Method: get_uday
	// Get the Unix Day Number (full days since the Unix Epoch)
	inline unsigned int get_uday(void) { return dt.timestamp / 86400; };

	// Method: get_jday
	// Get the Julian Day Number
	inline unsigned int get_jday(void) { return (dt.timestamp / 86400) + 2440587.5; };

	// Method: get_tz
	// Get the timezone spec
	inline char* get_tz(void) { return dt.tz; };

	// Method: get_is_dst
	// Get the summer/daylight time flag
	inline bool get_is_dst(void) { return dt.is_dst?true:false; };

	// Method: get_weekday
	// Get the weekday (Sunday=0)
	inline unsigned short get_weekday(void) { return dt.weekday; };

	// Method: get_yearday
	// Get the day of the year (Jan 1=0)
	inline unsigned short get_yearday(void) { return dt.yearday; };

	// Method: get_tzoffset
	// Get the timezone offer (in seconds)
	inline int get_tzoffset(void) { return dt.tzoffset; };

	// Method: get_timestamp
	// Get the TIMESTAMP value (UTC)
	inline TIMESTAMP get_timestamp(void) { return dt.timestamp; };

	// Method: get_localtimestamp
	// Get the local TIMESTAMP value (ignoring DST)
	inline TIMESTAMP get_localtimestamp(void) { return dt.timestamp - dt.tzoffset; };

	// Method: get_localtimestamp_dst
	// Get the local TIMESTAMP value (including DST)
	inline TIMESTAMP get_localtimestamp_dst(bool force_dst=false) { return dt.timestamp - dt.tzoffset + (dt.is_dst||force_dst?3600:0); };
public: // write accessors

	// Method: set_date
	// Set the date
	inline TIMESTAMP set_date(unsigned short y, unsigned short m, unsigned short d) { dt.year=y; dt.month=m; dt.day=d; return callback->time.mkdatetime(&dt); };

	// Method: set_time
	// Set the time
	inline TIMESTAMP set_time(unsigned short H, unsigned short M, unsigned short S, unsigned long u=0, char *t=NULL, bool force_dst=false) { dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=u; strncpy(dt.tz,t,sizeof(dt.tz)); if (force_dst) dt.is_dst=true; return callback->time.mkdatetime(&dt); };

	// Method: set_datetime
	// Set the date and time
	inline TIMESTAMP set_datetime(unsigned short y, unsigned short m, unsigned short d, unsigned short H, unsigned short M, unsigned short S, unsigned long u=0, char *t=NULL, bool force_dst=false) { dt.year=y; dt.month=m; dt.day=d; dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=u; strncpy(dt.tz,t,sizeof(dt.tz)); if (force_dst) dt.is_dst=true; return callback->time.mkdatetime(&dt); };

	// Method: set_year
	// Set the year
	inline TIMESTAMP set_year(unsigned short y) { dt.year=y; return callback->time.mkdatetime(&dt); };

	// Method: set_month
	// Set the month (Jan=0)
	inline TIMESTAMP set_month(unsigned short m) { dt.month=m; return callback->time.mkdatetime(&dt); };

	// Method: set_day
	// Set the day (1-31)
	inline TIMESTAMP set_day(unsigned short d) { dt.day=d; return callback->time.mkdatetime(&dt); };

	// Method: set_hour
	// Set the hour (0-23)
	inline TIMESTAMP set_hour(unsigned short h) { dt.hour=h; return callback->time.mkdatetime(&dt); };

	// Method: set_minute
	// Set the minute (0-59)
	inline TIMESTAMP set_minute(unsigned short m) { dt.minute=m; return callback->time.mkdatetime(&dt); };

	// Method: set_second
	// Set the second (0-59)
	inline TIMESTAMP set_second(unsigned short s) { dt.second=s; return callback->time.mkdatetime(&dt); };

	// Method: set_nanosecond
	// Set the nanosecond (0-999999)
	inline TIMESTAMP set_nanosecond(unsigned int u) { dt.nanosecond=u; return callback->time.mkdatetime(&dt); };

	// Method: set_tz
	// Set the timezone (see tzinfo.txt)
	inline TIMESTAMP set_tz(char* t) { strncpy(dt.tz,t,sizeof(dt.tz)); return callback->time.mkdatetime(&dt); };

	// Method: set_is_dst
	// Set the DST flag
	inline TIMESTAMP set_is_dst(bool i) { dt.is_dst=i; return callback->time.mkdatetime(&dt); };
public: // special functions

	// Method: from_string
	// Convert from string
	inline bool from_string(const char *str) { return callback->time.local_datetime(callback->time.convert_to_timestamp(str),&dt)?true:false; };

	// Method: to_string
	// Convert to string
	inline unsigned int to_string(char *str, int size) {return callback->time.convert_from_timestamp(dt.timestamp,str,size); };

	// Method: to_days
	// Extract the total number of days since 1/1/1970 0:00:00 UTC
	inline double to_days(TIMESTAMP ts=0) { return (dt.timestamp-ts)/86400.0 + dt.nanosecond*1e-9; };

	// Method: to_hours
	// Extract the total number of hours since 1/1/1970 0:00:00 UTC
	inline double to_hours(TIMESTAMP ts=0) { return (dt.timestamp-ts)/3600.0 + dt.nanosecond*1e-9; };

	// Method: to_minutes
	// Extract the total number of minutes since 1/1/1970 0:00:00 UTC
	inline double to_minutes(TIMESTAMP ts=0) { return (dt.timestamp-ts)/60.0 + dt.nanosecond*1e-9; };

	// Method: to_seconds
	// Extract the total number of seconds since 1/1/1970 0:00:00 UTC
	inline double to_seconds(TIMESTAMP ts=0) { return dt.timestamp-ts + dt.nanosecond*1e-9; };

	// Method: to_nanoseconds
	// Extract the total number of nanoseconds since 1/1/1970 0:00:00 UTC
	inline double to_nanoseconds(TIMESTAMP ts=0) { return (dt.timestamp-ts)*1e9 + dt.nanosecond; };

	// Method: get_string
	// Get the timestamp as a string
	inline gld_string get_string(const size_t sz=1024) 
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};
};

/*	Class: gld_rlock

	Read lock container
 */
class gld_rlock 
{
private: OBJECT *my;
	
	// Constructor: gld_rlock
public: inline gld_rlock(OBJECT *obj) : my(obj) {::rlock(&my->lock);}; 

	// Destructor: ~gld_rlock
public: inline ~gld_rlock(void) {::runlock(&my->lock);};
};

/*	Class: gld_wlock

	Write lock container
 */
class gld_wlock 
{
private: OBJECT *my;

		 // Constructor: gld_wlock
public: inline gld_wlock(OBJECT *obj) : my(obj) {::wlock(&my->lock);}; 

		// Destructor: ~gld_wlock
public: inline ~gld_wlock(void) {::wunlock(&my->lock);};
};

class gld_class;

/* 	Class: gld_module

	Module container
 */
class gld_module 
{
private: 
	MODULE core;

public: 

	// Constructor: gld_module
	inline gld_module(void) { MODULE *m = callback->module.getfirst(); if (m) core=*m; else throw "no modules loaded";};
	
	// Operator: MODULE*
	inline operator MODULE*(void) { return &core; };

public: 

	// Method: get_name
	// Get module name
	inline char* get_name(void) { return core.name; };
	
	// Method: get_major
	// Get module major version number
	inline unsigned short get_major(void) { return core.major; };
	
	// Method: get_minor
	// Get module minor version number
	inline unsigned short get_minor(void) { return core.minor; };
	
	// Method: get_first_class
	// Get first class in module
	inline gld_class* get_first_class(void) { return (gld_class*)core.oclass; };

public:

	// Method: is_last
	// Check if last module loaded
	inline bool is_last(void) { return core.next==NULL; };
	
	// Method: get_next
	// Get next module loaded
	inline void get_next(void) { core = *(core.next); };
};

class gld_property;
class gld_function;

/*	Class: gld_class

	Class container
 */
class gld_class 
{
private: 
	CLASS core;

public: 

	// Constructor: gld_class
	// This is a blocker implementation to prevent direct creation of a class for now.
	inline gld_class(void) { throw "gld_class constructor not permitted"; };

	// Operatior: CLASS*
	// Cast to CLASS
	inline operator CLASS*(void) { return &core; };

public: 

	// Method: get_name
	// Get class name
	inline const char* get_name(void) { return core.name; };
	
	// Method: get_size
	// Get class size
	inline size_t get_size(void) { return core.size; };
	
	// Method: get_parent
	// Get class parent
	inline gld_class* get_parent(void) { return (gld_class*)core.parent; };
	
	// Method: get_module
	// Get module that implements the class
	inline gld_module* get_module(void) { return (gld_module*)core.module; };
	
	// Method: get_first_property
	// Get the first property in the class
	inline gld_property* get_first_property(void) { return (gld_property*)core.pmap; };
	
	// Method: get_next_property
	// Get the next property in the class
	inline gld_property* get_next_property(PROPERTY*p) { PROPERTY *prop=(PROPERTY*)p->next; return ( prop && prop->oclass==&core ) ? (gld_property*)prop : NULL; };
	
	// Method: get_first_function
	// Get the first function in the class
	inline gld_function* get_first_function(void) { return (gld_function*)core.fmap; };
	
	// Method: get_next_function
	// Get the next function in the class
	inline gld_function* get_next_function(FUNCTION*f) { return (gld_function*)f->next; };
	
	// Method: get_trl
	// Get TRL 
	inline TECHNOLOGYREADINESSLEVEL get_trl(void) { return core.trl; };

public: 
	
	// Method: set_trl
	// Set TRL
	inline void set_trl(TECHNOLOGYREADINESSLEVEL t) { core.trl=t; };

public: 
	
	// Method: create
	// Register a class	
	static inline CLASS *create(MODULE *m, const char *n, size_t s, unsigned int f) { return callback->register_class(m,n,(unsigned int)s,PASSCONFIG(f)); };
	
public: 
	
	// Method: is_last
	// Check if last class registered
	inline bool is_last(void) { return core.next==NULL; };
	
	// Method: is_module_last
	// Check if last class defined by this class' module
	inline bool is_module_last(void) { return core.next==NULL || core.module!=core.next->module; };
	
	// Method: get_next
	// Get the next class
	inline gld_class* get_next(void) { return (gld_class*)core.next; };
};

/*	Class: gld_function

	Function container
 */
class gld_function 
{
private: 
	FUNCTION core;

public: 

	// Contructor: gld_function
	// Construct a function (blocker implementation)
	inline gld_function(void) { throw "gld_function constructor not permitted"; };
	
	// Operator: FUNCTION*
	// Cast to a FUNCTION pointer
	inline operator FUNCTION*(void) { return &core; };

public: 

	// Method: get_name
	// Get function name
	inline FUNCTIONNAME get_name(void) { return core.name; };
	
	// Method: get_class
	// Get function class
	inline gld_class* get_class(void) { return (gld_class*)core.oclass; };
	
	// Method: get_class
	// Get function address
	inline FUNCTIONADDR get_addr(void) { return core.addr; };

public: 

public: 

	// Method: is_last
	// Check whether this is the last function defined
	inline bool is_last(void) { return core.next==NULL; };
	
	// Method: get_next
	// Get the next function in the list
	inline gld_function* get_next(void) { return (gld_function*)core.next; };
};

/*	Class: gld_type

	Built-in type container
 */
class gld_type 
{
private: 
	PROPERTYTYPE type;

public: 

	// Constructor: gld_type
	inline gld_type(PROPERTYTYPE t) : type(t) {};
	
	// Operator: PROPERTYTYPE
	// Cast to PROPERTYTYPE
	inline operator PROPERTYTYPE(void) { return type; };

public:

	// Method: get_spec
	PROPERTYSPEC *get_spec(void) { return callback->properties.get_spec(type);};

	// TODO size,conversions,etc...

public: 

public: 

	// Method: get_first
	// Get the first property type
	static inline PROPERTYTYPE get_first(void) { return PT_double; };
	
	// Method: get_next
	// Get the next property type
	inline PROPERTYTYPE get_next(void) { return (PROPERTYTYPE)(((int)type)+1); };
	
	// Method: is_last
	// Check whether this is the last property type
	inline bool is_last(void) { return (PROPERTYTYPE)(((int)type)+1)==_PT_LAST; }; 
};

/*	Class: gld_unit

	Unit container
 */
class gld_unit
{
private: 
	UNIT core;

public: 

	// Constructor: gld_unit
	// Construct empty unit container
	inline gld_unit(void) { memset(&core,0,sizeof(core)); };
	
	// Constructor: gld_unit
	// Construct a container for a named or derived unit
	inline gld_unit(const char *name) { UNIT *unit=callback->unit_find(name); if (unit) memcpy(&core,unit,sizeof(UNIT)); else memset(&core,0,sizeof(UNIT)); };
	
	// Operator: UNIT*
	// Cast to a UNIT structure
	inline operator UNIT*(void) { return &core; };

public: 

	// Method: get_name
	// Get the name or derivation of the unit
	inline char* get_name(void) { return core.name; };
	
	// Method: get_c
	// Get the C exponent
	inline double get_c(void) { return core.c; };
	
	// Method: get_e
	// Get the E exponent
	inline double get_e(void) { return core.e; };
	
	// Method: get_h
	// Get the H exponent
	inline double get_h(void) { return core.h; };
	
	// Method: get_k
	// Get the K exponent
	inline double get_k(void) { return core.k; };
	
	// Method: get_m
	// Get the M exponent
	inline double get_m(void) { return core.m; };
	
	// Method: get_s
	// Get the S exponent
	inline double get_s(void) { return core.s; };
	
	// Method: get_a
	// Get the A constant
	inline double get_a(void) { return core.a; };
	
	// Method: get_b
	// Get the B constant
	inline double get_b(void) { return core.b; };
	
	// Method: get_prec
	// Get the unit precision
	inline int get_prec(void) { return core.prec; };
	
	// Method: is_valid
	// Check whether the unit is valid
	inline bool is_valid(void) { return (UNIT*)&core != NULL && core.name[0]!='\0'; };

public: 

	// Method: set_unit
	// Change the unit
	inline bool set_unit(const char *name){ UNIT *unit=callback->unit_find(name); if (unit) {memcpy(&core,unit,sizeof(UNIT));return true;} else {memset(&core,0,sizeof(UNIT));return false;} };

public: 

	// Method: convert
	// Convert a value to another named or derived unit
	inline bool convert(const char *name, double &value) { UNIT *unit=callback->unit_find(name); return unit&&(callback->unit_convert_ex(&core,unit,&value))?true:false; }
	
	// Method: convert
	// Convert a value to another UNIT
	inline bool convert(UNIT *unit, double &value) { return callback->unit_convert_ex(&core,unit,&value)?true:false; }
	
	// Method: convert
	// Convert a value to another gld_unit
	inline bool convert(gld_unit &unit, double &value) { return callback->unit_convert_ex(&core,(UNIT*)unit,&value)?true:false; }

public: 

	// Method: is_last
	// Check whether this is the last defined unit
	inline bool is_last(void) { return core.next==NULL?true:false; };
	
	// Method: get_next
	// Get the next unit
	inline gld_unit* get_next(void) { return (gld_unit*)core.next; };
};

/*	Class: gld_keyword

	Keyword container
 */
class gld_keyword
{
private: 
	KEYWORD core;

public: 

	// Constructor: gld_keyword 
	// Construct a key word
	inline gld_keyword(KEYWORD &key) { core=key; };
	
	// Operator: KEYWORD* 
	inline operator KEYWORD* (void) { return &core; };

	// Operator: char* 
	inline operator const char* (void) { return core.name; };

	// Operator: long unsigned int 
	inline operator long unsigned int (void) { return core.value;};

public: 
	
	// Method: get_name
	// Get the name of a keyword
	inline const char* get_name(void) { return core.name; };
	
	// Method: get_value
	// Get the bit pattern for the keyword
	inline long unsigned int get_value(void) { return core.value; };

	// Method: get_enumeration_value
	inline enumeration get_enumeration_value(void) { return (enumeration)get_value(); };

	// Method: get_set_value
	inline set get_set_value(void) { return (set)get_value(); };

public: 

	// Method: compare
	inline int compare(const char *name) { return strcmp(name,core.name); };

	// Operator: operator==
	inline bool operator== (const char *name) { return compare(name)==0; };

	// Operator: operator<=
	inline bool operator<= (const char *name) { return compare(name)<=0; };

	// Operator: operator>=
	inline bool operator>= (const char *name) { return compare(name)>=0; };

	// Operator: operator<
	inline bool operator< (const char *name) { return compare(name)<0; };

	// Operator: operator>
	inline bool operator> (const char *name) { return compare(name)>0; };

	// Operator: operator!=
	inline bool operator!= (const char *name) { return compare(name)!=0; };

	// Method: compare
	inline int compare(long unsigned int value) { return value<(long unsigned int)core.value ? -1 : ( value>core.value ? +1 : 0 ); };

	// Operator: operator==
	inline bool operator== (long unsigned int value) { return compare(value)==0; };

	// Operator: operator<=
	inline bool operator<= (long unsigned int value) { return compare(value)<=0; };

	// Operator: operator>=
	inline bool operator>= (long unsigned int value) { return compare(value)>=0; };

	// Operator: operator<
	inline bool operator< (long unsigned int value) { return compare(value)<0; };

	// Operator: operator>
	inline bool operator> (long unsigned int value) { return compare(value)>0; };

	// Operator: operator!=
	inline bool operator!= (long unsigned int value) { return compare(value)!=0; };

public: 

	// Method: get_next
	// Get the next keyword (NULL if last)
	inline gld_keyword* get_next(void) { return (gld_keyword*)core.next; };

	// Method: find
	template <class T> inline gld_keyword* find(T value)
	{
		if ( compare(value)==0 ) return this;
		if ( get_next()==NULL ) return NULL;
		return get_next()->find(value);
	};
};

// Section: Property definitions

// Define: GL_ATOMIC(<type>,<name>)
// Define an atomic property (e.g., int, double, etc)
//
// Methods:
// size_t get_<name>_offset(void) - return the address of the value
// <type> get_<name>() - get the value
// gld_property get_<name>_property() - get the property of the value
// <type> get_<name>(gld_rlock&) - get the value with a read lock
// <type> get_<name>(gld_wlock&) - get the value with a write lock
// void set_<name>(<type> p) - set a value
// void set_<name>(<type> p, gld_wlock&) - set a value with a write lock 
// gld_string get_<name>_string(void) - get the value as a string
// void set_<name>(const char *str) - set the value from a string
// void init_<name>(void) - initialize the value to the default
// void init_<name>(<type> value) - initialize the value
#define GL_ATOMIC(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(void) { return X; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { X=p; }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; \
	inline gld_string get_##X##_string(void) { return get_##X##_property().get_string(); }; \
	inline void set_##X(const char *str) { get_##X##_property().from_string(str); }; \

// Define: GL_STRUCT
// Define a structured property
//
// Methods:
// size_t get_<name>_offset(void) - return the address of the value
// <type> get_<name>() - get the value
// gld_property get_<name>_property() - get the property of the value
// <type> get_<name>(gld_rlock&) - get the value with a read lock
// <type> get_<name>(gld_wlock&) - get the value with a write lock
// void set_<name>(<type> p) - set a value
// void set_<name>(<type> p, gld_wlock&) - set a value with a write lock 
// gld_string get_<name>_string(void) - get the value as a string
// void set_<name>(const char *str) - set the value from a string
// void init_<name>(void) - initialize the value to the default
// void init_<name>(<type> value) - initialize the value
#define GL_STRUCT(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(void) { gld_rlock _lock(my()); return X; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { gld_wlock _lock(my()); X=p; }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; \
	inline gld_string get_##X##_string(void) { return get_##X##_property().get_string(); }; \
	inline void set_##X(char *str) { get_##X##_property().from_string(str); }; \

// Define: GL_STRING
// Define a string property
//
// Methods:
// size_t get_<name>_offset(void) - return the address of the value
// <type> get_<name>() - get the value
// gld_property get_<name>_property() - get the property of the value
// char* get_<name>(gld_rlock&) - get the value with a read lock
// char* get_<name>(gld_wlock&) - get the value with a write lock
// char get_<name>(size_t n) - get a character
// char get_<name>(size_t n, gld_rlock&) - get a character with a read lock
// char get_<name>(size_t n, gld_wlock&) - get a character with a write lock
// void set_<name>(<type> p) - set a value
// void set_<name>(<type> p, gld_wlock&) - set a value with a write lock 
// void set_<name>(size_t n, char c) - set a character
// void set_<name>(size_t n, char c, gld_wlock&) - set a character with a write lock
// gld_string get_<name>_string(void) - get the value as a string
// void set_<name>(const char *str) - set the value from a string
// void init_<name>(void) - initialize the value to the default
// void init_<name>(<type> value) - initialize the value
#define GL_STRING(T,X) 	protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline char* get_##X(void) { gld_rlock _lock(my()); return X.get_string(); }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline char* get_##X(gld_rlock&) { return X.get_string(); }; \
	inline char* get_##X(gld_wlock&) { return X.get_string(); }; \
	inline char get_##X(size_t n) { gld_rlock _lock(my()); return X[n]; }; \
	inline char get_##X(size_t n, gld_rlock&) { return X[n]; }; \
	inline char get_##X(size_t n, gld_wlock&) { return X[n]; }; \
	inline void set_##X(char *p) { gld_wlock _lock(my()); strncpy(X,p,sizeof(X)); }; \
	inline void set_##X(char *p, gld_wlock&) { strncpy(X,p,sizeof(X)); }; \
	inline void set_##X(size_t n, char c) { gld_wlock _lock(my()); X[n]=c; }; \
	inline void set_##X(size_t n, char c, gld_wlock&) { X[n]=c; };  \

// Define: GL_ARRAY
// Define an array property
#define GL_ARRAY(T,X,S) protected: T X[S]; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T* get_##X(void) { gld_rlock _lock(my()); return X; }; \
	inline T* get_##X(gld_rlock&) { return X; }; \
	inline T* get_##X(gld_wlock&) { return X; }; \
	inline T get_##X(size_t n) { gld_rlock _lock(my()); return X[n]; }; \
	inline T get_##X(size_t n, gld_rlock&) { return X[n]; }; \
	inline T get_##X(size_t n, gld_wlock&) { return X[n]; }; \
	inline void set_##X(T* p) { gld_wlock _lock(my()); memcpy(X,p,sizeof(X)); }; \
	inline void set_##X(T* p, gld_wlock&) { memcpy(X,p,sizeof(X)); }; \
	inline void set_##X(size_t n, T m) { gld_wlock _lock(my()); X[n]=m; }; \
	inline void set_##X(size_t n, T m, gld_wlock&) { X[n]=m; };  \

// Define: GL_BITFLAGS
// Define a bitflag property
#define GL_BITFLAGS(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(T mask=-1) { return X&mask; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { X=p; }; \
	inline void set_##X##_bits(T p) { gld_rlock _lock(my()); (X)|=(p); }; \
	inline void clr_##X##_bits(T p) { gld_rlock _lock(my()); (X)&=~(p); }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; \
	inline gld_string get_##X##_string(void) { return get_##X##_property().get_string(); }; \
	inline void set_##X(char *str) { get_##X##_property().from_string(str); }; \

// Define: GL_METHOD(<class>,<name>)
// Define a method property
//
// Methods:
// size_t get_<name>_offset(void) - get the address of the method interface
// int get_<name>(char *buffer, size_t len) - get the value of a property using a method interface
// int set_<name>(char *buffer) - set the value of a property using a method interface
#define GL_METHOD(C,X) public: int X(char *buffer, size_t len=0); \
	static inline size_t get_##X##_offset(void) { return (size_t)method_##C##_##X; }; \
	inline int get_##X(char *buffer, size_t len) { return X(buffer,len); }; \
	inline int set_##X(char *buffer) { return X(buffer,0); }

#define GL_OBJECT(T,X) protected: OBJECT* X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline gld_object *get_##X(void) { return get_object(X); }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline gld_object *get_##X(gld_rlock&) { return get_object(X); }; \
	inline gld_object *get_##X(gld_wlock&) { return get_object(X); }; \
	inline void set_##X(OBJECT* p) { X=p; }; \
	inline void set_##X(OBJECT* p, gld_wlock&) { X=p; }; \
	inline void set_##X(const char *str) { get_##X##_property().from_string(str); }; \
	inline void init_##X(void) { memset((void*)&X,0,sizeof(X));}; \
	inline void init_##X(OBJECT* value) { X=value;}; \
	inline T *get_##X##_object(void) { return (T*)(X+1);};

// Define: IMPL_METHOD(class,name)
// Parameters:
// buffer - a char* reference to the buffer containing the value
// len - a size_t indicating the size of the buffer
// This implements a method handler. If the len is zero, the value should be read from the
// buffer. If the len is non-zero, the value should be written to the buffer.
#define IMPL_METHOD(C,X) int C::X(char *buffer, size_t len) 

// Function: setbits
// Set bits of a bitflag property
inline void setbits(unsigned long &flags, unsigned int bits) { flags|=bits; }; 

// Function: clearbits
// Clear bits of a bitflag property
inline void clearbits(unsigned long &flags, unsigned int bits) { flags&=~bits; }; 

// Function: hasbits
// Test bits of a bitflag property
inline bool hasbits(unsigned long flags, unsigned int bits) { return (flags&bits) ? true : false; };

/* 	Class: gld_object

	Object container
 */
class gld_object 
{
public:

	// Method: my
	// Returns: reference the object header
	inline OBJECT *my() { return (((OBJECT*)this)-1); }

public:

	// Operator: operator=
	inline gld_object &operator=(gld_object&o) { exception("copy constructor is forbidden on gld_object"); return *this;};

public: 

	// Method: find_object
	inline static gld_object *find_object(const char *n) { OBJECT *obj = callback->get_object(n); if (obj) return (gld_object*)(obj+1); else return NULL; };

public: 

	// Method: get_id
	inline OBJECTNUM get_id(void) { return my()->id; };

	// Method: get_groupid
	inline char* get_groupid(void) { return my()->groupid.get_string(); };

	// Method: get_oclass
	inline gld_class* get_oclass(void) { return (gld_class*)my()->oclass; };

	// Method: get_parent
	inline gld_object* get_parent(void) { return my()->parent?OBJECTDATA(my()->parent,gld_object):NULL; };

	// Method: get_rank
	inline OBJECTRANK get_rank(void) { return my()->rank; };

	// Method: get_clock
	inline TIMESTAMP get_clock(void) { return my()->clock; };

	// Method: get_valid_to
	inline TIMESTAMP get_valid_to(void) { return my()->valid_to; };

	// Method: get_schedule_skew
	inline TIMESTAMP get_schedule_skew(void) { return my()->schedule_skew; };

	// Method: get_forecast
	inline FORECAST* get_forecast(void) { return my()->forecast; };

	// Method: get_latitude
	inline double get_latitude(void) { return my()->latitude; };

	// Method: get_longitude
	inline double get_longitude(void) { return my()->longitude; };

	// Method: get_in_svc
	inline TIMESTAMP get_in_svc(void) { return my()->in_svc; };

	// Method: get_out_svc
	inline TIMESTAMP get_out_svc(void) { return my()->out_svc; };

	// Method: get_name
	inline const char* get_name(void) { static char _name[sizeof(CLASSNAME)+16]; return my()->name?my()->name:(sprintf(_name,"%s:%d",my()->oclass->name,my()->id),_name); };

	// Method: get_space
	inline NAMESPACE* get_space(void) { return my()->space; };

	// Method: get_lock
	inline unsigned int get_lock(void) { return my()->lock; };

	// Method: get_rng_state
	inline unsigned int get_rng_state(void) { return my()->rng_state; };

	// Method: get_heartbeat
	inline TIMESTAMP get_heartbeat(void) { return my()->heartbeat; };

	// Method: get_guid
	inline unsigned long long* get_guid(void) { return my()->guid; };

	// Method: get_guid_size
	inline size_t get_guid_size(void) { OBJECT *_t=my(); return sizeof(_t->guid)/sizeof(_t->guid[0]); };

	// Method: get_flags
	inline uint64 get_flags(uint64 mask=0xffffffffffffffff) { return (my()->flags)&mask; };

	// Method: get_header_string
	inline const char *get_header_string(const char *item, char *buffer=NULL, size_t len=0) { return callback->object.get_header_string(my(),item,buffer,len); };

protected: 

	// Method: set_clock
	inline void set_clock(TIMESTAMP ts=0) { my()->clock=(ts?ts:gl_globalclock); };

	// Method: set_heartbeat
	inline void set_heartbeat(TIMESTAMP dt) { my()->heartbeat=dt; };

	// Method: set_forecast
	inline void set_forecast(FORECAST *fs) { my()->forecast=fs; };

	// Method: set_latitude
	inline void set_latitude(double x) { my()->latitude=x; };

	// Method: set_longitude
	inline void set_longitude(double x) { my()->longitude=x; };

	// Method: set_flags
	inline void set_flags(uint64 flags) { my()->flags=flags; };

	// Method: set_flags_bits
	inline void set_flags_bits(uint64 bits) { my()->flags|=bits; };

	// Method: unset_flags_bits
	inline void unset_flags_bits(uint64 bits) { my()->flags&=~bits; };

protected: 

	// Method: rlock(void)
	inline void rlock(void) { ::rlock(&my()->lock); };

	// Method: runlock(void)
	inline void runlock(void) { ::runlock(&my()->lock); };

	// Method: wlock(void)
	inline void wlock(void) { ::wlock(&my()->lock); };

	// Method: wunlock(void)
	inline void wunlock(void) { ::wunlock(&my()->lock); };

protected: 

	// Method: rlock(OBJECT *obj)
	inline void rlock(OBJECT *obj) { ::rlock(&obj->lock); };

	// Method: runlock(OBJECT *obj)
	inline void runlock(OBJECT *obj) { ::runlock(&obj->lock); };

	// Method: wlock(OBJECT *obj)
	inline void wlock(OBJECT *obj) { ::wlock(&obj->lock); };

	// Method: wunlock(OBJECT *obj)
	inline void wunlock(OBJECT *obj) { ::wunlock(&obj->lock); };

protected: 

	// Method: operator==
	inline bool operator== (gld_object *o) { return o!=NULL && my()==o->my(); };

	// Method: operator==
	inline bool operator== (OBJECT *o) { return o!=NULL && my()==o; };

public: 

	// Method: get_property
	inline PROPERTY* get_property(PROPERTYNAME name, PROPERTYSTRUCT *pstruct=NULL) { return callback->properties.get_property(my(),name,pstruct); };

	// Method: get_function
	inline FUNCTIONADDR get_function(const char *name) { return (*callback->function.get)(my()->oclass->name,name); };

public: 

	// Method: getp(PROPERTY &prop, T &value)
	template <class T> inline void getp(PROPERTY &prop, T &value) { rlock(); value=*(T*)(GETADDR(my(),&prop)); wunlock(); };

	// Method: setp(PROPERTY &prop, T &value)
	template <class T> inline void setp(PROPERTY &prop, T &value) { wlock(); *(T*)(GETADDR(my(),&prop))=value; wunlock(); };

	// Method: getp(PROPERTY &prop, T &value, gld_rlock&)
	template <class T> inline void getp(PROPERTY &prop, T &value, gld_rlock&) { value=*(T*)(GETADDR(my(),&prop)); };

	// Method: getp(PROPERTY &prop, T &value, gld_wlock&)
	template <class T> inline void getp(PROPERTY &prop, T &value, gld_wlock&) { value=*(T*)(GETADDR(my(),&prop)); };

	// Method: setp(PROPERTY &prop, T &value, gld_wlock&)
	template <class T> inline void setp(PROPERTY &prop, T &value, gld_wlock&) { *(T*)(GETADDR(my(),&prop))=value; };

public: 

	// Method: set_dependent
	inline int set_dependent(OBJECT *obj) { return callback->object.set_dependent(my(),obj); };

	// Method: set_parent
	inline int set_parent(OBJECT *obj) { return callback->object.set_parent(my(),obj); };

	// Method: set_rank
	inline OBJECTRANK set_rank(unsigned int r) { return callback->object.set_rank(my(),r); };

	// Method: isa
	inline bool isa(const char *type, const char *module = NULL) { return callback->object_isa(my(),type,module) ? true : false; };

	// Method: is_valid
	inline bool is_valid(void) { return my()!=NULL && my()==THISOBJECTHDR; };

public: 

	// Method: is_last
	inline bool is_last(void) { return my()->next==NULL; };

	// Method: get_first
	inline static gld_object *get_first(void) { OBJECT *o=callback->object.get_first(); return OBJECTDATA(o,gld_object);};

	// Method: get_next
	inline gld_object* get_next(void) { return OBJECTDATA(my()->next,gld_object); };

public: 

	// Method: exception
	inline void exception(const char *msg, ...) { static char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); throw (const char*)buf;};

	// Method: error
	inline void error(const char *msg, ...) { if ( !(my()->flags&OF_QUIET) ) { char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); gl_error("%s",buf);}}

	// Method: warning
	inline void warning(const char *msg, ...) { if ( !(my()->flags&OF_WARNING) ) { char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); gl_warning("%s",buf);}}

	// Method: verbose
	inline void verbose(const char *msg, ...) { if ( my()->flags&OF_VERBOSE ) { char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); gl_verbose("%s",buf);}}

	// Method: debug
	inline void debug(const char *msg, ...) { if ( my()->flags&OF_DEBUG ) { char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); gl_debug("%s",buf);}}

	// Method: set_defaults
//	virtual void set_defaults(bool is_template = false); /* this force proper V4 initialization of objects (legacy defaults copy is no longer permitted) */
};

// Function: get_object(OBJECT*)
// Create a gld_object from an OBJECT*
static inline gld_object* get_object(OBJECT*obj)
{
	return obj ? (gld_object*)(obj+1) : NULL;
}

// Function: get_object(char*)
// Find a gld_object from an object name
static inline gld_object* get_object(char *n)
{
	OBJECT *obj = callback->get_object(n);
	return get_object(obj);
}

class gld_finder {
private:
	FINDLIST *findlist;

public:

	// Constructor: gld_finder
	inline gld_finder()
	{
		findlist = NULL;
	}

	// Method: create
	inline bool create(const char *search)
	{
		findlist = callback->find.list_create(NULL,search);
		return findlist != NULL;
	};

	// Method: get_first
	inline struct s_object_list *get_first(void)
	{
		return findlist ? gl_find_next(findlist,NULL) : NULL;
	};

	// Method: get_next
	inline struct s_object_list *get_next(struct s_object_list *item)
	{
		return gl_find_next(findlist,item);
	};
};

static PROPERTYSTRUCT nullpstruct;

/* 	Class: gld_property

	Property container
 */
class gld_property {

private: // data
	PROPERTYSTRUCT pstruct;
	OBJECT *obj;

public: 

	// Constructor: gld_property(void)
	inline gld_property(void) : pstruct(nullpstruct), obj(NULL) {};

	// Constructor: gld_property(gld_object *o, const char *n)
	inline gld_property(gld_object *o, const char *n) : pstruct(nullpstruct), obj(o->my())
	{ 
		if (o) 
			callback->properties.get_property(o->my(),n,&pstruct); 
		else 
		{
			GLOBALVAR *v=callback->global.find(n); 
			pstruct.prop= (v?v->prop:NULL);
		} 
	};
	
	// Constructor: gld_property(OBJECT *o, const char *n)
	inline gld_property(OBJECT *o, const char *n) : pstruct(nullpstruct), obj(o)
	{ 
		if (o) 
			callback->properties.get_property(o,n,&pstruct); 
		else 
		{
			GLOBALVAR *v=callback->global.find(n); 
			pstruct.prop= (v?v->prop:NULL);
		} 
	};

	// Constructor: gld_property(OBJECT *o) 
	inline gld_property(OBJECT *o) : pstruct(nullpstruct), obj(o) { pstruct.prop=o->oclass->pmap; };

	// Constructor: gld_property(OBJECT *o, PROPERTY *p)
	inline gld_property(OBJECT *o, PROPERTY *p) : pstruct(nullpstruct), obj(o) { pstruct.prop=p; };

	// Constructor: gld_property(GLOBALVAR *v)
	inline gld_property(GLOBALVAR *v) : pstruct(nullpstruct), obj(NULL) { pstruct.prop=v->prop; };

	// Constructor: gld_property(const char *n)
	inline gld_property(const char *n) : pstruct(nullpstruct), obj(NULL)
	{
		char oname[256], vname[256];
		if ( sscanf(n,"%[A-Za-z0-9_].%[A-Za-z0-9_.]",oname,vname)==2 )
		{
			obj = callback->get_object(oname);
			if ( obj )
			{
				callback->properties.get_property(obj,vname,&pstruct);
				return;
			}
		}
		GLOBALVAR *v=callback->global.find(n); 
		pstruct.prop = (v?v->prop:NULL);  
	};

	// Constructor: gld_property(const char *m, const char *n)
	inline gld_property(const char *m, const char *n) : pstruct(nullpstruct), obj(NULL)
	{
		obj = callback->get_object(m);
		if ( obj != NULL ) {
			callback->properties.get_property(obj, n, &pstruct);
			return;
		} 
		char1024 vn; 
		sprintf(vn,"%s::%s",m,n); 
		GLOBALVAR *v=callback->global.find(vn); 
		pstruct.prop= (v?v->prop:NULL);  
	};

	// Operator: PROPERTY*
	inline operator PROPERTY*(void) { return pstruct.prop; };

	// Operator: OBJECT*
	inline operator OBJECT*(void) { return obj; };

public: 

	// Method: get_object
	inline OBJECT *get_object(void) { return obj; };

	// Method: get_property
	inline PROPERTY *get_property(void) { return pstruct.prop; };

	// Method: get_offset
	inline size_t get_offset(gld_object *self) { return (char*)this - (char*)self; };
	
	// Method: get_class
	inline gld_class* get_class(void) { return (gld_class*)pstruct.prop->oclass; };

	// Method: get_name
	inline const char *get_name(void) { return pstruct.prop->name; };

	// Method: get_type
	inline gld_type get_type(void) { return gld_type(pstruct.prop->ptype); };

	// Method: get_size
	inline size_t get_size(void) { return (size_t)(pstruct.prop->size); };

	// Method: get_width
	inline size_t get_width(void) { return (size_t)(pstruct.prop->width); };

	// Method: get_access
	inline PROPERTYACCESS get_access(void) { return pstruct.prop->access; };

	// Method: get_access
	inline bool get_access(unsigned int bits, unsigned int mask=0xffff) {  return ((pstruct.prop->access&mask)|bits); };

	// Method: get_unit
	inline gld_unit* get_unit(void) { return (gld_unit*)pstruct.prop->unit; };

	// Method: get_addr
	inline void* get_addr(void) { return obj?((void*)((char*)(obj+1)+(unsigned int64)(pstruct.prop->addr))):pstruct.prop->addr; };

	// Method: get_first_keyword
	inline gld_keyword* get_first_keyword(void) { return (gld_keyword*)pstruct.prop->keywords; };

	// Method: get_description
	inline const char* get_description(void) { return pstruct.prop->description; };

	// Method: get_flags
	inline PROPERTYFLAGS get_flags(void) { return pstruct.prop->flags; };

	// Method: to_string
	inline int to_string(char *buffer, int size) { return callback->convert.property_to_string(pstruct.prop,get_addr(),buffer,size); };

	// Method: get_string
	inline gld_string get_string(const size_t sz=1024)
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};

	// Method: from_string
	inline int from_string(const char *string) { return callback->convert.string_to_property(pstruct.prop,get_addr(),string); };

	// Method: get_partname
	inline const char *get_partname(void) { return pstruct.part; };

	// Method: get_part
	inline double get_part(char *part=NULL) { return callback->properties.get_part(obj,pstruct.prop,part?part:pstruct.part); };

public: 

	// Method: set_object
	inline void set_object(OBJECT *o) { obj=o; };

	// Method: set_object
	inline void set_object(gld_object *o) { obj=o->my(); };

	// Method: set_property
	inline void set_property(char *n) { callback->properties.get_property(obj,n,&pstruct); };

	// Method: set_property
	inline void set_property(PROPERTY *p) { pstruct.prop=p; };

public: 

	// Method: is_valid
	inline bool is_valid(void) { return pstruct.prop!=NULL; }

	// Method: has_part
	inline bool has_part(void) { return pstruct.part[0]!='\0'; };

	// Method: is_complex
	inline bool is_complex(void) { if(pstruct.prop->ptype == PT_complex) return true; return false;}

	// Method: is_double
	inline bool is_double(void) { switch(pstruct.prop->ptype) { case PT_double: case PT_random: case PT_enduse: case PT_loadshape: return true; default: return false;} };

	// Method: is_integer
	inline bool is_integer(void) { switch(pstruct.prop->ptype) { case PT_int16: case PT_int32: case PT_int64: return true; default: return false;} };

	// Method: is_enumeration
	inline bool is_enumeration(void) { return pstruct.prop->ptype==PT_enumeration; };

	// Method: is_set
	inline bool is_set(void) { return pstruct.prop->ptype==PT_set; };

	// Method: is_character
	inline bool is_character(void) { switch(pstruct.prop->ptype) { case PT_char8: case PT_char32: case PT_char256: case PT_char1024: return true; default: return false;} };

	// Method: is_random
	inline bool is_random(void) { return pstruct.prop->ptype==PT_random; };

	// Method: is_enduse
	inline bool is_enduse(void) { return pstruct.prop->ptype==PT_enduse; };

	// Method: is_loadshape
	inline bool is_loadshape(void) { return pstruct.prop->ptype==PT_loadshape; };

	// Method: is_double_array
	inline bool is_double_array(void) { return pstruct.prop->ptype==PT_double_array; };

	// Method: is_complex_array
	inline bool is_complex_array(void) { return pstruct.prop->ptype==PT_complex_array; };

	// Method: is_objectref
	inline bool is_objectref(void) { return pstruct.prop->ptype==PT_object; };

	// Method: is_bool
	inline bool is_bool(void) { return pstruct.prop->ptype==PT_bool; };

	// TODO these need to use throw instead of returning overloaded values

	// Method: get_bool(void)
	inline bool get_bool(void) { return *(bool*)get_addr(); };

	// Method: get_double(void)
	inline double get_double(void) { errno=0; switch(pstruct.prop->ptype) { case PT_double: return *(double*)get_addr(); case PT_random: case PT_enduse: case PT_loadshape: return has_part() ? get_part() : *(double*)get_addr(); default: errno=EINVAL; return NaN;} };

	// Method: get_double(UNIT*to)
	inline double get_double(UNIT*to) { double rv = get_double(); return get_unit()->convert(to,rv) ? rv : QNAN; };

	// Method: get_double(gld_unit&to)
	inline double get_double(gld_unit&to) { double rv = get_double(); return get_unit()->convert(to,rv) ? rv : QNAN; };

	// Method: get_double(char*to)
	inline double get_double(const char*to) { double rv = get_double(); return get_unit()->convert(to,rv) ? rv : QNAN; };

	// Method: get_complex
	inline complex get_complex(void) { errno=0; if ( pstruct.prop->ptype==PT_complex ) return *(complex*)get_addr(); else return complex(QNAN,QNAN); };

	// Method: get_integer
	inline int64 get_integer(void) { errno=0; switch(pstruct.prop->ptype) { case PT_int16: return (int64)*(int16*)get_addr(); case PT_int32: return (int64)*(int32*)get_addr(); case PT_int64: return *(int64*)get_addr(); default: errno=EINVAL; return 0;} };

	// Method: get_enumeration
	inline enumeration get_enumeration(void) { if ( pstruct.prop->ptype != PT_enumeration ) exception("get_enumeration() called on a property that is not an enumeration"); return *(enumeration*)get_addr(); };

	// Method: get_set
	inline set get_set(void) { if ( pstruct.prop->ptype != PT_set ) exception("get_set() called on a property that is not a set"); return *(set*)get_addr(); };

	// Method: get_objectref
	inline gld_object* get_objectref(void) { if ( is_objectref() ) return ::get_object(*(OBJECT**)get_addr()); else return NULL; };

	// Method: getp(T &value)
	template <class T> inline void getp(T &value) { ::rlock(&obj->lock); value = *(T*)get_addr(); ::runlock(&obj->lock); };

	// Method: setp(T &value)
	template <class T> inline void setp(T &value) { ::wlock(&obj->lock); *(T*)get_addr()=value; ::wunlock(&obj->lock); };

	// Method: getp(T &value, gld_rlock&)
	template <class T> inline void getp(T &value, gld_rlock&) { value = *(T*)get_addr(); };

	// Method: getp(T &value, gld_wlock&)
	template <class T> inline void getp(T &value, gld_wlock&) { value = *(T*)get_addr(); };

	// Method: setp(T &value, gld_wlock&)
	template <class T> inline void setp(T &value, gld_wlock&) { *(T*)get_addr()=value; };

	// Method: setp(enumeration value)
	inline void setp(enumeration value) { ::wlock(&obj->lock); *(enumeration*)get_addr()=value; ::wunlock(&obj->lock); };

	// Method: setp(set value)
	inline void setp(set value) { ::wlock(&obj->lock); *(set*)get_addr()=value; ::wunlock(&obj->lock); };

	// Method: find_keyword(unsigned long value)
	inline gld_keyword* find_keyword(unsigned long value) { return get_first_keyword()->find(value); };

	// Method: find_keyword(const char *name)
	inline gld_keyword* find_keyword(const char *name) { return get_first_keyword()->find(name); };

	// Method: compare(char *op, char *a, char *b=NULL, char *p=NULL)
	inline bool compare(const char *op, const char *a, const char *b=NULL, char *p=NULL) 
	{ 
		PROPERTYCOMPAREOP n = callback->properties.get_compare_op(pstruct.prop->ptype,op); 
		if (n==TCOP_ERR) throw "invalid property compare operation"; 
		return compare((enumeration)n,a,b,p); 
	};

	// Method: compare(enumeration op, char *a, char *b=NULL) 
	inline bool compare(enumeration op, const char *a, const char *b=NULL) 
	{ 
		char v1[1024], v2[1024]; 
		return callback->convert.string_to_property(pstruct.prop,(void*)v1,a)>0 && callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),(void*)v1,(b&&callback->convert.string_to_property(pstruct.prop,(void*)v2,b)>0)?(void*)v2:NULL, NULL);
	};

	// Method: compare(enumeration op, char *a, char *b, char *p) 
	inline bool compare(enumeration op, const char *a, const char *b, const char *p) 
	{
		double v1, v2; v1=atof(a); v2=b?atof(b):0;
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),(void*)&v1,b?(void*)&v2:NULL, p);
	};

	// Method: compare(enumeration op, double *a, double *b=NULL, char *p=NULL) 
	inline bool compare(enumeration op, double *a, double *b=NULL, const char *p=NULL) 
	{ 
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),a,b,p);
	};

	// Method: compare(enumeration op, void *a, void *b=NULL) 
	inline bool compare(enumeration op, void *a, void *b=NULL) 
	{ 
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),a,b,NULL);
	};

	// Method: compare_string(enumeration op, char *a, char *b=NULL)
	inline bool compare_with_string(enumeration op, const char *a, const char *b=NULL, const char *p=NULL)
	{
		return callback->properties.compare_basic_str(pstruct.prop,(PROPERTYCOMPAREOP)op,get_addr(),a,b,p);
	};

	// Method: call(char *buffer, size_t len)
	inline int call(char *buffer, size_t len) { return (*(pstruct.prop->method))(obj,buffer,len); };

	// Method: call(const char *buffer) 
	inline int call(const char *buffer) { return (*(pstruct.prop->method))(obj,(char*)buffer,0); };

	// Method: callf(const char *format,...)
	inline int callf(const char *format,...) 
	{
		va_list ptr;
		va_start(ptr,format);
		char buffer[1024];
		vsprintf(buffer,format,ptr);
		va_end(ptr);
		return call((const char*)buffer);
	};

public: // iterators

	// Method: is_last
	inline bool is_last(void) { return pstruct.prop==NULL || pstruct.prop->next==NULL || pstruct.prop->oclass!=pstruct.prop->next->oclass; };

	// Method: get_next
	inline PROPERTY* get_next(void) { return is_last() ? NULL : pstruct.prop->next; };

public: // comparators

	// Method: perator==
	inline bool operator == (char* a) { return compare(TCOP_EQ,a,NULL); };

	// Method: operator<=
	inline bool operator <= (char* a) { return compare(TCOP_LE,a,NULL); };

	// Method: operator>=
	inline bool operator >= (char* a) { return compare(TCOP_GE,a,NULL); };

	// Method: operator!=
	inline bool operator != (char* a) { return compare(TCOP_NE,a,NULL); };

	// Method: operator<
	inline bool operator < (char* a) { return compare(TCOP_LT,a,NULL); };

	// Method: operator>
	inline bool operator > (char* a) { return compare(TCOP_GT,a,NULL); };

	// Method: inside
	inline bool inside(char* a, char* b) { return compare(TCOP_IN,a,b); };

	// Method: outside
	inline bool outside(char* a, char* b) { return compare(TCOP_NI,a,b); };

private: // exceptions

	// Method: exception
	inline void exception(const char *msg, ...)
	{ 
		static char buf[1024]; 
		va_list ptr; 
		va_start(ptr,msg); 
		vsprintf(buf+sprintf(buf,"%s.%s: ",OBJECTDATA(obj,gld_object)->get_name(),pstruct.prop->name),msg,ptr); 
		va_end(ptr); 
		throw (const char*)buf;
	};
};

/* 	Class: gld_global

	Global variable container
 */
class gld_global {

private: // data
	GLOBALVAR *var;

public: 

	// Constructor: gld_global(void)
	inline gld_global(void) { var=callback->global.find(NULL); };

	// Constructor: gld_global(GLOBALVAR *v)
	inline gld_global(GLOBALVAR *v) : var(v) {};

	// Constructor: gld_global(const char *n)
	inline gld_global(const char *n) { var=callback->global.find(n); };

	// Constructor: gld_global(const char *n, PROPERTYTYPE t, void *p)
	inline gld_global(const char *n, PROPERTYTYPE t, void *p) { var=callback->global.create(n,t,p,NULL); };

	// Method: GLOBALVAR*
	inline operator GLOBALVAR*(void) { return var; };

	// Method: get_name
	inline const char* get_name(void) { return var->prop->name; };

	// Method: is_valid
	inline bool is_valid(void) { return var!=NULL; };

	// Method: get_property
	inline PROPERTY* get_property(void) { if (!var) return NULL; return var->prop; };

	// Method: get_flags
	inline unsigned long get_flags(void) { if (!var) return -1; return var->flags; };

	// Method: to_string
	inline size_t to_string(char *bp, size_t sz) { if (!var) return -1; gld_property p(var); return p.to_string(bp,(int)sz); };

	// Method: get_string
	inline gld_string get_string(const size_t sz=1024)
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};

	// Method: get_bool
	inline bool get_bool(void) { return *(bool*)(var->prop->addr); };

	// Method: get_int16
	inline int16 get_int16(void) { return *(int16*)(var->prop->addr); };

	// Method: get_int32
	inline int32 get_int32(void) { return *(int32*)(var->prop->addr); };

	// Method: get_int64
	inline int64 get_int64(void) { return *(int64*)(var->prop->addr); };

	// Method: get_double
	inline double get_double(void) { return *(double*)(var->prop->addr); };

	// Method: get_complex
	inline complex get_complex(void) { return *(complex*)(var->prop->addr); };

	// Method: get_timestamp
	inline TIMESTAMP get_timestamp(void) { return *(TIMESTAMP*)(var->prop->addr); };

	// Method: from_string
	inline size_t from_string(const char *bp) { if (!var) return -1; gld_property p(var); return p.from_string(bp); };

	// Method: get
	inline bool get(char *n) { var=callback->global.find(n); return var!=NULL; };

	// Method: create
	inline bool create(char *n, PROPERTYTYPE t, void *p) { var=callback->global.create(n,t,p,NULL); return var!=NULL; };

	// Method: get_first
	inline GLOBALVAR* get_first(void) { return callback->global.find(NULL); };

	// Method: is_last
	inline bool is_last(void) { if (!var) return false; else return (var->next==NULL); };

	// Method: get_next
	inline GLOBALVAR* get_next(void) { if (!var) return NULL; else return var->next; };
};

/* 	Class: gld_aggregate

	Aggregation container
 */
class gld_aggregate {
private:
	AGGREGATION *aggr;
public:

	// Constructor: gld_aggregate(void) 
	inline gld_aggregate(void) { aggr=NULL; };

	// Constructor: gld_aggregate(char *spec, char *group)
	inline gld_aggregate(char *spec, char *group) { set_aggregate(spec,group); };
public:

	// Method: set_aggregate
	inline bool set_aggregate(char *spec, char *group) { aggr=callback->aggregate.create(spec,group); return aggr!=NULL; };

	// Method: is_valid
	inline bool is_valid(void) { return aggr!=NULL; };

	// Method: get_value
	inline double get_value(void) { if (!aggr) throw "null aggregate"; return callback->aggregate.refresh(aggr); };
};

/* 	Class: gld_objlist

	Object list container
 */
class gld_objlist {
private:
	struct s_objlist *list;

public:

	// Constructor: gld_objlist(void)
	inline gld_objlist(void) : list(NULL) {};

	// Constructor: gld_objlist(char *group)
	inline gld_objlist(char *group) { list=callback->objlist.search(group); };

	// Constructor: gld_objlist(CLASS *c, PROPERTY *m, char *p, char *o, void *a, void *b=NULL)
	inline gld_objlist(CLASS *c, PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { list=callback->objlist.create(c,m,p,o,a,b); };

	// Constructor: gld_objlist(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	inline gld_objlist(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{ 
		CLASS *c=callback->class_getname(cn,NULL); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		list=callback->objlist.create(c,m,p,o,a,b); 
	};

	// Destructor: ~gld_objlist(void) 
	inline ~gld_objlist(void) { callback->objlist.destroy(list); };

	// Method: OBJLIST*
	inline operator OBJLIST*() { return list; };

	// Method: get
	inline OBJECT *get(size_t n) { return list->objlist[n]; };

	// Method: set
	inline size_t set(char *group) { if ( list ) callback->objlist.destroy(list); list=callback->objlist.search(group); return list?list->size:-1; };

	// Method: add(PROPERTY *m, char *p, char *o, void *a, void *b=NULL)
	inline size_t add(PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { return callback->objlist.add(list,m,p,o,a,b); };

	// Method: add(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL)
	inline size_t add(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{
		CLASS *c=callback->class_getname(cn,NULL); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		return callback->objlist.add(list,m,p,o,a,b); 
	};

	// Method: del(PROPERTY *m, char *p, char *o, void *a, void *b=NULL)
	inline size_t del(PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { return callback->objlist.add(list,m,p,o,a,b); };

	// Method: del(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL)
	inline size_t del(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{ 
		CLASS *c=callback->class_getname(cn,NULL); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		return callback->objlist.add(list,m,p,o,a,b); 
	};

	// Method: is_valid
	inline bool is_valid(void) { return list!=NULL; };

	// Method: get_size
	inline size_t get_size(void) { return list->size; };

	// Method: apply
	inline int apply(void *arg, int (*function)(OBJECT *,void*,int)) { return callback->objlist.apply(list,arg,function);};

	// Method: exception
	inline void exception(const char *msg, ...) { static char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf,msg,ptr); va_end(ptr); throw (const char*)buf;};
};

#include "http_client.h"

/*	Class: gld_webdata

	Web data container
 */
class gld_webdata {
private:
	HTTPRESULT *result;
public:

	// Constructor: gld_webdata(void)
	inline gld_webdata(void) {result=NULL;};

	// Constructor: gld_webdata(char *url, size_t maxlen=4096)
	inline gld_webdata(char *url, size_t maxlen=4096) {open(url,maxlen);};

	// Constructor: ~gld_webdata
	inline ~gld_webdata(void) {};
public:

	// Method: open
	inline bool open(char *url, size_t maxlen=4096) { result=callback->http.read(url,(int)maxlen); return is_valid();};

	// Method: close
	inline void close(void) { callback->http.free(result);};

	// Method: is_valid
	inline bool is_valid(void) { return result!=NULL; };

	// Method: get_header
	inline const char *get_header(void) { return result->header.data;};

	// Method: get_header_size
	inline size_t get_header_size(void) { return result->header.size; };

	// Method: get_body
	inline const char *get_body(void) { return result->body.data; };

	// Method: get_body_size
	inline size_t get_body_size(void) { return result->body.size; };

	// Method: get_status
	inline int get_status(void) { return result->status; };
};


/*	Section: Module main

	Every module must implement certain main function.  To signal that the <gridlabd.h>
	must define the main function, define the <DLMAIN> macro before including the header.

	--- C++ Code ---
	#define DLMAIN
	#include "gridlabd.h"
	--- End Code ---
 */

#ifdef DLMAIN

/*	Function: do_kill
 */
EXPORT int do_kill(void*);

/*	Variable: module_message_flags

	This variable control which module message streams are active. See <MMF_ALL>.
 */
set module_message_flags = MMF_NONE;

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int gld_major=MAJOR, gld_minor=MINOR; 
BOOL APIENTRY DllMain(HANDLE h, DWORD r) { if (r==DLL_PROCESS_DETACH) do_kill(h); return TRUE; }

#else // !WIN32

int gld_major=MAJOR, gld_minor=MINOR; 

//	Function: dllinit
int dllinit() __attribute__((constructor));
int dllinit() { return 0; }

//	Function: dllkill
int dllkill() __attribute__((destructor));
int dllkill() { return do_kill(NULL); }

#endif // !WIN32

#elif defined CONSOLE
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "console.h"

#endif // DLMAIN

#define INIT_MMF(M) { \
	static KEYWORD mmf_keys[] = { \
		{"QUIET",MMF_QUIET,mmf_keys+1}, \
		{"WARNING",MMF_WARNING,mmf_keys+2}, \
		{"DEBUG",MMF_DEBUG,mmf_keys+3}, \
		{"VERBOSE",MMF_VERBOSE,NULL}, \
	}; \
	GLOBALVAR *var = gl_global_create(#M "::message_flags",PT_set,&module_message_flags,PT_DESCRIPTION,"module message control flags",NULL); \
	var->prop->keywords = mmf_keys; \
}

/*	Define: EXPORT_CREATE_C(classname,class)

	This macro is used to implement the create function of a class whose classname differs from the C++ class.
	See <EXPORT_CREATE>.
 */
#define EXPORT_CREATE_C(X,C) EXPORT int create_##X(OBJECT **obj, OBJECT *parent) \
{	try { *obj = gl_create_object(C::oclass); \
	if ( *obj != NULL ) { C *my = OBJECTDATA(*obj,C); \
		gl_set_parent(*obj,parent); (*obj)->flags|=module_message_flags; return my->create(); \
	} else return 0; } CREATE_CATCHALL(X); }

/*	Define: EXPORT_CREATE(class)

	This macro is used to implement the create function of a class.
	See <EXPORT_CREATE_C>.
 */
#define EXPORT_CREATE(X) EXPORT_CREATE_C(X,X)

/*	Define: EXPORT_INIT_C(classname,class)

	This macro is used to implement the init function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_INIT>.
 */
#define EXPORT_INIT_C(X,C) EXPORT int init_##X(OBJECT *obj, OBJECT *parent) \
{	try { if (obj!=NULL) return OBJECTDATA(obj,C)->init(parent); else return 0; } \
	INIT_CATCHALL(X); }

/*	Define: EXPORT_INIT(class)

	This macro is used to implement the init function of a class.
	See <EXPORT_INIT_C>.
 */
#define EXPORT_INIT(X) EXPORT_INIT_C(X,X)

/*	Define: EXPORT_COMMIT_C(classname,class)

	This macro is used to implement the commit function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_COMMIT>.
 */
#define EXPORT_COMMIT_C(X,C) EXPORT TIMESTAMP commit_##X(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->commit(t1,t2) : TS_NEVER; } \
	T_CATCHALL(C,commit); }

/*	Define: EXPORT_COMMIT(class)

	This macro is used to implement the commit function of a class.
	See <EXPORT_COMMIT_C>.
 */
#define EXPORT_COMMIT(X) EXPORT_COMMIT_C(X,X)

/*	Define: EXPORT_NOTIFY_C(classname,class)

	This macro is used to implement the notify function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_NOTIFY>.
 */
#define EXPORT_NOTIFY_C(X,C) EXPORT int notify_##X(OBJECT *obj, int notice, PROPERTY *prop, const char *value) \
{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
	switch (notice) { \
	case NM_POSTUPDATE: return my->postnotify(prop,value); \
	case NM_PREUPDATE: return my->prenotify(prop,value); \
	default: return 0; } } else return 0; } \
	T_CATCHALL(X,commit); return 1; }

/*	Define: EXPORT_NOTIFY(class)

	This macro is used to implement the notify function of a class.
	See <EXPORT_NOTIFY_C>.
 */
#define EXPORT_NOTIFY(X) EXPORT_NOTIFY_C(X,X)

/*	Define: EXPORT_SYNC_C(classname,class)

	This macro is used to implement the sync function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_SYNC>.
 */
#define EXPORT_SYNC_C(X,C) EXPORT TIMESTAMP sync_##X(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) { \
	try { TIMESTAMP t1=TS_NEVER; C *p=OBJECTDATA(obj,C); \
	switch (pass) { \
	case PC_PRETOPDOWN: t1 = p->presync(t0); break; \
	case PC_BOTTOMUP: t1 = p->sync(t0); break; \
	case PC_POSTTOPDOWN: t1 = p->postsync(t0); break; \
	default: throw "invalid pass request"; break; } \
	if ( (obj->oclass->passconfig&(PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN)&(~pass)) <= pass ) obj->clock = t0; \
	return t1; } \
	SYNC_CATCHALL(X); }

/*	Define: EXPORT_SYNC(class)

	This macro is used to implement the sync function of a class.
	See <EXPORT_SYNC_C>.
 */
#define EXPORT_SYNC(X) EXPORT_SYNC_C(X,X)

/*	Define: EXPORT_ISA_C(classname,class)

	This macro is used to implement the isa function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_ISA>.
 */
#define EXPORT_ISA_C(X,C) EXPORT int isa_##X(OBJECT *obj, CLASSNAME name) { \
	return ( obj!=0 && name!=0 ) ? OBJECTDATA(obj,C)->isa(name) : 0; }

/*	Define: EXPORT_ISA(class)

	This macro is used to implement the isa function of a class.
	See <EXPORT_ISA_C>.
 */
#define EXPORT_ISA(X) EXPORT_ISA_C(X,X)

/*	Define: EXPORT_PLC_C(classname,class)

	This macro is used to implement the plc function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_PLC>.
 */
#define EXPORT_PLC_C(X,C) EXPORT TIMESTAMP plc_##X(OBJECT *obj, TIMESTAMP t1) { \
	try { return OBJECTDATA(obj,C)->plc(t1); } \
	T_CATCHALL(plc,X); }

/*	Define: EXPORT_PLC(class)

	This macro is used to implement the plc function of a class.
	See <EXPORT_PLC>.
 */
#define EXPORT_PLC(X) EXPORT_PLC_C(X,X)

/*	Define: EXPORT_PRECOMMIT_C(classname,class)

	This macro is used to implement the precommit function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_PRECOMMIT>.
 */
#define EXPORT_PRECOMMIT_C(X,C) EXPORT int precommit_##X(OBJECT *obj, TIMESTAMP t1) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->precommit(t1) : 0; } \
	T_CATCHALL(C,precommit); }

/*	Define: EXPORT_PRECOMMIT(class)

	This macro is used to implement the precommit function of a class.
	See <EXPORT_PRECOMMIT_C>.
 */
#define EXPORT_PRECOMMIT(X) EXPORT_PRECOMMIT_C(X,X)

/*	Define: EXPORT_FINALIZE_C

	This macro is used to implement the finalize function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_FINALIZE>.
 */
#define EXPORT_FINALIZE_C(X,C) EXPORT int finalize_##X(OBJECT *obj) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->finalize() : 0; } \
	T_CATCHALL(C,finalize); }

/*	Define: EXPORT_FINALIZE(class)

	This macro is used to implement the finalize function of a class.
	See <EXPORT_FINALIZE_C>.
 */
#define EXPORT_FINALIZE(X) EXPORT_FINALIZE_C(X,X)

/*	Define: EXPORT_NOTIFY_C_P(classname,class,property)

	This macro is used to implement the property notify function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_NOTIFY_PROP>.
 */
#define EXPORT_NOTIFY_C_P(X,C,P) EXPORT int notify_##X##_##P(OBJECT *obj, const char *value) \
{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
	return my->notify_##P(value); \
	} else return 0; } \
	T_CATCHALL(X,notify_##P); return 1; }

/*	Define: EXPORT_NOTIFY_PROP(class,property)

	This macro is used to implement the property notify function of a class.
	See <EXPORT_NOTIFY_C_P>.
 */
#define EXPORT_NOTIFY_PROP(X,P) EXPORT_NOTIFY_C_P(X,X,P)

/*	Define: EXPORT_LOADMETHOD_C(classname,class,name)

	This macro is used to implement a load method function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_LOADMETHOD>.
 */
#define EXPORT_LOADMETHOD_C(X,C,N) EXPORT int loadmethod_##X##_##N(OBJECT *obj, char *value, size_t len=0) \
{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
	return my->N(value,len); \
	} else return 0; } \
	T_CATCHALL(X,loadmethod); }

/*	Define: EXPORT_LOADMETHOD(class,name)

	This macro is used to implement a load method function of a class.
	See <EXPORT_LOADMETHOD_C>.
 */
#define EXPORT_LOADMETHOD(X,N) EXPORT_LOADMETHOD_C(X,X,N)

/*	Define: DECL_METHOD(class,name)

	This macro is used to declare a method function of a class.
	See <EXPORT_METHOD_C>.
 */
#define DECL_METHOD(X,N) EXPORT int method_##X##_##N(OBJECT *obj, char *value, size_t size)

/*	Define: EXPORT_METHOD_C(classname,class,name)

	This macro is used to implement a method function of a class when the GridLAB-D class name differs from the C++ class name.
	See <EXPORT_METHOD>.
 */
#define EXPORT_METHOD_C(X,C,N) DECL_METHOD(X,N) \
		{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
			return my->N(value,size); \
			} else return 0; } \
			T_CATCHALL(X,method); }

/*	Define: EXPORT_METHOD(class,name)

	This macro is used to implement a method function of a class.
	See <EXPORT_METHOD_C>.
 */
#define EXPORT_METHOD(X,N) EXPORT_METHOD_C(X,X,N)

/*	Define: EXPORT_DESTROY(class)

	This macro is used to implement a destroy function of a class.
 */
#define EXPORT_DESTROY_C(X,N) EXPORT void destroy_##X(OBJECT *obj) { OBJECTDATA(obj,N)->destroy(); free(obj); }
#define EXPORT_DESTROY(C) EXPORT_DESTROY_C(C,C)

#endif

/****************************************
 * GENERAL SOLVERS 
 ****************************************/
#ifdef USE_GLSOLVERS

#if defined WIN32 && ! defined __MINGW32__
	#define _WIN32_WINNT 0x0400
	#undef int64 // wtypes.h also used int64
	#include <windows.h>
	#define int64 _int64
	#define PREFIX ""
	#ifndef DLEXT
		#define DLEXT ".dll"
	#endif
	#define DLLOAD(P) LoadLibrary(P)
	#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
	#define snprintf _snprintf
#else /* ANSI */
#ifndef __MINGW32__
	#include "dlfcn.h"
#endif
	#define PREFIX ""
	#ifndef DLEXT
		#define DLEXT ".so"
	#endif
#ifndef __MINGW32__
	#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#else
	#include "dlfcn.h"
	#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#endif
	#define DLSYM(H,S) dlsym(H,S)
#endif

inline int null_function(void)
{
	return 0;
}

/*	Class: gld_solver

	Solver container for module solvers.
 */
class gld_solver {
public:

	/*	Function: init

		This function is called to initialize the solver.
	 */
	int (*init)(void*);
	
	/*	Function: solve

		This function is called to run to solver.
	 */
	int (*solve)(void*);
	
	/*	Function: set

		This function is called to set a solver input
	 */
	int (*set)(const char*,...);
	
	/*	Function: get

		This function is called to get a solver output
	 */
	int (*get)(const char*,...);

	/*	Function: term

		This function is called to terminate the solver.
	 */
	int (*term)(void);
private:
	inline void exception(const char *fmt,...)
	{
		static char buffer[1024]="";
		va_list ptr;
		va_start(ptr,fmt);
		int len = vsprintf(buffer,fmt,ptr);
		va_end(ptr);
		if ( errno!=0 )
			sprintf(buffer+len," (%s)", strerror(errno));
		throw (const char*)buffer;
	};
public:
	/*	Constructor: gld_solver

		The constructor is given the name and dyanmic link/shared object library
		that implements it.  The library must export four functions

		init - called when the solver is initialized
		set - called to set the solver inputs
		solve - called to run the solver
		get - called to get the solver outputs
		term - called to terminate the solver (NULL if not implemented)
	 */
	inline gld_solver(const char *name, const char *lib="glsolvers" DLEXT)
	{
		char path[1024];
		errno = 0;
		if ( callback->file.find_file(lib,NULL,X_OK,path,sizeof(path))!=NULL )
		{
			errno = 0;
			void* handle = DLLOAD(path);
			if ( handle==NULL )
				exception("glsolver(char *name='%s'): load of '%s' failed",name,path);
			else
			{
				char fname[64];
				struct {
					const char *part;
					void **func;
					bool optional;
				} map[] = {
					{"init", (void**)&init},
					{"solve", (void**)&solve},
					{"set", (void**)&set},
					{"get", (void**)&get},
					{"term", (void**)&term, true},
				};
				size_t n;
				for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
				{
					strcpy(fname,name);
					strcat(fname,"_");
					strcat(fname,map[n].part);
					errno = 0;
					*(map[n].func) = (void*)DLSYM(handle,fname);
					if ( *(map[n].func)==NULL )
					{
						if ( ! map[n].optional )
							exception("glsolver(char *name='%s'): function '%s' not found in '%s'",name,fname,path);
						else
							*(map[n].func) = (void*)null_function;
					}
				}
				errno = 0;
				if ( !(*init)(callback) )
					exception("glsolver(char *name='%s'): init failed",name);
			}
		}
		else
			exception("glsolver(char *name='%s'): solver library '%s' not found", name, lib);
	};
};

/*	Function: method_extract

	This function extract the values associated with a method.

	This function is preview.
 */
inline int method_extract(char *value, va_list args)
{
	char *buffer = va_arg(args,char*);
	size_t size = va_arg(args,size_t);
	int offset = va_arg(args,int);
	char *delims = va_arg(args,char*);
	size_t len = strcspn(value+offset,delims);
	if ( len < size ) // result will fit in buffer
	{
		strncpy(buffer,value+offset,len);
		buffer[len] = '\0';
		return value[offset+len] == '\0' ? 0 : offset+len+1;
	}
	return -1;
}

#endif // __cplusplus

inline PyObject *python_import(const char *module, const char *path=NULL)
{
	return callback->python.import(module, path);
}

inline bool python_call(PyObject *pModule, void *result, const char *method, const char *vargsfmt, ...)
{
	va_list ptr;
	va_start(ptr,vargsfmt);
	bool ok = callback->python.call(pModule,method,vargsfmt,ptr,result);
	va_end(ptr);
	return ok;
}

class GldTimer 
{
private:
	struct timeval start;
public:
	GldTimer(void) { restart(); };
	inline void restart(void) { gettimeofday(&start,NULL);};
	inline double elapsed(void) 
	{ 
		struct timeval now; 
		gettimeofday(&now,NULL); 
		return (now.tv_sec-start.tv_sec) + (now.tv_usec-start.tv_usec)/1.0e6;
	};
};

/** @} **/
#endif

