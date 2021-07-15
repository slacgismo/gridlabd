# Modules

Runtime modules are implemented as dynamic libraries that are loaded as needed.  The model loaded determines whether a runtime module is needed by specifying a [[module]] block.

; Prior to {{VERSION20}} : Adding a module in Windows can be done using the "Add GridLAB module" wizard.  In Linux it can be done using the "add_gridlab_module" script. 	This document is provided for completeness and to provide details that may be necessary should the scripts not function as required.

The exact approach to use in building a GridLAB module is not clear cut.
In general a module is a solver that can compute the steady state of
a collection of objects given a specific boundary condition.  For example,
a power flow solver makes sense as a module because the steady state
of a flow network can be directly computed.  However, a market clearing
system and a load simulation doesn't make sense because the market is
influenced not only by demand from loads, but also by supply.  As a general
rule, if a set of simultaneous equations can be solved to obtain the state
of a system, the system is suitable for implementation as a module.

Modules must be able to implement a least three capabilities:
* they must be able to create objects on demand (see create)
* they must be able to initialize objects on demand (see init)
* they must be able to compute the state of individual objects at a specified date and time on demand (see sync)

In addition, modules generally should be able to implement the following

* compute the states of objects with some degree of parallelism
* load and save data in various exchange formats
* inform the GridLAB-D core which objects data members are exposed to other modules
* handle notification events of data members about to be changed or just changed
* determine whether an object is a subtype of another object
* verify that a collection of objects form a self-consistent and correct model

It turns out that implementing these capabilities is not as easy as it
at first seems.  In particular, the synchronization has typically been one
of the most challenging concepts for programmers to understand.  Given
the amount of time spend in sync calls, it is recommended that
considerable time and effort be put into its design.

= Basic Synchronization =

An object's sync method actually performs two essential functions. First,
it updates the state of an object to a designated point in time, and second
it lets the core know when the object is next expected to change state. This
is vital for the core to know because the core's clock will be advanced
to the time of the next expected state change, and all objects will be
synchronized to that time.

In general a sync() function should be aware of three time values:
* <math>t_0</math> is the time from which the object is being advanced.  This is not the current time, because it is presumed that the object has not yet advanced to the current time and this is why ''sync''() is being called.
* <math>t_1</math> is the time to which the object is being advanced.  Think
of this as \e now from the object's perspective. This is usually the
current time from the core's perspective (but don't assume it \e always is).
* <math>t_2</math> is the time at which the object expects to have its next
state change.  This time must be computed by the object during or immediately
after the state is advanced to <math>t_1</math>.  This is the time returned to the
core should the ''sync''() call succeed.

If no state change is ever anticipated
then <math>t_2 = TS\_NEVER</math> is returned, indicating that barring any changes to its
boundary condition, the object is in steady state.

If an object's ''sync''() method determines that
the object is not yet in steady state (i.e., the module has not converged), then
<math>t_2 = t_1</math> is returned.

If an object's ''sync''() method determines that it cannot update to <math>t_1</math> as required,
the simulation has failed. It can either throw an exception using ''GL_THROW''() or
return <math>t_2 < t_1</math> to indicate the time at which the problem is believed to have occurred.

The time window <math>t_1 - t_0</math> is the past window and the ''sync''() method
must implement all behaviors during that time as though they have already occurred.

The time window <math>t_2 - t_1</math> is the future window and the sync function
must not implement behaviors in this window yet, as they have not yet occurred.

It is a non-trivial fact that if all objects in all modules in GridLAB model
return <math>t_2 = TS\_NEVER</math>, then the simulation stops successfully.  This
is because the system has completely settle to a steady state and there is
nothing further to compute.

For more details on synchronization in GridLAB-D see [[theory of operation]].

= Control Code =

One very important aspect of synchronization behavior is how control code
is handled when object behavior goes beyond the mere physics of its
response to its boundary condition.  It is quite easy to implement
control code that is integrated with the physical model.  However, this
would prevent users from altering the control code without altering the
source code of the object's implementation.

; Prior to {{VERSION30}} : To address this problem, objects can implement default control code that is disabled if a [[PLC]] object is attached later.  The ability to alter control code should be made available when implemented for any object for which this is a realistic possibility, which is very nearly always.

: To implement default machine [[PLC]] code for an object, the module must expose a ''plc''() method that will be called immediately before ''sync''() is called, but only if not external PLC method is provided in the model.  This ''plc''() method may be written as though it was integrated with the physics implemented in ''sync''(), but the physics must be able to update even when the default [[PLC]] code is not run.

; {{VERSION30}} and later : As of {{VERSION30}} the PLC module will no longer be supported and the [[PID controller]] module replaces it.  Alternatively, the [[class]] [[class#intrinsic|intrinsic]] function [[class#plc|plc]] supports simple control code replacements.

= Building a GridLAB module =

A GridLAB module must be a Windows DLL or a Linux SO.

=== [[VS2005]] ===
The compile options that are typically required are
* Include path: ../core
* Runtime library: Multi-threaded Debug DLL (/MDd)

For example the command-line options used by the climate module is

 compiler: /Od /I "../core" /I "../test/cppunit2/include"
 	/D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL"
 		/D "_CRT_SECURE_NO_DEPRECATE" /D "_WINDLL" /D "_MBCS"
 		/Gm /EHsc /RTC1 /MDd /Fo"Win32\Debug\climate\"
 		/Fd"Win32\Debug\climate\vc80.pdb" /W3 /nologo /c /Wp64
 		/ZI /TP /errorReport:prompt
 
 	linker: /OUT:"Win32\Debug\climate.dll" /INCREMENTAL /NOLOGO
 	/LIBPATH:"..\test\cppunit2\lib" /DLL /MANIFEST
 	/MANIFESTFILE:"Win32\Debug\climate\climate.dll.intermediate.manifest"
 	/DEBUG /PDB:"c:\projects\GridlabD\source\VS2005\Win32\Debug\climate.pdb"
 	/SUBSYSTEM:WINDOWS /MACHINE:X86 /ERRORREPORT:PROMPT cppunitd.lib
 	kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
 	advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

=== Linux makefiles ===

; configure.ac: You must add the module's makefile to the AC_CONFIG_FILES variable near the end of the list (but not in the last place).

; Makefile.am: You must add the module folder location to the SUBDIRS variable in the last position.

=== Module implementation Prior to {{VERSION30}} ===

The <tt>main.cpp</tt> file contains the code to load and unload both Windows DLL
and Linux/Unix shared-object libraries:

 	// the version can be used to make sure the right module is loaded
 	#define MAJOR 0 // TODO: set the major version of your module here
 	#define MINOR 0 // TODO: set the minor version of your module here
 
 	#define DLMAIN
 	#include <stdlib.h>
 	#include "gridlabd.h"
 	EXPORT int do_kill(void*);
 	EXPORT int major=MAJOR, minor=MINOR;
 
 	#ifdef WIN32
 	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
 	#include <windows.h>
 	BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID)
 	{
 		switch (ul_reason_for_call)
 		{
 			case DLL_PROCESS_ATTACH:
 			case DLL_THREAD_ATTACH:
 				break;
 			case DLL_THREAD_DETACH:
 			case DLL_PROCESS_DETACH:
 				do_kill(hModule);
 				break;
 		}
 		return TRUE;
 	}
 	#else // !WIN32
 	CDECL int dllinit() __attribute__((constructor));
 	CDECL int dllkill() __attribute__((destructor));
 	CDECL int dllinit() { return 0;}
 	CDECL int dllkill() { do_kill(NULL);}
 	#endif // !WIN32
 	\endcode
 
The <tt>init.cpp</tt> file contains the code needed to initialize a module
after it is loaded:

 	#include <stdlib.h>
 	#include <stdio.h>
 	#include <errno.h>
 	#include "gridlabd.h"
 	#include "myclass.h"
 	EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
 	{
 		// set the GridLAB core API callback table
 		callback = fntable;
 
 		// TODO: register each object class by creating its first instance
 		new myclass(module);
 
 		// always return the first class registered
 		return myclass::oclass;
 	}
 	CDECL int do_kill()
 	{
 		// if anything needs to be deleted or freed, this is a good time to do it
 		return 0;
 	}

You can also implement <tt>EXPORT int check(void)</tt> to allow the core
to request a check of the objects that are implemented by the module.  This is
particularly useful to perform topology checks for network models.

If you implement the <tt>EXPORT int import_file(char *file)</tt> this will
permit users to use the [[import]] command in the model definition.

If you implement the <tt>EXPORT int save(char *file)</tt> this will permit
users to request saves in formats other than '''.glm''' or '''.xml'''.

; Prior to {{VERSION20}} : The <tt>EXPORT int setvar(char *varname, char *value)</tt> and the
<tt>EXPORT void* getvar(char *varname, char *value, unsigned int size)</tt>
are implemented when you wish to allow the user to alter any of the module's global
variables.  See <tt>network/varmap.c</tt> for an example.

: <tt>EXPORT int module_test(TEST_CALLBACKS *callbacks,int argc, char* argv[])</tt>
function implements the module's unit test code.  See "Unit testing" for
more information.

The <tt>EXPORT MODULE *subload(char *modname, MODULE **pMod, CLASS **cptr, int argc, char **argv)</tt>
function is used to load modules written in foreign languages.  Look at the gldjava project
for examples of how this is used.  This function needs to manually set the function pointers
for any classes registered by subload-ed modules.  A module subload is triggered with
"module X::Y".

The implementation of each class will require two files for each object class
be included in your module's source code.  The header file will usually include
the following:

 	#ifndef _MYCLASS_H
 	#define _MYCLASS_H
 	#include "gridlabd.h"
 	class myclass {
 	public:
 		// TODO: add your published variables here using GridLAB types (see PROPERTY)
 		double myDouble; // just an example
 	private:
 		// TODO: add any unpublished variables here (any type is ok)
 		double *pDouble; // just an example
 	public:
 		static CLASS *oclass;
 		static myclass *defaults;
 	public:
 		myclass(MODULE *module);
 		int create(void);
 		int init(OBJECT *parent);
 		TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
 	};
 	#endif

The implementation file should include the following:
 	#include <stdlib.h>
 	#include <stdio.h>
 	#include <errno.h>
 	#include "myclass.h"
 	CLASS *myclass::oclass = NULL;
 	myclass *myclass::defaults = NULL;
 	myclass::myclass(MODULE *module)
 	{
 		if (oclass==NULL)
 		{
 			oclass = gl_register_class(module,"myclass",PC_BOTTOMUP);
 			if (gl_publish_variable(oclass,
 				// TODO: publish your variables here
 				PT_double, "my_double", PADDR(myDouble), // just an example
 				NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
 			defaults = this;
 			// TODO: initialize the default values that apply to all objects of this class
 			myDouble = 1.23; // just an example
 			pDouble = NULL; // just an example
 		}
 	}
 	void myclass::create(void)
 	{
 		memcpy(this,defaults,sizeof(*this));
 		// TODO: initialize the defaults values that do not depend on relatioships with other objects
 	}
 	int myclass::init(OBJECT *parent)
 	{
 		// find the data in the parent object
 		myclass *pMyClass = OBJECTDATA(parent,myclass); // just an example
 
 		// TODO: initialize the default values that depend on relationships with other objects
 		pDouble = &(pMyClass->double); // just an example
 
 		// return 1 on success, 0 on failure
 		return 1;
 	}
 	TIMESTAMP myclass::sync(TIMESTAMP t0, TIMESTAMP t1)
 	{
 		// TODO: update the state of the object
 		myDouble = myDouble*1.01; // just an example
 
 		// TODO: compute time to next state change
 		return (TIMESTAMP)(t1 + myDouble/TS_SECOND); // just an example
 	}
 
 	//////////////////////////////////////////////////////////////////////////
 	// IMPLEMENTATION OF CORE LINKAGE
 	//////////////////////////////////////////////////////////////////////////
 
 	/// Create a myclass object
 	EXPORT int create_myclass(OBJECT **obj, ///< a pointer to the OBJECT*
 							  OBJECT *parent) ///< a pointer to the parent OBJECT
 	{
 		*obj = gl_create_object(myclass::oclass,sizeof(myclass));
 		if (*obj!=NULL)
 		{
 			myclass *my = OBJECTDATA(*obj,myclass);
 			gl_set_parent(*obj,parent);
 			my->create();
 			return 1;
 		}
 		return 0;
 	}
 
 	/// Initialize the object
 	EXPORT int init_myclass(OBJECT *obj, ///< a pointer to the OBJECT
 							OBJECT *parent) ///< a pointer to the parent OBJECT
 	{
 		if (obj!=NULL)
 		{
 			myclass *my = OBJECTDATA(obj,myclass);
 			return my->init(parent);
 		}
 		return 0;
 	}
 
 	/// Synchronize the object
 	EXPORT TIMESTAMP sync_myclass(OBJECT *obj, ///< a pointer to the OBJECT
 				  TIMESTAMP t1) ///< the time to which the OBJECT's clock should advance
 	{
 		TIMESTAMP t2 = OBJECTDATA(obj,myclass)->sync(obj->clock,t1);
 		obj->clock = t1;
 		return t2;
 	}

There are a number of useful extended capabilities that can be added.  These include
* <tt>int notify_myclass(OBJECT *obj, NOTIFYMODULE msg)</tt>
can be implemented to receive notification messages before and after a variable is changed by
the core ('''NM_PREUPDATE''' / '''NM_POSTUPDATE''') and when the core needs the module to reset ('''NM_RESET''')

* <tt>int isa_myclass(OBJECT *obj, char *classname)</tt>
can be implemented to allow the core to discover whether an object is a subtype of
another class.

* <tt>int plc_myclass(OBJECT *obj, TIMESTAMP t1)</tt>
can be implemented to create default [[PLC]] code that can be overridden by attaching a
child plc object.

* <tt>EXPORT int recalc_myclass(OBJECT *obj)</tt>
can be implemented to create a recalculation triggered based on changes to properties made
through ''object_set_value_by_addr''() and related functions.  A property can be made to
trigger recalculation calls by adding '''PT_FLAGS''', '''PF_RECALC''' after the property publish specification, e.g.,

 gl_publish_variable(oclass, PT_double, "resistance[Ohm]", PADDR(resistance), PT_FLAGS, PF_RECALC, NULL);

will cause ''recalc''() to be called after resistance is changed.  The recalc calls occur right before the PLC code
is run during ''sync''() events.

; Linux/Unix Note : A Linux GridLAB module must be a shared object library and must be listed in the appropriate <tt>makefile.am</tt> files.

=== Module implementation as of {{VERSION30}} ===

The above implementation protocol is simplified using the C++ [[Module API]].  

<tt>main.cpp</tt>:

 #define DLMAIN
 #include <stdlib.h>
 #include <stdio.h>
 #include <errno.h>
 #include "gridlabd.h"
 #include "''myclass''.h"
 EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
 {
 	// set the GridLAB core API callback table
 	callback = fntable;
  
 	// TODO: register each object class by creating its first instance
 	new ''myclass''(module);
 
 	// always return the first class registered
 	return ''myclass''::oclass;
 }
 CDECL int do_kill()
 {
 	// if anything needs to be deleted or freed, this is a good time to do it
 	return 0;
 }

<tt>''myclass''.h</tt>:

 #ifndef _''MYCLASS''_H
 #define _''MYCLASS''_H
 #include "gridlabd.h"
 class ''myclass'' : public [[gld_object]] {
 public: // declare typedefs
   // declare properties using [[GL_ATOMIC]], [[GL_STRING]], [[GL_ARRAY]], [[GL_STRUCT]]
 private: // declare private members
 public:
   ''myclass''(MODULE *mod);
   // add declarations for create, init, presync, sync, postsync, precommit, commit, finalize, prenotify, postnotify, etc.
 public:
   static CLASS *oclass;
   static myclass *defaults;
   // add pclass only if derived from another registered class
   static CLASS *pclass;
 };
 #endif // _''MYCLASS''_H

<tt>''myclass''.cpp</tt> when not derived from a registered class

 #include "''myclass''.h"
 EXPORT_CREATE(''myclass'')
 // add EXPORT_* as needed (note that sync and notify are grouped)
 ''myclass''::''myclass''(MODULE *mod)
 {
   if ( !oclass ) { // first time
     oclass = [[gld_class]]::[[gld_class#create|create]](mod,"''myclass''",sizeof(''myclass''),''passconfig'');
     if ( !oclass ) [[gld_object#exception|exception]]("unable to register class ''myclass''");
     oclass->trl = [[TRL_UNKNOWN]];
     defaults = this;
     if ( gl_public_variable(oclass,
          // add ''type'',"''name''", get_''name''_offset(), PT_DESCRIPTION,"''description''",
          NULL)<1) { [[gld_object#exception|exception]]("unable to publish ''myclass'' properties"); }
     memset(this,0,sizeof(''myclass''));
     // add other default values as needed
   }
 }
 // add implementations for create, init, presync, sync, postsync, precommit, commit, finalize, prenotify, postnotify, etc.

<tt>''myclass''.cpp</tt> when derived from a registered class (with difference in bold)

 #include "''myclass''.h"
 EXPORT_CREATE(''myclass'')
 // add EXPORT_* as needed (note that sync and notify are grouped)
 ''myclass''::''myclass''(MODULE *mod) ''': ''parentclass''(mod)'''
 {
   if ( !oclass ) { // first time
     '''pclass = ''parentclass''::oclass;'''
     oclass = [[gld_class]]::[[gld_class#create|create]](mod,"''myclass''",sizeof(''myclass''),''passconfig'');
     if ( !oclass ) [[gld_object#exception|exception]]("unable to register class ''myclass''");
     oclass->trl = [[TRL_UNKNOWN]];
     defaults = this;
     if ( gl_public_variable(oclass,
          '''PT_INHERIT, "''parentclass''",'''
          // add ''type'',"''name''", get_''name''_offset(), PT_DESCRIPTION,"''description''",
          NULL)<1) { [[gld_object#exception|exception]]("unable to publish ''myclass'' properties"); }
     memset(this,0,sizeof(''myclass''));
     // add other default values as needed
   }
 }
 // add implementations for create, init, presync, sync, postsync, precommit, commit, finalize, prenotify, postnotify, etc.

= Upgrading modules to {{VERSION30}} =

As of {{VERSION30}} modules must support multithreaded communication between classes.  To upgrade a module created prior to {{VERSION30}} full R/W locking of class members must be supported.  The following must be done to enable use of locked accessors.

== Minimum upgrade ==

The following steps are the minimum required to upgrade a class created prior to {{VERSION30}} to full compatibility with {{VERSION30}} and later:

# Change <tt>main.cpp</tt> to contain only the following
 #define DLMAIN
 #include "gridlabd.h"
# Derive all registered base classes from [[gld_object]].
# Add PC_AUTOLOCK to the class registration pass configuration for any class that requires locking.
# Remove all self-locking calls.
# Add read or write lock/unlock calls around all accesses to other objects.
# Move first sync initialization code when t0=0 to init() and return deferred status when necessary (see [[Tech:Initialization|Initialization Technical Manual]]).

== Full upgrade ==

; Update the <tt>main.cpp</tt> to contain only the following

 #define DLMAIN
 #include "gridlabd.h"

; Derive existing base classes from [[gld_object]]

 [[class]] ''myclass'' : public [[gld_object]] {
   // declarations ...
 }

; Change declarations of published variables to define get/set accessors

 [[class]] ''myclass'' : protected [[gld_object]] {
   // declare members that can updated atomically (e.g., intN, double, enum, set)
   [[GL_ATOMIC]](''type'',''name'');
   // declare string members (e.g., charN)
   [[GL_STRING]](''type'',''name'');
   // declare struct members (e.g., struct, class)
   [[GL_STRUCT]](''type'',''name'');
   // declare array members (e.g., anything[N])
   [[GL_ARRAY]](''type'',''name'',''size'');
 }

; Replace all internal references to members with get/set accessors

: Lookups (offsets)

 PADDR(''value'')

become

 get_''value''_offset()

and <tt>defaults=this;</tt> must be moved to before first use of <tt>get_*_offset()</tt>.

: Right-hand-side uses of ''value''

 ''atomic_value'' = get_''atomic_var''();
 ''struct_value'' = get_''struct_var''();
 ''char_ptr'' = get_''string_var''();
 ''char_value'' = get_''string_var''(''index'');
 ''array_ptr'' = get_''array_var''();
 ''array_value'' = get_''array_var''(''index'');

: Left-hand-side uses of ''value''

 set_''atomic_var''(''atomic_value'');
 set_''struct_var''(''struct_value'');
 set_''string_var''(''char_ptr'');
 set_''string_var''(''index'',''char_value'');
 set_''array_var''(''array_ptr'');
 set_''array_var''(''index'',''array_value'');

; Replace needed core linkage code with standard export macros

 EXPORT_CREATE(my_class);
 EXPORT_INIT(my_class);
 EXPORT_COMMIT(my_class);
 EXPORT_SYNC(my_class);
 EXPORT_NOTIFY(my_class);
 EXPORT_PRECOMMIT(my_class);
 EXPORT_FINALIZE(my_class);
 EXPORT_ISA(my_class);
 EXPORT_PLC(my_class);
 EXPORT_RECALC(my_class);

: Depending on how the original export function was implemented, you may have to move 
some code into the class member.

; Move old-style initialization in sync() function to init().  The clock will no longer
be 0 on the first sync call.  It will be set to the start time prior to init().

; Surround all internal references requiring coherent access with locked blocks

 [[rlock]]();
 ''copy1'' = ''var1'';
 ''copy2'' = ''var2'';
 [[runlock]]();

 [[wlock]]();
 ''var1'' = ''value1'';
 ''var2'' = ''value2'';
 [[wunlock]]();

: For programming convenience a scope lock is available

 { [[gld_wlock]] _lock(my); // write locked when block is entered
   // write safe code
   value1 += 12.3;
   value2 -= 12.3;
 } // unlock when scope is exited

: Ideally, enable automatic locking for all sync operations by added the ''PC_AUTOLOCK'' to the class registration call:

 oclass = gl_register_class(module,"''myclass''",sizeof(''myclass''),passconfig|PC_AUTOLOCK);

Be careful not to use locking accessors within scope locks or autolocked sync functions.  Doing so will cause a deadlock.

; Modify all external references to use set/get accessors

 [[gld_property]] prop(obj,"varname");
 if ( prop.[[gld_property#is_valid|is_valid]]() )
 {
    double value;
    prop.[[gld_property#getp|getp]](value); // read lock is automatic for non-atomic operations
    value = 12.3;
    prop.[[gld_property#setp|setp]](value); // write lock is automatic for non-atomic operations
 }

: If code requires a locked block use a scope lock w/non-locking accessors

 [[gld_property]] prop(obj,"varname");
 if ( prop.[[gld_property#is_valid|is_valid]]() )
 {
    double value;
    [[gld_wlock]] lock(obj); // write lock is taken
    prop.[[gld_property#getp|getp]](value,lock); // write lock continues so read is ok
    value += 12.3;
    prop.[[gld_property#setp|setp]](value,lock); // write lock continues so write is ok
 }  // unlocked

= Debugging lock timeouts =

A maximum spin timeout is implemented in both read and write locks to prevent deadlocks. If you run into a situation where you get a "write lock timeout" or "read lock timeout" then most likely you've encountered a condition where an object is trying to take a lock out on itself.  Consider the following

; Are you using a locking accessor in a sync call on an autolocked object? : If so, consider using direct access call instead because the object is already locked by the core.

; Is the object locked for a very long time because it's doing something that really does take a long time? : If so, you may have to increase the value of '''MAXSPIN''' in the <tt>core/lock.cpp</tt>. Note that this value has a maximum of about 4e9 and it is currently set to 1e9 (which is roughly 10 seconds), so there's not much room left for growth.  If this is a problem, please file a ticket and an alternate timeout method will have to be implemented.

; Try debugging with a breakpoint on the <tt>throw_exception</tt> calls in <tt>core/lock.cpp</tt> : This should tell you exactly which lock is causing the timeout.

== See also ==
{{:Xref:Developers}}
