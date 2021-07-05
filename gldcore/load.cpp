// load.cpp
// Copyright (C) 2008 Battelle Memorial Institute
// Copyright (C) 2020 Regents of the Leland Stanford Junior University

#include "gldcore.h"

#include <dlfcn.h>

/* TODO: remove these when reentrant code is completed */
DEPRECATED extern GldMain *my_instance;
DEPRECATED STATUS loadall(const char *filename)
{
	return my_instance->get_loader()->load(filename) ? SUCCESS : FAILED;
}
DEPRECATED OBJECT *load_get_current_object(void)
{
	return my_instance->get_loader()->get_current_object();
}
DEPRECATED MODULE *load_get_current_module(void)
{
	return my_instance->get_loader()->get_current_module();
}

SET_MYCONTEXT(DMC_LOAD)

// flags that identify functions are included in runtime classes
#define FN_CREATE		0x0001
#define FN_INIT			0x0002
#define FN_NOTIFY		0x0004
#define FN_RECALC		0x0008
#define FN_PRESYNC		0x0010
#define FN_SYNC			0x0020
#define FN_POSTSYNC		0x0040
#define FN_PLC			0x0080
#define FN_ISA			0x0100
#define FN_COMMIT		0x0200
#define FN_PRECOMMIT	0x0400
#define FN_FINALIZE		0x0800
#define FN_EXPORT		0x1000

struct GldLoader::s_threadlist *GldLoader::threadlist = NULL;

//
// Public methods
//

GldLoader::GldLoader(GldMain *main)
: instance(*main)
{
	include_fail = 0;
	modtime = 0;
	memset(filename,0,sizeof(filename));
	linenum = 1;
	code_used = 0;
	include_list = NULL;
	header_list = NULL;
	outlinenum = 0;
	outfilename = NULL;
	// object_index = NULL;
	// object_linked = NULL;
	// object_index_size = 65536;
	first_unresolved = NULL;
	current_object = NULL;
	current_module = NULL;
	loaderhooks = NULL;
	suppress = 0;
	nesting = 0;
	memset(macro_line,0,sizeof(macro_line));
	forloopstate = FOR_NONE;
	forloop = NULL;
	lastfor = NULL;
	forvar = NULL;
	forvalue = NULL;
	forloop_verbose = false;
	static LANGUAGE builtin_languages[] =
	{
		{"python",python_loader_init,python_parser,NULL},
	};
	language_list = builtin_languages;
	language = NULL;
	last_term = NULL;
	last_term_buffer = NULL;
	last_term_buffer_size = 1024;
}

GldLoader::~GldLoader(void)
{
	if ( last_term_buffer ) free(last_term_buffer);
	// TODO: cleanup other allocated items
}

bool GldLoader::load(const char *filename)
{
	return loadall(filename) == SUCCESS;
}

GldObject GldLoader::get_current_object(void)
{
	return GldObject(current_object);
}

GldModule GldLoader::get_current_module(void)
{
	return GldModule(current_module);
}

//
// Private methods
//

void GldLoader::syntax_error(const char *filename, const int linenum, const char *format, ...)
{
	va_list ptr;
	va_start(ptr,format);
	char msg[1024];
	vsnprintf(msg,sizeof(msg),format,ptr);
	GldException *error = new GldException("%s(%d): %s",filename,linenum,msg);
	va_end(ptr);
	error->throw_now();
}

std::string GldLoader::format_object(OBJECT *obj)
{
	if ( obj->name == NULL )
	{
		return std::string(obj->oclass->name) + ":" + std::to_string(obj->id);
	}
	else
	{
		return std::string(obj->name) + " (" + obj->oclass->name + ":" + std::to_string(obj->id) + ")";
	}
}

char *GldLoader::strip_right_white(char *b)
{
	size_t len, i;
	len = strlen(b) - 1;
	for ( i = len; i >= 0; --i )
	{
		if ( b[i] == '\r' || b[i] == '\n' || b[i] == ' ' || b[i] == '\t' )
		{
			b[i] = '\0';
		}
		else
		{
			break;
		}
	}
	return b;
}

std::string GldLoader::forward_slashes(const char *a)
{
	char buffer[MAXPATHNAMELEN];
	char *b=buffer;
	while ( *a != '\0' && b < buffer+sizeof(buffer)-1 )
	{
		if ( *a == '\\' )
		{
			*b = '/';
		}
		else
		{
			*b = *a;
		}
		a++;
		b++;
	}
	*b='\0';
	return std::string(buffer);
}

void GldLoader::filename_parts(const char *fullname, char *path, char *name, char *ext)
{
	/* fix delimiters (result is a static copy) */
	char file[MAXPATHNAMELEN];
	strcpy(file,forward_slashes(fullname).c_str());

	/* find the last delimiter */
	char *s = strrchr(file,'/');

	/* find the last dot */
	char *e = strrchr(file,'.');

	/* clear results */
	path[0] = name[0] = ext[0] = '\0';

	/* if both found but dot is before delimiter */
	if ( e != NULL && s != NULL && e < s )
	{

		/* there is no extension */
		e = NULL;
	}

	/* copy extension (if any) and terminate filename at dot */
	if ( e != NULL )
	{
		strcpy(ext,e+1);
		*e = '\0';
	}

	/* if path is given */
	if ( s != NULL )
	{
		/* copy name and terminate path */
		strcpy(name,s+1);
		*s = '\0';

		/* copy path */
		strcpy(path,file);
	}

	/* otherwise copy everything */
	else
		strcpy(name,file);
}

int GldLoader::append_init(const char* format,...)
{
	char *code = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&code,format,ptr);
	va_end(ptr);
	if ( code == NULL )
	{
		output_fatal("insufficient memory to compile init code");
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			initialization code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	init_block.append(code);
	free(code);
	return ++code_used;
}

int GldLoader::append_code(const char* format,...)
{
	char *code = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&code,format,ptr);
	va_end(ptr);
	if ( code == NULL )
	{
		output_fatal("insufficient memory to compile runtime code");
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			runtime code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	code_block.append(code);
	free(code);
	return ++code_used;
}

int GldLoader::append_global(const char* format,...)
{
	char *code = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&code,format,ptr);
	va_end(ptr);
	if ( code == NULL )
	{
		output_fatal("insufficient memory to compile global code");
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			global code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	global_block.append(code);
	free(code);
	return ++code_used;
}

void GldLoader::mark_linex(const char *filename, int linenum)
{
	char buffer[64];
	if (global_getvar("noglmrefs",buffer, 63)==NULL)
	{
		char fname[MAXPATHNAMELEN];
		strcpy(fname,filename);
		append_code("#line %d \"%s\"\n", linenum, forward_slashes(fname).c_str());
	}
}

void GldLoader::mark_line(void)
{
	mark_linex(filename,linenum);
}

STATUS GldLoader::exec(const char *format,...)
{
	char cmd[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(cmd,format,ptr);
	va_end(ptr);
	IN_MYCONTEXT output_debug("Running '%s' in '%s'", cmd, getcwd(NULL,0));
	int rc = my_instance->subcommand(cmd);
	if ( rc != 0 )
	{
		output_error("command [%s] failed, rc=%d",cmd,rc);
	}
	return rc==0?SUCCESS:FAILED;
}

STATUS GldLoader::debugger(const char *target)
{
	int result;
	IN_MYCONTEXT output_debug("Starting debugger");
	IN_MYCONTEXT output_debug("Use 'dll-symbols %s' to load symbols",target);
	result = exec("gdb --quiet %s --pid=%d &",target,global_process_id)>=0?SUCCESS:FAILED;
	return result == 0 ? SUCCESS : FAILED;
}

std::string GldLoader::setup_class(CLASS *oclass)
{
	std::string result;
	char buffer[65536] = "";

	snprintf(buffer,sizeof(buffer),"\tOBJECT obj; obj.oclass = oclass; %s *t = (%s*)((&obj)+1);\n",oclass->name,oclass->name);
	result.append(buffer);

	snprintf(buffer,sizeof(buffer),"\toclass->size = sizeof(%s);\n", oclass->name);
	result.append(buffer);

	if ( oclass->parent != NULL )
	{
		snprintf(buffer,sizeof(buffer),"\toclass->parent = (*(callback->class_getname(\"%s\")));",oclass->parent->name);
		result.append(buffer);
	}

	PROPERTY *prop;
	for (prop=oclass->pmap; prop!=NULL; prop=prop->next)
	{
		snprintf(buffer,sizeof(buffer),"\t(*(callback->properties.get_property))(&obj,\"%s\",NULL)->addr = (PROPERTYADDR)((char*)&(t->%s) - (char*)t);\n",prop->name,prop->name);
		result.append(buffer);
	}

	snprintf(buffer,sizeof(buffer),"\t/* begin init block */\n%s\n\t/* end init block */\n",init_block.c_str());
	result.append(buffer);

	return result;
}

int GldLoader::write_file(FILE *fp, const char *data, ...)
{
	char buffer[65536];
	char var_buf[64];
	char *c, *d=buffer;
	int len=0;
	int diff = 0;
	char *b;
	va_list ptr;
	va_start(ptr,data);
	vsnprintf(buffer,sizeof(buffer)-1,data,ptr);
	va_end(ptr);
	while ( (c=strstr(d,"/*RESETLINE*/\n")) != NULL )
	{
		for ( b = d ; b < c; b++ )
		{
			if ( *b == '\n' )
			{
				outlinenum++;
			}
			fputc(*b,fp);
			diff++;
			len++;
		}
		d =  c + strlen("/*RESETLINE*/\n");
		if ( global_getvar("noglmrefs",var_buf,63) == NULL )
		{
			len += fprintf(fp,"#line %d \"%s\"\n", ++outlinenum+1,forward_slashes(outfilename).c_str());
		}
	}
	for ( b = d ; *b != '\0' ; b++ )
	{
		if ( *b == '\n' )
		{
			outlinenum++;
		}
		fputc(*b,fp);
		len++;
	}
	return len;
}

int GldLoader::mkdirs(const char *path)
{
	int rc;
	if ( path == NULL )
	{
		errno = EINVAL;
		return -1;
	}
	if ( (rc=access(path, F_OK)) && errno == ENOENT )
	{
		// path doesn't exist
		char *pos, *end, *tmp;
		IN_MYCONTEXT output_verbose("creating directory '%s'", path);
		if ( ! (tmp=(char *)malloc(strlen(path) + 1)) )
		{
			errno = ENOMEM;
			output_fatal("mkdirs() failed: '%s'", strerror(errno));
			return -1;
		}
		strcpy(tmp, path);
		end = tmp + strlen(tmp);
		// strip off directories until one is found that exists
		while ( (pos=strrchr(tmp, '/')) )
		{
			*pos = '\0';
			if ( ! (*tmp) || ! (rc=access(tmp, F_OK)) )
			{
				break;
			}
			if ( errno != ENOENT )
			{
				output_error("cannot access directory '%s': %s", tmp, strerror(errno));
				free(tmp);
				tmp = NULL;
				return -1;
			}
		}
		// add back components creating them as we go
		for ( pos = tmp+strlen(tmp) ; pos < end ; pos = tmp+strlen(tmp) )
		{
			if ( *pos == '\0' )
			{
				*pos = '/';
				if ( (rc=mkdir(tmp, 0775)) && errno != EEXIST )
				{
					output_error("cannot create directory '%s': %s", tmp, strerror(errno));
					free(tmp);
					tmp = NULL;
					return -1;
				}
			}
		}
		free(tmp);
		tmp = NULL;
		return 0;
	}
	else if (rc)
	{
		output_error("cannot access directory '%s': %s", path, strerror(errno));
	}
	return rc;
}

STATUS GldLoader::compile_code(CLASS *oclass, int64 functions)
{
	char include_file_str[1024];
	char buffer[256];
	bool use_msvc = (global_getvar("use_msvc",buffer,255)!=NULL);

	include_file_str[0] = '\0';

	/* check global_include */
	if ( strlen(global_include) == 0 )
	{
		if ( getenv("GRIDLABD") )
		{
			strncpy(global_include,getenv("GRIDLABD"),sizeof(global_include)-1);
			IN_MYCONTEXT output_verbose("global_include is not set, assuming value of GRIDLABD variable '%s'", global_include);
		}
		else
		{
			output_error("'include' variable is not set and neither is GRIDLABD environment, compiler cannot proceed without a way to find gridlabd.h");
			/* TROUBLESHOOT
				The runtime class compiler needs to find the file gridlabd.h and uses either the <i>include</i> global variable or the <b>gridlabd</b>
				environment variable to find it.  Check the definition of the <b>gridlabd</b> environment variable or use the
				<code>#define include=<i>path</i></code> to specify the path to the <code>gridlabd.h</code>.
			 */
			return FAILED;
		}
	}

	if ( code_used > 0 )
	{
		MODULE *mod;

		FILE *fp;
		struct stat stat;
		int outdated = true;
		char cfile[4097];
		char ofile[4097];
		char afile[4097];
		char file[4097];
		char tmp[1025];
		char tbuf[1025];
		size_t ifs_off = 0;
		INCLUDELIST *lptr = 0;

		/* build class implementation files */
		strncpy(tmp, global_tmp, sizeof(tmp)-1);
		if ( mkdirs(tmp) )
		{
			errno = 0;
			return FAILED;
		}
		if ( strlen(tmp) > 0 && tmp[strlen(tmp)-1] != '/' )
		{
			strcat(tmp,"/");
		}
		snprintf(cfile,sizeof(cfile)-1,"%s%s.cpp", (use_msvc||global_gdb||global_gdb_window)?"":tmp,oclass->name);
		snprintf(ofile,sizeof(ofile)-1,"%s%s.%s", (use_msvc||global_gdb||global_gdb_window)?"":tmp,oclass->name, use_msvc?"obj":"o");
		snprintf(file,sizeof(file)-1,"%s%s", (use_msvc||global_gdb||global_gdb_window)?"":tmp, oclass->name);
		snprintf(afile,sizeof(afile)-1,"%s" DLEXT , oclass->name);

		/* peek at library file */
		fp = fopen(afile,"r");
		if ( fp != NULL && fstat(fileno(fp),&stat) == 0 )
		{
			if ( global_debug_mode || use_msvc || global_gdb || global_gdb_window )
			{
				IN_MYCONTEXT output_verbose("%s is being used for debugging", afile);
			}
			else if ( global_force_compile )
			{
				IN_MYCONTEXT output_verbose("%s recompile is forced", afile);
			}
			else if ( modtime < stat.st_mtime )
			{
				IN_MYCONTEXT output_verbose("%s is up to date", afile);
				outdated = false;
			}
			else
			{
				IN_MYCONTEXT output_verbose("%s is outdated", afile);
			}
		}
		if ( fp != NULL )
		{
			fclose(fp);
		}

		if ( outdated )
		{
			/* write source file */
			fp = fopen(cfile,"w");

			IN_MYCONTEXT output_verbose("writing inline code to '%s'", cfile);
			if ( fp == NULL )
			{
				output_fatal("unable to open '%s' for writing", cfile);
				/*	TROUBLESHOOT
					The internal compiler cannot write a temporary C or C++ that
					it needs to build your model.  The message indicates where
					the file is.  To remedy the problem you must make sure that
					the system allows full access to the file.
				 */
				return FAILED;
			}
			outfilename = cfile;
			ifs_off = 0;
			for ( lptr = header_list ; lptr != 0; lptr = lptr->next )
			{
				snprintf(include_file_str+ifs_off, sizeof(include_file_str)-ifs_off-1, "#include \"%s\"\n;", lptr->file);
				ifs_off+=strlen(lptr->file)+13;
			}
			if ( write_file(fp,"/* automatically generated from %s */\n\n"
					"int gld_major=%d, gld_minor=%d;\n\n"
					"%s\n\n"
					"#include <gridlabd.h>\n\n"
					"%s"
					"CALLBACKS *callback = NULL;\n"
					"static CLASS *myclass = NULL;\n"
					"static int setup_class(CLASS *);\n\n",
					PACKAGE_NAME,
					REV_MAJOR, REV_MINOR,
					include_file_str,
					global_getvar("use_msvc",tbuf,63)!=NULL
					?
						"int __declspec(dllexport) dllinit() { return 0;};\n"
						"int __declspec(dllexport) dllkill() { return 0;};\n"
					:
						"extern \"C\" int dllinit() __attribute__((constructor));\n"
						"extern \"C\" int dllinit() { return 0;}\n"
						"extern \"C\"  int dllkill() __attribute__((destructor));\n"
						"extern \"C\" int dllkill() { return 0;};\n\n"
					)<0
				|| write_file(fp,"extern \"C\" CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])\n"
					"{\n"
					"\tcallback=fntable;\n"
					"\tmyclass=(CLASS*)((*(callback->class_getname))(\"%s\",NULL));\n"
					"\tif (!myclass) return NULL;\n"
					"\tif (!setup_class(myclass)) return NULL;\n"
					"\treturn myclass;"
					"}\n",oclass->name)<0
				|| write_file(fp,"%s",code_block.c_str())<0
				|| write_file(fp,"%s",global_block.c_str())<0
				|| write_file(fp,"static int setup_class(CLASS *oclass)\n"
					"{\t\n%s\treturn 1;\n}\n",setup_class(oclass).c_str())<0
				)
			{
				output_fatal("unable to write to '%s'", cfile);
				/*	TROUBLESHOOT
					The internal compiler cannot write a temporary C or C++ that
					it needs to build your model.  The message indicates where
					the file is.  To remedy the problem you must make sure that
					the system allows full access to the file.
				 */
				return FAILED;
			}
			fclose(fp);
			outfilename = NULL;

			/* compile object file */
			IN_MYCONTEXT output_verbose("compiling inline code from '%s'", cfile);
#define DEFAULT_CXX "g++"
#define DEFAULT_CXXFLAGS ""
#define DEFAULT_LDFLAGS ""
			char execstr[65537];
			char ldstr[65537];
			char mopt[8]="";
			const char *libs = "-lstdc++";

			snprintf(execstr,sizeof(execstr)-1, "%s %s %s %s %s -fPIC -c \"%s\" -o \"%s\"",
					getenv("CXX")?getenv("CXX"):DEFAULT_CXX,
					global_warn_mode?"-w":"",
					global_debug_output?"-g -O0":"-O0",
					mopt,
					getenv("CXXFLAGS")?getenv("CXXFLAGS"):DEFAULT_CXXFLAGS,
					cfile, ofile);
			IN_MYCONTEXT output_verbose("compile command: [%s]", execstr);
			if ( exec("%s",execstr) == FAILED )
			{
				errno = EINVAL;
				return FAILED;
			}
			if ( !global_debug_output )
			{
				unlink(cfile);
			}
			else
			{
				output_verbose("keeping %s for debugging",cfile);
			}

			/* link new runtime module */
			IN_MYCONTEXT output_verbose("linking inline code from '%s'", ofile);
			snprintf(ldstr,sizeof(ldstr)-1, "%s %s %s %s -shared -Wl,\"%s\" -o \"%s\" %s",
					getenv("CXX")?getenv("CXX"):DEFAULT_CXX,
					mopt,
					global_debug_output?"-g -O0":"",
					getenv("LDFLAGS")?getenv("LDFLAGS"):DEFAULT_LDFLAGS,
					ofile, afile, libs);
			IN_MYCONTEXT output_verbose("link command: [%s]", ldstr);
			if ( exec("%s",ldstr) == FAILED )
			{
				errno = EINVAL;
				return FAILED;
			}
			/* post linking command (optional) */
			if ( global_getvar("LDPOSTLINK",tbuf,sizeof(tbuf))!=NULL )
			{
				/* SE linux needs the new module marked as relocatable (textrel_shlib_t) */
				exec("%s '%s'", tbuf, afile);
			}

			if ( !global_debug_output )
			{
				unlink(ofile);
			}
		}

		/* load runtime module */
		IN_MYCONTEXT output_verbose("loading dynamic link library %s...", afile);
		mod = module_load(oclass->name,0,NULL);
		if ( mod == NULL )
		{
			output_error("unable to load inline code");
			return FAILED;
		}
		oclass->module = mod;

		/* start debugger if requested */
		if ( global_gdb || global_gdb_window )
		{
			if ( global_debug_mode )
			{
				IN_MYCONTEXT output_debug("using gdb requires GLD debugger be disabled");
			}
			global_debug_output = 1;
			IN_MYCONTEXT output_verbose("attaching debugger to process id %d", getpid());
			if ( debugger(afile) == FAILED )
			{
				output_error("debugger load failed: %s", errno?strerror(errno):"(no details)");
				return FAILED;
			}
		}

		/* provide info so external debugger can be attached */
		else
		{
			IN_MYCONTEXT output_debug("class %s running in process %d", oclass->name, getpid());
		}

		oclass->has_runtime = true;
		strcpy(oclass->runtime,afile);
	}
	else
	{
		oclass->has_runtime = false;
	}

	/* clear buffers */
	code_block.clear();
	global_block.clear();
	init_block.clear();
	code_used = 0;

	return SUCCESS;
}

STATUS GldLoader::load_set_index(OBJECT *obj, OBJECTNUM id)
{
	INDEXITEM item = {obj,0};
	indexmap[id] = item;
	return SUCCESS;
}

OBJECT *GldLoader::load_get_index(OBJECTNUM id)
{
	INDEXITEM &item = indexmap[id];
	item.count++;
	return item.obj;
}

OBJECT *GldLoader::get_next_unlinked(CLASS *oclass)
{
	for ( INDEXMAP::iterator iter = indexmap.begin() ; iter != indexmap.end() ; iter++ )
	{
		INDEXITEM &item = iter->second;
		if ( item.count == 0 && item.obj != NULL && item.obj->oclass == oclass )
		{
			item.count++;
			return item.obj;
		}
	}
	return NULL;
}

void GldLoader::free_index(void)
{
	indexmap.clear();
}

GldLoader::UNRESOLVED *GldLoader::add_unresolved(OBJECT *by, PROPERTYTYPE ptype, void *ref, CLASS *oclass, char *id, char *file, unsigned int line, int flags)
{
	UNRESOLVED *item = (UNRESOLVED*)malloc(sizeof(UNRESOLVED));
	if ( item == NULL )
	{
		errno = ENOMEM;
		return NULL;
	}
	item->by = by;
	item->ptype = ptype;
	item->ref = ref;
	item->oclass = oclass;
	item->id = strdup(id);
	if ( first_unresolved != NULL && strcmp(first_unresolved->file,file) == 0 )
	{
		item->file = first_unresolved->file; // means keep using the same file
		first_unresolved->file = NULL;
	}
	else
	{
		item->file = (char*)malloc(strlen(file)+1);
		item->file = strdup(file);
	}
	item->line = line;
	item->next = first_unresolved;
	item->flags = flags;
	first_unresolved = item;
	return item;
}

int GldLoader::resolve_object(UNRESOLVED *item, const char *filename)
{
	OBJECT *obj;
	char classname[65];
	char propname[65];
	char target[256];
	OBJECTNUM id = 0;
	char op[2];
	char star;

	if ( 0 == strcmp(item->id, "root") )
	{
		obj = NULL;
	}
	else if ( sscanf(item->id,"childless:%[^=]=%s",propname,target) == 2 )
	{
		for ( obj = object_get_first() ; obj != NULL ; obj = object_get_next(obj) )
		{
			char value[1024];
			if ( object_get_child_count(obj)==0 && !object_get_value_by_name(obj,propname,value,sizeof(value)) && strcmp(value,target)==0 )
			{
				object_set_parent(*(OBJECT**)(item->ref),obj);
				break;
			}
		}
		if ( obj == NULL )
		{
			syntax_error(filename,item->line,"no childless objects found in %s=%s (parent unresolved)", propname, target);
			return FAILED;
		}
	}
	else if ( sscanf(item->id,"%64[^.].%64[^:]:",classname,propname) == 2 )
	{
		const char *value = strchr(item->id,':');
		FINDLIST *match;
		if ( value++ == NULL )
		{
			syntax_error(filename,item->line,"%s reference to %s is missing match value",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
		match = find_objects(FL_NEW,FT_CLASS,SAME,classname,AND,FT_PROPERTY,propname,SAME,value,FT_END);
		if ( match == NULL || match->hit_count == 0 )
		{
			syntax_error(filename,item->line,"%s reference to %s does not match any existing objects",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
		else if ( match->hit_count > 1 )
		{
			syntax_error(filename,item->line,"%s reference to %s matches more than one object",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
		obj = find_first(match);
	}
	else if ( sscanf(item->id,"%[^:]:id%[+-]%d",classname,op,&id) == 3 )
	{
		CLASS *oclass = class_get_class_from_classname(classname);
		obj = object_find_by_id(item->by->id + (op[0]=='+'?+1:-1)*id);
		if ( oclass == NULL || obj == NULL )
		{
			obj = object_find_name(item->id);
			if ( obj == NULL )
			{
				syntax_error(filename,item->line,"cannot resolve implicit reference from %s to %s",
					format_object(item->by).c_str(), item->id);
				return FAILED;
			}
		}
	}
	else if ( sscanf(item->id,global_object_scan,classname,&id) == 2 )
	{
		obj = load_get_index(id);
		if ( obj == NULL )
		{
			obj = object_find_name(item->id);
		}
		if ( obj == NULL )
		{
			syntax_error(filename,item->line,"cannot resolve explicit reference from %s to %s",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
		if ( strcmp(obj->oclass->name,classname) !=0 && strcmp("id", classname) != 0 )
		{
			syntax_error(filename,item->line,"class of reference from %s to %s mismatched",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
	}
	else if ( sscanf(item->id,"%[^:]:%c",classname,&star) == 2 && star == '*' )
	{
		CLASS *oclass = class_get_class_from_classname(classname);
		obj = get_next_unlinked(oclass);
		if ( obj == NULL )
		{
			syntax_error(filename,item->line,"cannot resolve last reference from %s to %s",
				format_object(item->by).c_str(), item->id);
			return FAILED;
		}
	}
	else if ( (obj=object_find_name(item->id)) != NULL )
	{
		/* found it already*/
	}
	else
	{
		syntax_error(filename,item->line,"'%s' not found", item->id);
		return FAILED;
	}
	*(OBJECT**)(item->ref) = obj;
	if ( (item->flags&UR_RANKS) == UR_RANKS )
	{
		object_set_rank(obj,item->by->rank);
	}
	return SUCCESS;
}

int GldLoader::resolve_double(UNRESOLVED *item, const char *context)
{
	char oname[65];
	char pname[65];
	const char *filename = (item->file ? item->file : context);

	if (sscanf(item->id,"%64[^.].%64s",oname,pname)==2)
	{
		OBJECT *obj = NULL;
		PROPERTY *prop = NULL;
		double **ref = NULL;
		TRANSFORM *xform = NULL;

		/* get and check the object */
		obj = object_find_name(oname);
		if (obj==NULL)
		{
			syntax_error(filename,item->line,"object '%s' not found", oname);
			return FAILED;
		}

		/* get and check the property */
		prop = object_get_property(obj,pname,NULL);
		if (prop==NULL)
		{
			syntax_error(filename,item->line,"property '%s' not found", pname);
			return FAILED;
		}

		/* get transform reference */
		if ((item->flags&UR_TRANSFORM)==UR_TRANSFORM)
		{
			/* find transform that uses this target */
			for ( TRANSFORM *xform = transform_getnext(NULL) ; xform != NULL ; xform = transform_getnext(xform) )
			{
				/* the reference is to the schedule's source */
				if (xform==item->ref)
				{
					ref = &(xform->source);
					break;
				}
			}
			if ( ref == NULL )
			{
				syntax_error(filename,item->line,"transform reference not found");
				return FAILED;
			}
		}

		/* get the direct reference */
		else
			ref = (double**)(item->ref);

		/* extract the reference to the object property */
		switch (prop->ptype) {
		case PT_double:
			*ref = object_get_double(obj,prop);
			if (xform) xform->source_type = XS_DOUBLE;
			break;
		case PT_complex:
			*ref = &(((complex*)object_get_addr(obj,prop->name))->Re());
			if (xform) xform->source_type = XS_COMPLEX;
			break;
		case PT_loadshape:
			*ref = &(((loadshape*)object_get_addr(obj,prop->name))->load);
			if (xform) xform->source_type = XS_LOADSHAPE;
			break;
		case PT_enduse:
			*ref = &(((enduse*)object_get_addr(obj,prop->name))->total.Re());
			if (xform) xform->source_type = XS_ENDUSE;
			break;
		default:
			syntax_error(filename,item->line,"reference '%s' type is not supported", item->id);
			return FAILED;
		}

		IN_MYCONTEXT output_debug("reference '%s' resolved ok", item->id);

		return SUCCESS;
	}
	return FAILED;
}

STATUS GldLoader::resolve_list(UNRESOLVED *item)
{
	UNRESOLVED *next;
	const char *filename = NULL;
	while (item!=NULL)
	{
		// context file name changes
		if (item->file!=NULL)
		{
			// free last context file name
			if (filename!=NULL){
				free((void*)filename); // last one - not used again
				filename = NULL;
			}

			// get next context file name
			filename = item->file;
		}

		// handle different reference types
		switch (item->ptype) {
		case PT_object:
			if (resolve_object(item, filename)==FAILED)
				return FAILED;
			break;
		case PT_double:
		case PT_complex:
		case PT_loadshape:
		case PT_enduse:
			if (resolve_double(item, filename)==FAILED)
				return FAILED;
			break;
		default:
			syntax_error(filename,item->line,"unresolved reference to property '%s' uses unsupported type (ptype=%d)", item->id, item->ptype);
			break;
		}
		next = item->next;
		free(item);
		item=next;
	}
	if ( filename!=NULL )
		free((void*)filename);
	return SUCCESS;
}

STATUS GldLoader::load_resolve_all(void)
{
	STATUS result = resolve_list(first_unresolved);
	first_unresolved = NULL;
	return result;
}

void GldLoader::clear_last_term(void)
{
	last_term = NULL;
}

void GldLoader::set_last_term(const char *p)
{
	last_term = p;
}

void GldLoader::save_last_term(const char *p)
{
	if ( last_term != NULL )
	{
		size_t len = p-last_term+1;
		if ( len < last_term_buffer_size || last_term_buffer == NULL )
		{
			last_term_buffer = (char*)realloc(last_term_buffer,len);
			last_term_buffer_size = len;
		}
		strncpy(last_term_buffer,last_term,len);
		last_term_buffer[len] = '\0';
	}
}

const char *GldLoader::get_last_term()
{
	return last_term ? last_term_buffer : NULL;
}

void GldLoader::start_parse(int &mm, int &m, int &n, int &l, int linenum)
{
	mm = m = n = l = 0;
	l = linenum;
}

#define START int _mm, _m, _n, _l; start_parse(_mm,_m,_n,_l,linenum);
#define MARK true // (set_last_term(HERE),true)
#define ACCEPT { _n+=_m; _p+=_m; _m=0; }
#define HERE (_p+_m)
#define OR {_m=0;}
#define REJECT { linenum=_l; return 0; }
#define WHITE (TERM(white(HERE)))
#define LITERAL(X) (_mm=literal(HERE,(X)),_m+=_mm,_mm>0)
#define PEEK(C) (_p[_m]==(C))
#define TERM(X) (_mm=(X),_m+=_mm,_mm>0)
#define MARKTERM(X) TERM(X) // (set_last_term(HERE),TERM(X))
#define COPY(X) {size--; (X)[_n++]=*_p++;}
#define SAVETERM // save_last_term(HERE);
#define DONE return _n;
#define BEGIN_REPEAT {const char *__p=_p; int __mm=_mm, __m=_m, __n=_n, __l=_l; int __ln=linenum;
#define REPEAT _p=__p;_m=__m; _mm=__mm; _n=__n; _l=__l; linenum=__ln;
#define END_REPEAT }

void GldLoader::syntax_error_here(const char *p)
{
	char context[16], *nl;
	strncpy(context,p,15);
	nl = strchr(context,'\n');
	if (nl!=NULL) *nl='\0'; else context[15]='\0';
	if (strlen(context)>0)
		syntax_error(filename,linenum,"syntax error at '%s...'", context);
	else
		syntax_error(filename,linenum,"syntax error");
}

int GldLoader::white(PARSER)
{
	int len = 0;
	for(len = 0; *_p != '\0' && isspace((unsigned char)(*_p)); ++_p){
		if (*_p == '\n')
			++linenum;
		++len;
	}
	return len;
}

int GldLoader::pattern(PARSER, const char *pattern, char *result, int size)
{
	char format[64];
	START;
	snprintf(format,sizeof(format)-1,"%%%s",pattern);
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

int GldLoader::literal(PARSER, const char *text)
{
	if (strncmp(_p,text,strlen(text))==0)
		return (int)strlen(text);
	return 0;
}

int GldLoader::quoted_string(PARSER, char *result, int size)
{
	int len = 0;
	while ( isspace(*_p) ) _p++;
	if ( *_p++ != '"' )
	{
		return 0;
	}
	else
	{
		len++;
		while ( _p[0] != '"' )
		{
			if ( len < size-1 )
			{
				if ( _p[0] == '\\' && _p[1] != '\0' )
				{
					switch ( *++_p )
					{
					case 'n':
						*result++ = '\n';
						break;
					case 't':
						*result++ = '\t';
						break;
					default:
						*result++ = *_p++;
						break;
					}
				}
				else
				{
					*result++ = *_p++;
					len++;
				}
			}
			else
			{
				return 0;
			}
		}
		*result++ = '\0';
		len++;
		return len;
	}
}
int GldLoader::dashed_name(PARSER, char *result, int size)
{	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='_' || *_p=='-' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}

int GldLoader::name(PARSER, char *result, int size)
{	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='_' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}
int GldLoader::namelist(PARSER, char *result, int size)
{	/* basic list of names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p==',' || *_p=='@' || *_p==' ' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}
int GldLoader::variable_list(PARSER, char *result, int size)
{	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

int GldLoader::property_list(PARSER, char *result, int size)
{	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_' || *_p==':') COPY(result);
	result[_n]='\0';
	DONE;
}

int GldLoader::unitspec(PARSER, UNIT **unit)
{
	char result[1024];
	size_t size = sizeof(result);
	START;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='$' || *_p=='%' || *_p=='*' || *_p=='/' || *_p=='^') COPY(result);
	result[_n]='\0';
	try {
		if ((*unit=unit_find(result))==NULL){
			linenum=_l;
			_n = 0;
		} else {
			_n = (int)strlen(result);
		}
	}
	catch (const char *msg)
	{
		linenum=_l;
		_n = 0;
	}
	DONE;
}

int GldLoader::unitsuffix(PARSER, UNIT **unit)
{
	START;
	if (LITERAL("["))
	{
		if (!TERM(unitspec(HERE,unit)))
		{
			syntax_error(filename,linenum,"missing valid unit after [");
			REJECT;
		}
		if (!LITERAL("]"))
		{
			syntax_error(filename,linenum,"missing ] after unit '%s'",(*unit)->name);
		}
		ACCEPT;
		DONE;
	}
	REJECT;
	DONE;
}

int GldLoader::nameunit(PARSER,char *result,int size,UNIT **unit)
{
	START;
	if (TERM(name(HERE,result,size)) && TERM(unitsuffix(HERE,unit))) ACCEPT; DONE;
	REJECT;
}

int GldLoader::dotted_name(PARSER, char *result, int size)
{	/* basic name */
	START;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='_' || *_p=='.') COPY(result);
	result[_n]='\0';
	DONE;
}

int GldLoader::hostname(PARSER, char *result, int size)
{	/* full path name */
	START;
	while ( (size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='_' || *_p=='.' || *_p=='-' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}

int GldLoader::delim_value(PARSER, char *result, int size, const char *delims)
{
	/* everything to any of delims */
	int quote=0;
	const char *start=_p;
	START;
	if (*_p=='"')
	{
		quote=1;
		_p++;
		size--;
	}
	while (size>1 && *_p!='\0' && ((quote&&*_p!='"') || strchr(delims,*_p)==NULL) && *_p!='\n')
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++;
		COPY(result);
	}
	result[_n]='\0';
	return (int)(_p - start);
}

int GldLoader::structured_value(PARSER, char *result, int size)
{
	int depth=0;
	const char *start=_p;
	START;
	if (*_p!='{') return 0;
	while (size>1 && *_p!='\0' && !(*_p=='}'&&depth==1) )
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++;
		else if ( *_p=='{' ) depth++;
		else if ( *_p=='}' ) depth--;
		COPY(result);
	}
	COPY(result);
	result[_n]='\0';
	return (int)(_p - start);
}

int GldLoader::multiline_value(PARSER,char *result,int size)
{
	const char *start = _p;
	const char *end = strstr(_p,"\"\"\"");
	if ( end == NULL )
	{
		output_error_raw("%s(%d): unterminated multi-line value ('\"\"\"' not found)",filename,linenum);
		return 0;
	}

	std::string value("");
	for ( ; _p < end ; _p++)
	{
		const char *esc = strchr(_p,'\\');
		if ( esc == NULL )
		{
			value += std::string(_p,end-_p);
			break;
		}
		if ( esc > _p )
		{
			std::string fragment(_p,esc-_p);
			value = value + fragment;
		}
		switch ( esc[1] ) {
		case 'n':
			value += std::string("\n");
			break;
		case 't':
			value += std::string("\t");
			break;
		case 'b':
			value += std::string("\b");
			break;
		case 'f':
			value += std::string("\f");
			break;
		case 'r':
			value += std::string("\r");
			break;
		// TODO: need \uXXXX
		default:
			break;
		}
		_p = esc+1;
	}
	int len = value.length();
	if ( len < size )
	{
		strcpy(result,value.c_str());
		return (int)(end-start);
	}
	else
	{
		syntax_error(filename,linenum,"multi-line value too long for loader buffer (len %d > size %d)",len,size);
		return 0;
	}
}

int GldLoader::value(PARSER, char *result, int size)
{
	/* everything to a semicolon */
	char delim=';';
	const char *start=_p;
	int quote=0;
	START;
	if ( strncmp(_p,"\"\"\"",3) == 0 )
	{
		int len = multiline_value(_p+3,result,size);
		return len > 0 ? (len+6) : 0;
	}
	else if ( *_p == '{' )
	{
		return structured_value(_p,result,size);
	}
	while ( size > 1 && *_p != '\0' && !(*_p==delim && quote == 0) && *_p != '\n' )
	{
		if ( _p[0]=='\\' && _p[1]!='\0' )
		{
			_p++; COPY(result);
		}
		else if (*_p=='"')
		{
			_p++;
			size--;
			quote = (1+quote) % 2;
		}
		else
		{
			COPY(result);
		}
	}
	result[_n]='\0';
	if ( quote&1 )
	{
		output_warning("%s(%d): missing closing double quote", filename, linenum);
	}
	return (int)(_p - start);
}

int GldLoader::integer(PARSER, int64 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi64(result);
	return _n;
}

int GldLoader::unsigned_integer(PARSER, unsigned int64 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=(unsigned int64)atoi64(result);
	return _n;
}

int GldLoader::integer32(PARSER, int32 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

int GldLoader::integer16(PARSER, int16 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

int GldLoader::real_value(PARSER, double *value)
{
	char result[256];
	int ndigits=0;
	int size=sizeof(result);
	START;
	if (*_p=='+' || *_p=='-') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);++ndigits;}
	if (*_p=='.') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);ndigits++;}
	if (ndigits>0 && (*_p=='E' || *_p=='e'))
	{
		COPY(result);
		if (*_p=='+' || *_p=='-') COPY(result);
		while (size>1 && isdigit(*_p)) COPY(result);
	}
	result[_n]='\0';
	*value=atof(result);
	return _n;
}

int GldLoader::functional(PARSER, double *pValue)
{
	char fname[32];
	START;
	if WHITE ACCEPT;
	if (LITERAL("random.") && TERM(name(HERE,fname,sizeof(fname))))
	{
		RANDOMTYPE rtype = random_type(fname);
		int nargs = random_nargs(fname);
		double a;
		if (rtype==RT_INVALID || nargs==0 || (WHITE,!LITERAL("(")))
		{
			syntax_error(filename,linenum,"%s is not a valid random distribution",fname);
			REJECT;
		}
		if (nargs==-1)
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				double b[1024];
				int maxb = sizeof(b)/sizeof(b[0]);
				int n;
				b[0] = a;
				for (n=1; n<maxb && (WHITE,LITERAL(",")); n++)
				{
					if (WHITE,TERM(real_value(HERE,&b[n])))
						continue;
					else
					{
						// variable arg list
						syntax_error(filename,linenum,"expected a %s distribution term after ,", fname);
						REJECT;
					}
				}
				if (WHITE,LITERAL(")"))
				{
					*pValue = random_value(rtype,n,b);
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"missing ) after %s distribution terms", fname);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"expected first term of %s distribution", fname);
				REJECT;
			}
		}
		else
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				// fixed arg list
				double b,c;
				if (nargs==1)
				{
					if (WHITE,LITERAL(")"))
					{
						*pValue = random_value(rtype,a);
						ACCEPT;
					}
					else
					{
						syntax_error(filename,linenum,"expected ) after %s distribution term", fname);
						REJECT;
					}
				}
				else if (nargs==2)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b);
						ACCEPT;
					}
					else
					{
						syntax_error(filename,linenum,"missing second %s distribution term and/or )", fname);
						REJECT;
					}
				}
				else if (nargs==3)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&c))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b,c);
						ACCEPT;
					}
					else
					{
						syntax_error(filename,linenum,"missing terms and/or ) in %s distribution ", fname);
						REJECT;
					}
				}
				else
				{
					syntax_error(filename,linenum,"%d terms is not supported", nargs);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"expected first term of %s distribution", fname);
				REJECT;
			}
		}
	} else if TERM(real_value(HERE,pValue)){
		ACCEPT;
	} else
	{
		/* possibly valid through expression() -MH */
		//output_message("%s(%d): expected property or functional value", filename,linenum);
		REJECT;
	}
	DONE;
}

struct s_rpn
{
	int op;
	double val; // if op = 0, check val
};

static double pos(double a)
{
	return a > 0.0 ? a : 0.0;
}

static double neg(double a)
{
	return a < 0.0 ? -a : 0.0;
}

static double nonzero(double a)
{
	return a != 0.0;
}

static double sign(double a)
{
	return a < 0.0 ? -1.0 : ( a > 0 ? +1.0 : 0.0);
}

struct s_rpn_func
{
	const char *name;
	int args; /* use a mode instead? else assume only doubles */
	int index;
	double (*fptr)(double);
	/* fptr? for now, just to recognize */
} rpn_map[] =
{
	{"sin", 1, -1, sin},
	{"cos", 1, -2, cos},
	{"tan", 1, -3, tan},
	{"abs", 1, -4, fabs},
	{"sqrt", 1, -5, sqrt},
	{"acos", 1, -6, acos},
	{"asin", 1, -7, asin},
	{"atan", 1, -8, atan},
//	{"atan2", 2},	/* only one with two inputs? */
	{"log", 1, -10, log},
	{"log10", 1, -11, log10},
	{"floor", 1, -12, floor},
	{"ceil", 1, -13, ceil},
	{"pos", 1, -14, pos}, // clamp positive
	{"neg", 1, -15, neg}, // clamp negative
	{"nonzero", 1, -16, nonzero}, // returns 1 if nonzero
	{"sign",1,-17,sign},
	{"exp",1,-18,exp},
};

int GldLoader::rpnfunc(PARSER, int *val)
{
	int i = 0, count = 0;
	START;
	count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
	for(i = 0; i < count; ++i){
		if(strncmp(rpn_map[i].name, HERE, strlen(rpn_map[i].name)) == 0){
			*val = rpn_map[i].index;
			return (int)strlen(rpn_map[i].name);
		}
	}
	return 0;
}

#define OP_END 0
#define OP_OPEN 1
#define OP_CLOSE 2
#define OP_POW 3
#define OP_MULT 4
#define OP_MOD 5
#define OP_DIV 6
#define OP_ADD 7
#define OP_SUB 8
#define OP_SIN -1
#define OP_COS -2
#define OP_TAN -3
#define OP_ABS -4

static int op_prec[] = {0, 0, 0, 3, 2, 2, 2, 1, 1};

#define PASS_OP(T) \
	while(op_prec[(T)] <= op_prec[op_stk[op_i]]){	\
		rpn_stk[rpn_i].op = op_stk[op_i];				\
		rpn_stk[rpn_i].val = 0;							\
		++rpn_i;										\
		--op_i;											\
	}													\
	op_stk[++op_i] = (T);							\
	++rpn_sz;

int GldLoader::expression(PARSER, double *pValue, UNIT **unit, OBJECT *obj)
{
	double val_q[128], tVal;
	char tname[128]; /* type name for this.prop */
	char oname[128], pname[128];
	struct s_rpn rpn_stk[256];
	int op_stk[128], val_i = 0, op_i = 1, rpn_i = 0, depth = 0, rfname = 0, rpn_sz = 0;
	int i = 0;

	START;
	/* RPN-ify */
	if LITERAL("("){
		ACCEPT;
		if WHITE ACCEPT;
		depth = 1;
		op_stk[0] = OP_OPEN;
		op_i = 0;
	} else {
		REJECT; /* all expressions must be contained within a () block */
	}
	while(depth > 0){ /* grab tokens*/
		if LITERAL(";"){ /* says we're done */
			ACCEPT;
			break;
		} else if LITERAL("("){ /* parantheses */
			ACCEPT;
			op_stk[++op_i] = OP_OPEN;
			//++op_i;
			++depth;
			if WHITE ACCEPT;
		} else if LITERAL(")"){
			ACCEPT;
			if WHITE ACCEPT;
			--depth;
			/* consume operations until OP_OPEN found */
			while((op_i >= 0) && (op_stk[op_i] != OP_OPEN)){
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* consume OP_OPEN too */
			op_i--;
			/* rpnfunc lookahead */
			if(op_stk[op_i] < 0){ /* push rpnfunc */
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* op_stk[op_i] == OP_CLOSE */
		} else if LITERAL("^"){ /* operators */
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = OP_POW; /* nothing but () and functions hold higher precedence */
			++rpn_sz;
		} else if LITERAL("*"){ /* prec = 4 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MULT);
		} else if LITERAL("/"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_DIV);
		} else if LITERAL("%"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MOD);
		} else if LITERAL("+"){ /* prec = 6 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_ADD);
		} else if LITERAL("-"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_SUB);
		} else if(TERM(rpnfunc(HERE, &rfname))){
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = rfname;
			if LITERAL("("){
				ACCEPT;
				if WHITE ACCEPT;
				op_stk[++op_i] = OP_OPEN;
				++depth;
				++rpn_sz;
			} else {
				REJECT;
			}
		} else if ( TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(name(HERE,tname,sizeof(tname))))
		{
			OBJECT *nobj = object_find_name(oname);
			if ( nobj == NULL )
			{
				syntax_error(filename,linenum,"object not found (object must already exist): %s.%s", oname, tname);
				REJECT;
			}
			double *valptr = object_get_double_by_name(nobj, tname);
			if ( strcmp(tname,"latitude")==0 )
			{
				valptr = &(obj->latitude);
			}
			else if ( strcmp(tname,"longitude")==0 )
			{
				valptr = &(obj->longitude);
			}
			else if (valptr == NULL)
			{
				syntax_error(filename,linenum,"invalid property: %s.%s", oname, tname);
				REJECT;
			}
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;
			rpn_stk[rpn_i].val = *valptr;
			++rpn_sz;
			++rpn_i;
		} else if ((LITERAL("$") || LITERAL("this.")) && TERM(name(HERE,tname,sizeof(tname)))){
			double *valptr = object_get_double_by_name(obj, tname);
			if(valptr == NULL){
				syntax_error(filename,linenum,"invalid property: %s.%s", obj->oclass->name, tname);
				REJECT;
			}
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;
			rpn_stk[rpn_i].val = *valptr;
			++rpn_sz;
			++rpn_i;
		} else if (TERM(functional(HERE, &tVal))){ /* captures reals too */
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;

			rpn_stk[rpn_i].val = tVal;
			++rpn_i;
			++rpn_sz;
		} else if(TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(name(HERE,pname,sizeof(pname)))){
			/* obj.prop*/
			OBJECT *otarg = NULL;
			ACCEPT;
			if WHITE ACCEPT;
			if(0 == strcmp(oname, "parent")){
				otarg = obj->parent;
			} else {
				otarg = object_find_name(oname);
			}
			if(otarg == NULL){ // delayed checking
				// disabled for now
				syntax_error(filename,linenum,"unknown reference: %s.%s", oname, pname);
				output_error("may be an order issue, delayed reference checking is a todo");
				REJECT;
			} else {
				double *valptr = object_get_double_by_name(otarg, pname);
				if(valptr == NULL){
					syntax_error(filename,linenum,"invalid property: %s.%s", oname, pname);
					REJECT;
				}
				rpn_stk[rpn_i].op = 0;
				rpn_stk[rpn_i].val = *valptr;
				++rpn_sz;
				++rpn_i;
			}
		} else { /* oops */
			syntax_error(filename,linenum,"unrecognized token within: %s9", HERE-2);
			REJECT;
			/* It looked like an expression.  Give fair warning. */
		}
	}

	/* depth == 0 ~ pop the op stack to the rpn queue */
	while(op_i >= 0){
		if(op_stk[op_i] != OP_OPEN){
			rpn_stk[rpn_i].op = op_stk[op_i];
			rpn_stk[rpn_i].val = 0.0;
			++rpn_i;
		}
		--op_i;
	}
	/* if no semicolon, there's a bigger error, so we don't check that here */

	/* postfix algorithm */
	/*	- while there are input tokens left,
	 *		- read the next input token
	 *		- if the token is a value
	 *			- push the token onto a stack
	 *		- if the token is an operator
	 *			- it is known a priori that the operator takes N arguments
	 *			- if there are fewer than N values on the stack, error.
	 *			- else pop the top n values from the stack
	 *			- evaluate the operator with with the values as arguments
	 *			- push the returned value back onto the stack
	 *	- iff one value remains on the stack, return that value
	 *	- if more values exist on the stack, error
	 */

	rpn_i = 0;

	for(i = 0; i < rpn_sz; ++i){
		if(rpn_stk[i].op == 0){ /* push value */
			val_q[val_i++] = rpn_stk[i].val;
		} else if(rpn_stk[i].op > 0){ /* binary operator */
			double popval = val_q[--val_i];
			if(val_i < 0){
				syntax_error(filename,linenum,"insufficient arguments in equation", rpn_stk[i].op);
				REJECT;
			}
			switch(rpn_stk[i].op){
				case OP_POW:
					val_q[val_i-1] = pow(val_q[val_i-1], popval);
					break;
				case OP_MULT:
					val_q[val_i-1] *= popval;
					break;
				case OP_MOD:
					val_q[val_i-1] = fmod(val_q[val_i-1], popval);
					break;
				case OP_DIV:
					val_q[val_i-1] /= popval;
					break;
				case OP_ADD:
					val_q[val_i-1] += popval;
					break;
				case OP_SUB:
					val_q[val_i-1] -= popval;
					break;
				default:
					syntax_error(filename,linenum,"unrecognized operator index %i (bug!)", rpn_stk[i].op);
					REJECT;
			}
		} else if(rpn_stk[i].op < 0){ /* rpn_func */
			int j;
			int count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
			for(j = 0; j < count; ++j){
				if(rpn_map[j].index == rpn_stk[i].op){
					double popval = val_q[--val_i];
					if(val_i < 0){
						syntax_error(filename,linenum,"insufficient arguments in equation", rpn_stk[i].op);
						REJECT;
					}
					val_q[val_i++] = (*rpn_map[j].fptr)(popval);
					break;
				}
			}
			if(j == count){ /* missed */
				syntax_error(filename,linenum,"unrecognized function index %i (bug!)", rpn_stk[i].op);
				REJECT;
			}

		}
	}
	if((val_i > 1)){
		syntax_error(filename,linenum,"too many values in equation!");
		REJECT;
	}
	*pValue = val_q[0];
	DONE;
}

int GldLoader::functional_unit(PARSER,double *pValue,UNIT **unit)
{
	START;
	if TERM(functional(HERE,pValue))
	{
		*unit = NULL;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

int GldLoader::complex_value(PARSER, complex *pValue)
{
	double r, i, m, a;
	START;
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("i"))
	{
		pValue->Re() = r;
		pValue->Im() = i;
		pValue->Notation() = I;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("j"))
	{
		pValue->Re() = r;
		pValue->Im() = i;
		pValue->Notation() = J;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("d"))
	{
		pValue->Re() = m*cos(a*PI/180);
		pValue->Im() = m*sin(a*PI/180);
		pValue->Notation() = A;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("r"))
	{
		pValue->Re() = m*cos(a);
		pValue->Im() = m*sin(a);
		pValue->Notation() = R;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))))
	{
		pValue->Re() = m;
		pValue->Im() = 0.0;
		pValue->Notation() = I;
		ACCEPT;
		DONE;
	}

	REJECT;
}

int GldLoader::complex_unit(PARSER,complex *pValue,UNIT **unit)
{
	START;
	if TERM(complex_value(HERE,pValue))
	{
		*unit = NULL;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

int GldLoader::time_value_seconds(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("s")) { *t *= TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("S")) { *t *= TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int GldLoader::time_value_minutes(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("m")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("M")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int GldLoader::time_value_hours(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("h")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("H")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int GldLoader::time_value_days(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("d")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("D")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int GldLoader::time_value_datetime(PARSER, TIMESTAMP *t)
{
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if ( LITERAL("'") || LITERAL("\"") ) ACCEPT;
	if (TERM(integer16(HERE,(short*)&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,(short*)&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,(short*)&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,(short*)&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,(short*)&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,(short*)&dt.second)) && ( LITERAL("'") || LITERAL("\"") ))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		strcpy(dt.tz,"");
		*t = mkdatetime(&dt);
		if (*t!=-1)
		{
			ACCEPT;
		}
		else
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::time_value_datetimezone(PARSER, TIMESTAMP *t)
{
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if (LITERAL("'")||LITERAL("\"")) ACCEPT;
	if (TERM(integer16(HERE,(short*)&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,(short*)&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,(short*)&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,(short*)&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,(short*)&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,(short*)&dt.second)) && LITERAL(" ")
		&& TERM(name(HERE,dt.tz,sizeof(dt.tz))) && (LITERAL("'")||LITERAL("\"")))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		*t = mkdatetime(&dt);
		if (*t!=-1)
		{
			ACCEPT;
		}
		else
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::time_value_isodatetime(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	char timevalue[1024];
	if (LITERAL("\"") && TERM(delim_value(HERE,timevalue,sizeof(timevalue),"\"")) && LITERAL("\"") )
	{
		*t = convert_to_timestamp(timevalue);
		if (*t!=-1)
		{
			ACCEPT;
		}
		else
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::time_value(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(time_value_seconds(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_minutes(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_hours(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_days(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetime(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetimezone(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_isodatetime(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(integer(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	else
	{
		REJECT;
	}
	DONE;
}

double GldLoader::load_latitude(char *buffer)
{
	char oname[128], pname[128];
	double v = convert_to_latitude(buffer);
	if ( sscanf(buffer,"(%[^.].%[^)])",oname,pname)==2 && strcmp(pname,"latitude")==0 )
	{
		OBJECT *obj = object_find_name(oname);
		if ( obj==NULL )
			syntax_error(filename,linenum,"'%s' does not refer to an existing object",buffer);
		return obj->latitude;
	}
	else if ( isnan(v) && ( strcmp(buffer,"")!=0 || stricmp(buffer, "none")!=0 ) )
	{
		syntax_error(filename,linenum,"'%s' is not a valid latitude",buffer);
	}
	else
	{
		IN_MYCONTEXT output_debug("%s(%d): latitude is converted to %lf", filename, linenum, v);
	}
	return v;
}

double GldLoader::load_longitude(char *buffer)
{
	char oname[128], pname[128];
	double v = convert_to_longitude(buffer);
	if ( sscanf(buffer,"(%[^.].%[^)])",oname,pname)==2 && strcmp(pname,"longitude")==0 )
	{
		OBJECT *obj = object_find_name(oname);
		if ( obj==NULL )
			syntax_error(filename,linenum,"'%s' does not refer to an existing object",buffer);
		return obj->longitude;
	}
	else if ( isnan(v) && ( strcmp(buffer,"")!=0 || stricmp(buffer, "none")!=0 ) )
	{
		syntax_error(filename,linenum,"'%s' is not a valid longitude",buffer);
	}
	else
	{
		IN_MYCONTEXT output_debug("%s(%d): longitude is convert to %lf", filename, linenum, v);
	}
	return v;
}

int GldLoader::clock_properties(PARSER)
{
	TIMESTAMP tsval;
	char timezone[64];
	double realval;
	START;
	if WHITE ACCEPT;
	if (LITERAL("tick") && WHITE)
	{
		if (TERM(real_value(HERE,&realval)) && (WHITE,LITERAL(";")))
		{
			if (realval!=TS_RESOLUTION)
			{
				syntax_error(filename,linenum,"timestamp resolution %g does not match system resolution %g, this version does not support variable tick", realval, TS_RESOLUTION);
				REJECT;
			}
			ACCEPT;
			goto Next;
		}
		syntax_error(filename,linenum,"expected tick value");
		REJECT;
	}
	OR if (LITERAL("timestamp") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			//global_clock = tsval;
			global_starttime = tsval; // used to affect start time, before with
			ACCEPT;
			goto Next;
		}
		syntax_error(filename,linenum,"expected time value");
		REJECT;
	}
	OR if (LITERAL("starttime") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			if ( tsval <= global_stoptime )
			{
				global_starttime = tsval;
			}
			else
			{
				syntax_error(filename,linenum,"starttime before stoptime");
			}
			ACCEPT;
			goto Next;
		}
		syntax_error(filename,linenum,"expected time value");
		REJECT;
	}
	OR if (LITERAL("stoptime") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			if ( tsval >= global_starttime )
			{
				global_stoptime = tsval;
			}
			else
			{
				syntax_error(filename,linenum,"stoptime after starttime");
			}
			ACCEPT;
			goto Next;
		}
		syntax_error(filename,linenum,"expected time value");
		REJECT;
	}
	OR if (LITERAL("timezone") && WHITE)
	{
		if (TERM(value(HERE,timezone,sizeof(timezone))) && (WHITE,LITERAL(";")) && strlen(timezone)>0)
		{
			if (timestamp_set_tz(timezone)==NULL)
				output_warning("%s(%d): timezone %s is not defined",filename,linenum,timezone);
				/* TROUBLESHOOT
					The specified timezone is not defined in the timezone file <code>.../etc/tzinfo.txt</code>.
					Try using an known timezone, or add the desired timezone to the timezome file and try again.
				 */
			ACCEPT;
			goto Next;
		}
		syntax_error(filename,linenum,"expected time zone specification");
		REJECT;
	}
	OR if (WHITE,LITERAL("}")) {/* don't accept yet */ DONE;}
	OR { syntax_error_here(HERE); REJECT; }
	/* may be repeated */
Next:
	if TERM(clock_properties(HERE)) ACCEPT;
	DONE;
}

int GldLoader::pathname(PARSER, char *path, int size)
{
	START;
	if TERM(pattern(HERE,"[-A-Za-z0-9/\\:_,. ]",path,size)) {ACCEPT;}
	else REJECT;
	DONE;
}

/** Expanded values support in-place expansion of special context sensitive variables.
	Expanded values are enclosed in backquotes. The variables are specified using the
	{varname} syntax.  The following variables are supported:

	{file} embeds the current file (full path,name,extension)
	{filename} embeds the name of the file (no path, no extension)
	{fileext} embeds the extension of the file (no path, no name)
	{filepath} embeds the path of the file (no name, no extension)
	{line} embeds the current line number
	{namespace} embeds the name of the current namespace
	{class}	embeds the classname of the current object
	{id} embeds the id of the current object
	{var} embeds the current value of the current object's variable <var>

 **/
int GldLoader::expanded_value(const char *text, char *result, int size, const char *delims)
{
	int n=0;
	if (text[n] == '`')
	{
		n++;
		memset(result,0,size--); /* preserve the string terminator even when buffer is full */
		for ( ; text[n]!='`'; n++)
		{
			if (size==0)
			{
				syntax_error(filename,linenum,"string expansion buffer overrun");
				return 0;
			}
			if (text[n]=='{')
			{
				char varname[256];
				char value[1024];
				char path[1024], name[1024], ext[1024];
				filename_parts(filename,path,name,ext);

				if (sscanf(text+n+1,"%255[a-zA-Z0-9_:]",varname)==0)
				{
					syntax_error(filename,linenum,"expanded string variable syntax error");
					return 0;
				}
				n+=(int)strlen(varname)+1;
				if (text[n]!='}')
				{
					syntax_error(filename,linenum,"expanded string variable missing closing }");
					return 0;
				}

				/* expanded specials variables */
				if (strcmp(varname,"file")==0)
					strcpy(value,filename);
				else if (strcmp(varname,"filename")==0)
					strcpy(value,name);
				else if (strcmp(varname,"filepath")==0)
					strcpy(value,path);
				else if (strcmp(varname,"fileext")==0)
					strcpy(value,ext);
				else if (strcmp(varname,"namespace")==0)
					object_namespace(value,sizeof(value));
				else if (strcmp(varname,"class")==0)
					strcpy(value,current_object?current_object->oclass->name:"");
				else if (strcmp(varname,PACKAGE)==0)
					strcpy(value,global_execdir);
				else if (strcmp(varname,"hostname")==0)
					strcpy(value,global_hostname);
				else if (strcmp(varname,"hostaddr")==0)
					strcpy(value,global_hostaddr);
				else if (strcmp(varname,"cpu")==0)
					snprintf(value,sizeof(value)-1,"%d",sched_get_cpuid(0));
				else if (strcmp(varname,"pid")==0)
					snprintf(value,sizeof(value)-1,"%d",sched_get_procid());
				else if (strcmp(varname,"port")==0)
					snprintf(value,sizeof(value)-1,"%d",global_server_portnum);
				else if (strcmp(varname,"mastername")==0)
					strcpy(value,"localhost"); /* @todo copy actual master name */
				else if (strcmp(varname,"masteraddr")==0)
					strcpy(value,"127.0.0.1"); /* @todo copy actual master addr */
				else if (strcmp(varname,"masterport")==0)
					strcpy(value,"6267"); /* @todo copy actual master port */
				else if (strcmp(varname,"id")==0)
				{
					if (current_object)
						snprintf(value,sizeof(value)-1,"%d",current_object->id);
					else
						strcpy(value,"");
				}
				else if ( object_get_value_by_name(current_object,varname,value,sizeof(value)))
				{
					/* value is ok */
				}
				else if ( global_getvar(varname,value,sizeof(value)) )
				{
					/* value is ok */
				}
				else
				{
					syntax_error(filename,linenum,"variable '%s' not found in this context", varname);
					return 0;
				}

				/* accept the value */
				if ((int)strlen(value)>=size)
				{
					syntax_error(filename,linenum,"string expansion buffer overrun");
					return 0;
				}
				strcat(result,value);
				size -= (int)strlen(value);
				result += strlen(value);
			}
			else
			{
				*result++ = text[n];
				size--;
			}
		}
		if (text[n+1]==';')
			return n+1;
		else
		{
			syntax_error(filename,linenum,"missing terminating ;");
			return 0;
		}
	}
	else if (delims==NULL)
		return value(text,result,size);
	else
		return delim_value(text,result,size,delims);
}

/** alternate_value allows the use of ternary operations, e.g.,

		 property (expression) ? negzero_value : positive_value ;

 **/

int GldLoader::alternate_value(PARSER, char *value, int size)
{
	double test;
	char value1[1024];
	char value2[1024];
	START;
	if (WHITE) ACCEPT;
	if (TERM(expression(HERE,&test,NULL,current_object)) && (WHITE,LITERAL("?")))
	{
		if ((WHITE,TERM(expanded_value(HERE,value1,sizeof(value1)," \t\n:"))) && (WHITE,LITERAL(":")) && (WHITE,TERM(expanded_value(HERE,value2,sizeof(value2)," \n\t;"))))
		{
			ACCEPT;
			if (test>0)
			{
				if ((int)strlen(value1)>size)
				{
					syntax_error(filename,linenum,"alternate value 1 is too large ;");
					REJECT;
				}
				else
				{
					strcpy(value,value1);
					ACCEPT;
				}
			}
			else
			{
				if ((int)strlen(value2)>size)
				{
					syntax_error(filename,linenum,"alternate value 2 is too large ;");
					REJECT;
				}
				else
				{
					strcpy(value,value2);
					ACCEPT;
				}
			}
		}
		else
		{
			syntax_error(filename,linenum,"missing or invalid alternate values;");
			REJECT;
		}
		DONE;
	}
	OR if (TERM(expanded_value(HERE,value,size,NULL)))
	{
		ACCEPT;
		DONE
	}
	REJECT;
	DONE;
}

/** Line specs are generated internally to maintain proper filename and line number context.
	Line specs are always alone on a line and take the form @pathname;linenum
 **/
int GldLoader::line_spec(PARSER)
{
	char fname[1024];
	int32 lnum;
	START;
	if LITERAL("@")
	{
		if (TERM(pathname(HERE,fname,sizeof(fname))) && LITERAL(";") && TERM(integer32(HERE,&lnum)))
		{
			strcpy(filename,fname);
			linenum = lnum;
			ACCEPT; DONE;
		}
		else
		{
			syntax_error(filename,linenum,"@ syntax error");
			REJECT; DONE;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::clock_block(PARSER)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("clock") ACCEPT else REJECT;
	if WHITE ACCEPT;
	if LITERAL("{") ACCEPT
	else
	{
		syntax_error(filename,linenum,"expected clock block opening {");
		REJECT;
	}
	if WHITE ACCEPT;
	// cache timestamp for delayed timestamp offsets
	if TERM(clock_properties(HERE)) ACCEPT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT else
	{
		syntax_error(filename,linenum,"expected clock block closing }");
		REJECT;
	}
	DONE;
}

int GldLoader::module_properties(PARSER, MODULE *mod)
{
	int64 val;
	char classname[MAXCLASSNAMELEN];
	char propname[MAXPROPNAMELEN];
	char propvalue[MAXPROPERTYVALUELEN];
	START;
	if WHITE ACCEPT;
	if (LITERAL("major") && (WHITE))
	{
		if (TERM(integer(HERE,&val)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (val!=mod->major)
				{
					syntax_error(filename,linenum,"%s has an incompatible module major version",mod->name);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				syntax_error(filename,linenum,"expected ; after %s module major number", mod->name);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"expected %s module major number", mod->name);
			REJECT;
		}
	}
	OR if (LITERAL("minor") && (WHITE))
	{
		if (TERM(integer(HERE,&val)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (val!=mod->minor)
				{
					syntax_error(filename,linenum,"%s has an incompatible module minor version",mod->name);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				syntax_error(filename,linenum,"expected ; after %s module minor number", mod->name);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"expected %s module minor number", mod->name);
			REJECT;
		}
	}
	OR if (LITERAL("class") && WHITE)
	{
		if TERM(name(HERE,classname,sizeof(classname)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				CLASS *oclass = class_get_class_from_classname(classname);
				if (oclass==NULL || oclass->module!=mod)
				{
					syntax_error(filename,linenum,"module '%s' does not implement class '%s'", mod->name, classname);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				syntax_error(filename,linenum,"expected ; after module %s class %s declaration", mod->name, classname);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"missing class name in module %s class declaration", mod->name);
			REJECT;
		}
	}
	OR if (TERM(name(HERE,propname,sizeof(propname))) && (WHITE))
	{
		current_object = NULL; /* object context */
		current_module = mod; /* module context */
		if TERM(alternate_value(HERE,propvalue,sizeof(propvalue)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (module_setvar(mod,propname,(const char*)propvalue)>0)
				{
					ACCEPT;
					goto Next;
				}
				else
				{
					syntax_error(filename,linenum,"invalid module %s property '%s'", mod->name, propname);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"expected ; after module %s property specification", mod->name);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"missing module %s property %s value", mod->name, propname);
			REJECT;
		}
	}
	OR if LITERAL("}") {/* don't accept yet */ DONE;}
	OR { syntax_error_here(HERE); REJECT; }
	/* may be repeated */
Next:
	if TERM(module_properties(HERE,mod)) ACCEPT;
	DONE;
}

int GldLoader::module_block(PARSER)
{
	char module_name[64];
	char fmod[8],mod[54];
	MODULE *module;
	START;
	if WHITE ACCEPT;
	if (LITERAL("module") && WHITE) ACCEPT else REJECT;
	//if WHITE ACCEPT;
	/* load options should go here and get converted to argc/argv */

	/* foreign module */
	if (TERM(name(HERE,fmod,sizeof(fmod))) && LITERAL("::") && TERM(name(HERE,mod,sizeof(mod))))
	{
		snprintf(module_name,sizeof(module_name)-1,"%s::%s",fmod,mod);
		if ((module=module_load(module_name,0,NULL))!=NULL)
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"%s module '%s' load failed, %s", fmod, mod,errno?strerror(errno):"(no details)");
			REJECT;
		}
	}

	OR
	/* native C/C++ module */
	if (TERM(name(HERE,module_name,sizeof(module_name))))
	{
		if ((module=module_load(module_name,0,NULL))!=NULL)
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"module '%s' load failed, %s", module_name,errno?strerror(errno):"(no details)");
			REJECT;
		}
	}
	if WHITE ACCEPT;
	if LITERAL(";") {ACCEPT;DONE;}
	OR
	if LITERAL("{") ACCEPT
	else
	{
		syntax_error(filename,linenum,"expected module %s block opening {", module_name);
		REJECT;
	}
	if TERM(module_properties(HERE,module)) ACCEPT else REJECT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT
	else
	{
		syntax_error(filename,linenum,"expected module %s block closing }", module_name);
		REJECT;
	}
	DONE;
}

int GldLoader::property_specs(PARSER, KEYWORD **keys)
{
	char keyname[32];
	int32 keyvalue;
	START;
	if WHITE ACCEPT;
	if ( TERM(name(HERE,keyname,sizeof(keyname))) && (WHITE,LITERAL("=")) && TERM(integer32(HERE,&keyvalue)))
	{
		*keys = (KEYWORD*)malloc(sizeof(KEYWORD));
		(*keys)->next = NULL;
		if WHITE ACCEPT;
		if LITERAL(",") ACCEPT;
		if WHITE ACCEPT;
		if TERM(property_specs(HERE, &((*keys)->next))) {}
		ACCEPT;
		strcpy((*keys)->name,keyname);
		(*keys)->value = keyvalue;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::property_type(PARSER, PROPERTYTYPE *ptype, KEYWORD **keys)
{
	char type[32];
	START;
	if WHITE ACCEPT;
	if TERM(name(HERE,type,sizeof(type)))
	{
		*ptype = class_get_propertytype_from_typename(type);
		if (*ptype==PT_void)
		{
			syntax_error(filename,linenum,"class member %s is not recognized", type);
			REJECT;
		}
		if (WHITE,LITERAL("{"))
		{
			if (TERM(property_specs(HERE,keys)) && (WHITE,LITERAL("}")))
			{
				ACCEPT;}
			else
			{	REJECT;}
		}
		ACCEPT;
	}
	else REJECT;
	DONE;
}

int GldLoader::class_intrinsic_function_name(PARSER, CLASS *oclass, int64 *function, const char **ftype, const char **fname)
{
	char buffer[1024];
	START;
	if WHITE ACCEPT;
	if LITERAL("create")
	{
		*ftype = "int64";
		*fname = "create";
		*function |= FN_CREATE;
		ACCEPT;
	}
	else if LITERAL("init")
	{
		*ftype = "int64";
		*fname = "init";
		*function |= FN_INIT;
		ACCEPT;
	}
	else if LITERAL("precommit")
	{
		*ftype = "int64";
		*fname = "precommit";
		*function |= FN_PRECOMMIT;
		ACCEPT;
	}
	else if LITERAL("presync")
	{
		*ftype = "TIMESTAMP";
		*fname = "presync";
		oclass->passconfig = PASSCONFIG(oclass->passconfig|PC_PRETOPDOWN);
		*function |= FN_PRESYNC;
		ACCEPT;
	}
	else if LITERAL("sync")
	{
		*ftype = "TIMESTAMP";
		*fname = "sync";
		oclass->passconfig = PASSCONFIG(oclass->passconfig|PC_BOTTOMUP);
		*function |= FN_SYNC;
		ACCEPT;
	}
	else if LITERAL("postsync")
	{
		*ftype = "TIMESTAMP";
		*fname = "postsync";
		oclass->passconfig = PASSCONFIG(oclass->passconfig|PC_POSTTOPDOWN);
		*function |= FN_POSTSYNC;
		ACCEPT;
	}
	else if LITERAL("recalc")
	{
		*ftype = "int64";
		*fname = "recalc";
		*function |= FN_RECALC;
		ACCEPT;
	}
	else if LITERAL("notify")
	{
		*ftype = "int64";
		*fname = "notify";
		*function |= FN_NOTIFY;
		ACCEPT;
	}
	else if LITERAL("plc")
	{
		*ftype = "TIMESTAMP";
		*fname = "plc";
		*function |= FN_PLC;
		ACCEPT;
	}
	else if LITERAL("isa")
	{
		*ftype = "int64";
		*fname = "isa";
		*function |= FN_ISA;
		ACCEPT;
	}
	else if LITERAL("commit")
	{
		*ftype = "TIMESTAMP";
		*fname = "commit";
		*function |= FN_COMMIT;
		ACCEPT;
	}
	else if LITERAL("finalize")
	{
		*ftype = "int64";
		*fname = "finalize";
		*function |= FN_FINALIZE;
		ACCEPT;
	}
	else if TERM(name(HERE,buffer,sizeof(buffer)))
	{
		syntax_error(filename,linenum,"'%s' is not a recognized intrinsic function",buffer);
		REJECT;
	}
	DONE;
}

int GldLoader::argument_list(PARSER, char *args, int size)
{
	START;
	if WHITE ACCEPT;
	strcpy(args,"");
	if (LITERAL("("))
	{
		if (WHITE,TERM(pattern(HERE,"[^)]",args,size)))
		{
			ACCEPT;
		}
		if (WHITE,LITERAL(")"))
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"unterminated argument list");
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::source_code(PARSER, char *code, int size)
{
	int _n = 0;
	int nest = 0;
	char buffer[64];
	enum {CODE,COMMENTBLOCK,COMMENTLINE,STRING,CHAR} state=CODE;
	while (*_p!='\0')
	{
		char c1 = _p[0];
		char c2 = _p[1];
		if (c1=='\n')
			linenum++;
		if (size==0)
		{
			syntax_error(filename,linenum,"insufficient buffer space to load code");
			return 0;
		}
		switch(state) {
		case CODE:
			if (c1==';' && nest==0)
			{
					code[_n]='\0';
					return _n;
			}
			else if (c1=='{')
			{
				nest++;
				COPY(code);
			}
			else if (c1=='}')
			{
				if (nest>0)
				{
					nest--;
					COPY(code);
				}
				else
				{
					syntax_error(filename,linenum,"unmatched }");
					return 0;
				}
			}
			else if (c1=='/' && c2=='*')
				state = COMMENTBLOCK;
			else if (c1=='/' && c2=='/')
				state = COMMENTLINE;
			else if (c1=='"')
			{
				state = STRING;
				COPY(code);
			}
			else if (c1=='\'')
			{
				state = CHAR;
				COPY(code);
			}
			else
				COPY(code);
			break;
		case COMMENTBLOCK:
			if (c1=='*' && c2=='/')
			{
				int len = strlen(code);
				if (!global_debug_output && global_getvar("noglmrefs",buffer,63)==NULL)
					snprintf(code+len,size-len-1,"#line %d \"%s\"\n", linenum,forward_slashes(filename).c_str());
				state = CODE;
			}
			break;
		case COMMENTLINE:
			if (c1=='\n')
				state = CODE;
			break;
		case STRING:
			if (c1=='"')
				state = CODE;
			else if (c1=='\n')
			{
				syntax_error(filename,linenum,"unterminated string constant");
				return 0;
			}
			COPY(code);
			break;
		case CHAR:
			if (c1=='\'')
				state = CODE;
			else if (c1=='\n')
			{
				syntax_error(filename,linenum,"unterminated char constant");
				return 0;
			}
			COPY(code);
			break;
		default:
			COPY(code);
			break;
		}
	}
	syntax_error(filename,linenum,"unterminated code block");
	return 0;
}

int GldLoader::class_intrinsic_function(PARSER, CLASS *oclass, int64 *functions, char *code, int size)
{
	const char *fname = NULL;
	const char *ftype = NULL;
	char arglist[1024];
	char source[65536];
	int startline;
	START;
	if WHITE ACCEPT;
	if (LITERAL("intrinsic") && WHITE && TERM(class_intrinsic_function_name(HERE,oclass,functions,&ftype,&fname)) && (WHITE,TERM(argument_list(HERE,arglist,sizeof(arglist)))) && (startline=linenum,(WHITE,TERM(source_code(HERE,source,sizeof(source))))) && (WHITE,LITERAL(";")))
	{
		if (oclass->module==NULL)
		{
			mark_linex(filename,startline);
			append_code("\t%s %s (%s) {\n\tOBJECT*my=((OBJECT*)this)-1; ",ftype,fname,arglist);
			append_code("\n#ifdef GLAPI3\n\tgl_core gl(my);\n#endif");
			append_code("\n\ttry %s ",source);
			append_code("catch (char *msg) {callback->output_error(\"%%s[%%s:%%d] exception - %%s\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id,msg); return 0;} ");
			append_code("catch (const char *msg) {callback->output_error(\"%%s[%%s:%%d] exception - %%s\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id,msg); return 0;} ");
			append_code("catch (...) {callback->output_error(\"%%s[%%s:%%d] unhandled exception\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id); return 0;} ");
			append_code("callback->output_error(\"%s::%s(%s) not all paths return a value\"); return 0;}\n",oclass->name,fname,arglist);
			append_code("/*RESETLINE*/\n");
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"intrinsic functions not permitted in static classes");
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::class_export_function(PARSER, CLASS *oclass, char *fname, int fsize, char *arglist, int asize, char *code, int csize)
{
	int startline;
	char buffer[64];
	START;
	if WHITE ACCEPT;
	if (LITERAL("export")
		&& (WHITE,TERM(name(HERE,fname,fsize)))
		&& (WHITE,TERM(argument_list(HERE,arglist,asize)))
		&& (startline=linenum,(WHITE,TERM(source_code(HERE,code,csize)))) && (WHITE,LITERAL(";")))
	{
		if (oclass->module==NULL)
		{
			mark_linex(filename,startline);
			append_code("\tstatic int64 %s (%s) %s;\n/*RESETLINE*/\n",fname,arglist,code);

			if (global_getvar("noglmrefs",buffer,63)==NULL)
				append_init("#line %d \"%s\"\n"
					"\tif ((*(callback->function.define))(oclass,\"%s\",(FUNCTIONADDR)&%s::%s)==NULL) return 0;\n"
					"/*RESETLINE*/\n", startline, forward_slashes(filename).c_str(),
					fname,oclass->name,fname);

			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"export functions not permitted in static classes");
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::class_explicit_declaration(PARSER, char *type, int size)//, bool *is_static)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("private")
	{
		strcpy(type,"private");
		ACCEPT;
	}
	else if LITERAL("protected")
	{
		strcpy(type,"protected");
		ACCEPT;
	}
	else if LITERAL("public")
	{
		strcpy(type,"public");
		ACCEPT;
	}
	else if LITERAL("static")
	{
		strcpy(type,"static");
		ACCEPT;
	}
	else
		REJECT;
	WHITE;
/*	if LITERAL("static")
	{
		//strcpy(type,"static");
		*is_static = true;
	} else {
		*is_static = false;
	}
*/
	DONE;
}

int GldLoader::class_explicit_definition(PARSER, CLASS *oclass)
{
	int startline;
	char type[64];
	char code[4096];
//	bool is_static;
	START;
	if WHITE ACCEPT;
	if (TERM(class_explicit_declaration(HERE,type,sizeof(type)/*,&is_static*/)))
	{
		if (oclass->module==NULL)
		{
			startline=linenum;
			if WHITE ACCEPT;
			if TERM(source_code(HERE,code,sizeof(code)))
			{
				if WHITE ACCEPT;
				if LITERAL(";")
				{
					mark_linex(filename,startline);
					append_code("\t%s: %s;\n",type,code);
					append_code("/*RESETLINE*/\n");
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"missing ; after code block");
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"syntax error in code block");
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"explicit definitions not permitted in static classes");
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::class_external_function(PARSER, CLASS *oclass, CLASS **eclass,char *fname, int fsize)
{
	char classname[MAXCLASSNAMELEN+1];
	START;
	if (LITERAL("function")
		&& WHITE
		&& TERM(name(HERE,classname,sizeof(classname)))
		&& LITERAL("::")
		&& TERM(name(HERE,fname,fsize)) && (WHITE,LITERAL(";")))
	{
		if (oclass->module==NULL)
		{
			CLASS *oclass = class_get_class_from_classname(classname);
			if (oclass==NULL)
			{
				syntax_error(filename,linenum,"class '%s' does not exist", classname);
				REJECT;
			}
			else
			{
				if (class_get_function(classname,fname))
				{
					*eclass = oclass;
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"class '%s' does not define function '%s'", classname, fname);
					REJECT;
				}
			}
		}
		else
		{
			syntax_error(filename,linenum,"external functions not permitted in static classes");
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::class_event_handler(PARSER, CLASS *oclass)
{
	char value[1024];
	START;
	if WHITE ACCEPT;
	if ( LITERAL("on_init") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.init = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_precommit") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.precommit = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_presync") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.presync = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_sync") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.sync = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_postsync") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.postsync = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_commit") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.commit = strdup(value);
		ACCEPT;
	}
	else if ( LITERAL("on_finalize") && (WHITE,TERM(quoted_string(HERE,value,sizeof(value)))) && (WHITE,LITERAL(";")) )
	{
		oclass->events.finalize = strdup(value);
		ACCEPT;
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::class_parent_definition(PARSER, CLASS *oclass)
{
	char classname[1024];
	START;
	if WHITE ACCEPT;
	if ( LITERAL("parent") )
	{
		if WHITE ACCEPT;
		if ( TERM(name(HERE,classname,sizeof(classname))) )
		{
			if ( oclass->module != NULL )
			{
				syntax_error(filename,linenum,"cannot set parent of class defined in module");
				REJECT;
			}
			else
			{
				oclass->parent = class_get_class_from_classname(classname);
				if ( oclass->parent == NULL )
				{
					syntax_error(filename,linenum,"parent class '%s' not found", classname);
					REJECT;
				}
				else if ( WHITE,LITERAL(";") )
				{
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"missing semicolon after parent class name");
					REJECT;
				}
			}
		}
		else
		{
			syntax_error(filename,linenum,"missing or invalid parent class name");
			REJECT;
		}
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::class_properties(PARSER, CLASS *oclass, int64 *functions, char *initcode, int initsize)
{
	static char code[65536];
	char arglist[1024];
	char fname[64];
	char buffer[64];
	CLASS *eclass;
	PROPERTYTYPE type;
	char propname[64];
	KEYWORD *keys = NULL;
	UNIT *pUnit=NULL;
	START;
	if WHITE ACCEPT;
	if TERM(class_intrinsic_function(HERE,oclass,functions,code,sizeof(code)))
	{
		ACCEPT;
	}
	else if TERM(class_external_function(HERE,oclass,&eclass,fname,sizeof(fname)))
	{
		append_global("FUNCTIONADDR %s::%s = NULL;\n",oclass->name,fname);
		if (global_getvar("noglmrefs",buffer,63)==NULL)
			append_init("#line %d \"%s\"\n\tif ((%s::%s=gl_get_function(\"%s\",\"%s\"))==NULL) throw \"%s::%s not defined\";\n",
				linenum, forward_slashes(filename).c_str(), oclass->name, fname,
				eclass->name, fname, eclass->name, fname);
		append_code("\tstatic FUNCTIONADDR %s;\n",fname);
		ACCEPT;
	}
	else if TERM(class_explicit_definition(HERE, oclass))
	{
		ACCEPT;
	}
	else if TERM(class_event_handler(HERE, oclass))
	{
		ACCEPT;
	}
	else if TERM(class_export_function(HERE, oclass,fname,sizeof(fname),arglist,sizeof(arglist),code,sizeof(code)))
	{
		*functions |= FN_EXPORT;
		ACCEPT;
	}
	else if TERM(class_parent_definition(HERE,oclass) )
	{
		ACCEPT;
	}
	else if (TERM(property_type(HERE,&type,&keys)) && (WHITE,(TERM(nameunit(HERE,propname,sizeof(propname),&pUnit))||TERM(name(HERE,propname,sizeof(propname))))) && (WHITE,LITERAL(";")) )
	{
		PROPERTY *prop = class_find_property(oclass,propname);
		if (prop==NULL)
		{
			if (type==PT_void)
			{
				syntax_error(filename,linenum,"property type %s is not recognized", type);
				REJECT;
			}
			else
			{
				if (pUnit != NULL)
				{
					if (type==PT_double || type==PT_complex || type==PT_random)
						prop = class_add_extended_property(oclass,propname,type,pUnit->name);
					else
					{
						syntax_error(filename,linenum,"units not permitted for type %s", class_get_property_typename(type));
						REJECT;
					}
				}
				else if (keys!=NULL)
				{
					if (type==PT_enumeration || type==PT_set)
					{
						prop = class_add_extended_property(oclass,propname,type,NULL);
						prop->keywords = keys;
					}
					else
					{
						syntax_error(filename,linenum,"keys not permitted for type %s", class_get_property_typename(prop->ptype));
						REJECT;
					}
				}
				else
					prop = class_add_extended_property(oclass,propname,type,NULL);
				if (oclass->module==NULL)
				{
					mark_line();
					if (keys!=NULL)
					{
						KEYWORD *key;
						for (key=prop->keywords; key!=NULL; key=key->next)
							append_code("#define %s (0x%x)\n", key->name, key->value);
					}
					append_code("\t%s %s;\n", class_get_property_typename(prop->ptype), prop->name);
					append_code("/*RESETLINE*/\n");
				}
			}
		}
		else if (prop->ptype!=type)
		{
			syntax_error(filename,linenum,"property %s is defined in class %s as type %s", propname, oclass->name, class_get_property_typename(prop->ptype));
			REJECT;
		}
		ACCEPT;
	}
	else if LITERAL("}") {/* don't accept yet */ DONE;}
	else { syntax_error_here(HERE); REJECT; }
	/* may be repeated */
	if TERM(class_properties(HERE,oclass,functions,initcode,initsize)) ACCEPT;
	DONE;
}

int GldLoader::class_block(PARSER)
{
	char classname[MAXCLASSNAMELEN+1];
	CLASS *oclass;
	int64 functions = 0;
	char initcode[65536]="";
	char parent[64];
	enum {NONE, PRIVATE, PROTECTED, PUBLIC, EXTERNAL} inherit = NONE;
	START;
	if WHITE ACCEPT;
	if (LITERAL("class") && WHITE) /* enforced whitespace */
	{
		// startline = linenum;
		if TERM(name(HERE,classname,sizeof(classname)))
		{
			if (WHITE,LITERAL(":"))
			{
				if WHITE ACCEPT;
				if (LITERAL("public") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PUBLIC;
					ACCEPT;
				}
				else if (LITERAL("protected") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PROTECTED;
					ACCEPT;
				}
				else if (LITERAL("private") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PRIVATE;
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"missing inheritance qualifier");
					REJECT;
					DONE;
				}
				if (class_get_class_from_classname(parent)==NULL)
				{
					syntax_error(filename,linenum,"class %s inherits from undefined class %s", classname, parent);
					REJECT;
					DONE;
				}
			}
			if WHITE ACCEPT;
			if LITERAL("{")
			{
				oclass = class_get_class_from_classname(classname);
				if (oclass==NULL)
				{
					oclass = class_register(NULL,classname,0,PC_NOSYNC);
					mark_line();
					switch (inherit) {
					case NONE:
						append_code("class %s {\npublic:\n\t%s(MODULE*mod) {};\n", oclass->name, oclass->name);
						break;
					case PRIVATE:
						append_code("class %s : private %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					case PROTECTED:
						append_code("class %s : protected %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					case PUBLIC:
						append_code("class %s : public %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					default:
						output_error("class_block inherit status is invalid (inherit=%d)", inherit);
						REJECT;
						DONE;
						break;
					}
					mark_line();
				}
				ACCEPT;
			}
			else
			{
				syntax_error(filename,linenum,"expected class %s block opening {",classname);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"expected class name");
			REJECT;
		}
		if (TERM(class_properties(HERE,oclass,&functions,initcode,sizeof(initcode)))) ACCEPT;
		if WHITE ACCEPT;
		if LITERAL("}")
		{
			if ( oclass->module==NULL && functions!=0 )
			{
				append_code("};\n");
#define ENTERING(OBJ,X) if (strstr(global_trace,#X)!=NULL) append_code("trace(\"call %s::%s\",("#OBJ"));",oclass->name,#X)
#define EXITING(OBJ,X) if (strstr(global_trace,#X)!=NULL) append_code("trace(\"exit %s::%s\",("#OBJ"));",oclass->name,#X)

				append_code("/*RESETLINE*/\n");
				append_code("/*RESETLINE*/\n");
				append_code("extern \"C\" int64 create_%s(OBJECT **obj, OBJECT *parent)\n{\n",oclass->name);
				append_code(
						"\tif ((*obj=gl_create_object(myclass))==NULL)\n\t\treturn 0;\n"
						"\tif ( parent ) gl_set_parent(*obj,parent);\n", oclass->name,oclass->name);
					if (functions&FN_CREATE)
					{
						ENTERING(*obj,create);
						append_code("\tint64 ret = ((%s*)((*obj)+1))->create(parent);\n",oclass->name);
						EXITING(*obj,create);
						append_code("\treturn ret;\n}\n");
					}
					else
						append_code("\treturn 1;\n}\n");
				if (functions&FN_INIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 init_%s(OBJECT *obj, OBJECT *parent)\n{\n",oclass->name);
					ENTERING(*obj,init);
					append_code("\tint64 ret = ((%s*)(obj+1))->init(parent);\n",oclass->name);
					EXITING(*obj,init);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_PRECOMMIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 precommit_%s(OBJECT *obj, TIMESTAMP t1)\n{\n",oclass->name);
					ENTERING(*obj,precommit);
					append_code("\tint64 ret = ((%s*)(obj+1))->precommit(t1);\n",oclass->name);
					EXITING(*obj,precommit);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_SYNC || functions&FN_PRESYNC || functions&FN_POSTSYNC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 sync_%s(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)\n{\n",oclass->name);
					append_code("\tint64 t2 = TS_NEVER;\n\tswitch (pass) {\n");
					if (functions&FN_PRESYNC)
					{
						append_code("\tcase PC_PRETOPDOWN:\n");
						ENTERING(obj,presync);
						append_code("\t\tt2=((%s*)(obj+1))->presync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,presync);
						if ((functions&(FN_SYNC|FN_POSTSYNC))==0)
							append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					if (functions&FN_SYNC)
					{
						append_code("\tcase PC_BOTTOMUP:\n");
						ENTERING(obj,sync);
						append_code("\t\tt2=((%s*)(obj+1))->sync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,sync);
						if ((functions&FN_POSTSYNC)==0)
							append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					if (functions&FN_POSTSYNC)
					{
						append_code("\tcase PC_POSTTOPDOWN:\n");
						ENTERING(obj,postsync);
						append_code("\t\tt2=((%s*)(obj+1))->postsync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,postsync);
						append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					append_code("\tdefault:\n\t\tbreak;\n\t}\n\treturn t2;\n}\n");
				}
				if (functions&FN_PLC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 plc_%s(OBJECT *obj, TIMESTAMP t1)\n{\n",oclass->name);
					ENTERING(obj,plc);
					append_code("\tint64 t2 = ((%s*)(obj+1))->plc(obj->clock,t1);\n",oclass->name);
					EXITING(obj,plc);
					append_code("\treturn t2;\n}\n");
				}
				if (functions&FN_COMMIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" TIMESTAMP commit_%s(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)\n{\n",oclass->name);
					ENTERING(obj,commit);
					append_code("\tTIMESTAMP ret = ((%s*)(obj+1))->commit(t1, t2);\n",oclass->name);
					EXITING(obj,commit);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_ISA) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 isa_%s(OBJECT *obj, char *type)\n{\n",oclass->name);
					ENTERING(obj,isa);
					append_code("\tint64 ret = ((%s*)(obj+1))->isa(type);\n",oclass->name);
					EXITING(obj,isa);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_NOTIFY) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 notify_%s(OBJECT *obj, NOTIFYMODULE msg)\n{\n",oclass->name);
					ENTERING(obj,notify);
					append_code("\tint ret64 = ((%s*)(obj+1))->isa(type);\n",oclass->name);
					EXITING(obj,notify);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_RECALC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 recalc_%s(OBJECT *obj)\n{\n",oclass->name);
					ENTERING(obj,recalc);
					append_code("\tint ret64 = ((%s*)(obj+1))->recalc();\n",oclass->name);
					EXITING(obj,recalc);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_FINALIZE) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 finalize_%s(OBJECT *obj)\n{\n",oclass->name);
					ENTERING(*obj,create);
					append_code("\tint64 ret = ((%s*)(obj+1))->finalize();\n",oclass->name);
					EXITING(*obj,create);
					append_code("\treturn ret;\n}\n");
				}

				/* TODO add other intrinsics (notify, recalc, isa) */
				if (!compile_code(oclass,functions)) REJECT;
			} else if ( functions!=0 ) { // if module != NULL
				if(code_used){
					syntax_error(filename,linenum,"intrinsic functions found for compiled class");
					REJECT;
				}
			}
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"expected closing } after class block");
			REJECT;
		}
	}
	else REJECT;
	DONE;
}

int GldLoader::set_flags(OBJECT *obj, char *propval)
{
	if (convert_to_set(propval,&(obj->flags),object_flag_property())<=0)
	{
		syntax_error(filename,linenum,"flags of %s:%d %s could not be set to %s", obj->oclass->name, obj->id, obj->name, propval);
		return 0;
	};
	return 1;
}

int GldLoader::is_int(PROPERTYTYPE pt)
{
	if(pt == PT_int16 || pt == PT_int32 || pt == PT_int64){
		return (int)pt;
	} else {
		return 0;
	}
}

int GldLoader::schedule_ref(PARSER, SCHEDULE **sch)
{
	char name[64];
	START;
	if WHITE ACCEPT;
	if (TERM(dashed_name(HERE,name,sizeof(name))))
	{
		ACCEPT;
		if (((*sch)=schedule_find_byname(name))==NULL)
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::property_ref(PARSER, TRANSFORMSOURCE *xstype, void **ref, OBJECT *from)
{
	FULLNAME oname;
	char pname[64];
	START;
	if WHITE ACCEPT;
	if (TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(dotted_name(HERE,pname,sizeof(pname))))
	{
		OBJECT *obj = (strcmp(oname,"this")==0 ? from : object_find_name(oname));

		// object isn't defined yet
		if (obj==NULL)
		{
			// add to unresolved list
			char id[4096];
			snprintf(id,sizeof(id)-1,"%s.%s",oname,pname);
			*ref = (void*)add_unresolved(from,PT_double,NULL,from->oclass,id,filename,linenum,UR_TRANSFORM);
			ACCEPT;
		}
		else
		{
			PROPERTY *prop = object_get_property(obj,pname,NULL);
			if (prop==NULL)
			{
				syntax_error(filename,linenum,"property '%s' of object '%s' not found", oname,pname);
				REJECT;
			}
			else if (prop->ptype==PT_double)
			{
				*ref = (void*)object_get_addr(obj,pname);
				*xstype = XS_DOUBLE;
				ACCEPT;
			}
			else if (prop->ptype==PT_complex)
			{
				// TODO support R,I parts
				*ref = (void*)object_get_addr(obj,pname); // get R part only
				*xstype = XS_COMPLEX;
				ACCEPT;
			}
			else if (prop->ptype==PT_loadshape)
			{
				loadshape *ls = (loadshape*)object_get_addr(obj,pname);
				*ref = &(ls->load);
				*xstype = XS_LOADSHAPE;
				ACCEPT;
			}
			else if (prop->ptype==PT_enduse)
			{
				enduse *eu = (enduse*)object_get_addr(obj,pname);
				*ref = &(eu->total.Re());
				*xstype = XS_ENDUSE;
				ACCEPT;
			}
			else if ( prop->ptype==PT_random )
			{
				randomvar *rv = (randomvar*)object_get_addr(obj,pname);
				*ref = &(rv->value);
				*xstype = XS_RANDOMVAR;
				ACCEPT;
			}
			else
			{
				syntax_error(filename,linenum,"transform '%s.%s' does not reference a double or a double container like a loadshape", oname,pname);
				REJECT;
			}
		}
	}
	else
	{	REJECT;	}
	DONE;
}

int GldLoader::transform_source(PARSER, TRANSFORMSOURCE *xstype, void **source, OBJECT *from)
{
	SCHEDULE *sch;
	START;
	if WHITE ACCEPT;
	if (TERM(schedule_ref(HERE,&sch)))
	{
		*source = (void*)&(sch->value);
		*xstype = XS_SCHEDULE;
		ACCEPT;
	}
	else if (TERM(property_ref(HERE,xstype,source,from)))
	{	ACCEPT; }
	else
	{	REJECT; }
	DONE;
}

int GldLoader::filter_transform(PARSER, TRANSFORMSOURCE *xstype, char *sources, size_t srcsize, char *filtername, size_t namesize, OBJECT *from)
{
	char fncname[1024];
	char varlist[4096];
	START;
	if ( TERM(name(HERE,fncname,sizeof(fncname))) && (WHITE,LITERAL("(")) && (WHITE,TERM(property_list(HERE,varlist,sizeof(varlist)))) && LITERAL(")") )
	{
		if ( transform_find_filter(fncname) == NULL )
		{
			REJECT;
		}
		else if ( strlen(fncname)<namesize && strlen(varlist)<srcsize )
		{
			strcpy(filtername,fncname);
			strcpy(sources,varlist);
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"filter name/input too long");
			REJECT;
		}
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::external_transform(PARSER, TRANSFORMSOURCE *xstype, char *sources, size_t srcsize, char *functionname, size_t namesize, OBJECT *from)
{
	char fncname[1024];
	char varlist[4096];
	START;
	if ( TERM(name(HERE,fncname,sizeof(fncname))) && (WHITE,LITERAL("(")) && (WHITE,TERM(variable_list(HERE,varlist,sizeof(varlist)))) && LITERAL(")") )
	{
		if ( strlen(fncname)<namesize && strlen(varlist)<srcsize )
		{
			strcpy(functionname,fncname);
			strcpy(sources,varlist);
			ACCEPT;
			DONE
		}
	}
	REJECT;
	DONE;
}
int GldLoader::linear_transform(PARSER, TRANSFORMSOURCE *xstype, void **source, double *scale, double *bias, OBJECT *from)
{
	START;
	if WHITE ACCEPT;
	/* scale * schedule_name [+ bias]  */
	if (TERM(functional(HERE,scale)) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE, xstype, source, from))))
	{
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* scale * schedule_name [- bias]  */
	if (TERM(functional(HERE,scale)) &&( WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* schedule_name [* scale] [+ bias]  */
	if (TERM(transform_source(HERE,xstype,source,from)))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; DONE; }
	 	OR if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; DONE}
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* bias + scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		ACCEPT;
		DONE;
	}
	OR
	/* bias - scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		*scale *= -1;
		ACCEPT;
		DONE;
	}
	OR
	/* bias + schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	OR
	/* bias - schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("-")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; *scale *= -1; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	REJECT;
	DONE;
}

void GldLoader::json_free(JSONDATA **data)
{
	if ( data==NULL || *data == NULL )
		return;
	json_free(&((*data)->next));
	free((void*)(*data)->name);
	free((void*)(*data)->value);
	free((void*)(*data));
	*data = NULL;
}
bool GldLoader::json_append(JSONDATA **data, const char *name, size_t namelen, const char *value, size_t valuelen)
{
	JSONDATA *next = (JSONDATA*)malloc(sizeof(JSONDATA));
	if ( next == NULL )
	{
		output_error("json_append() memory allocation failed");
		return false;
	}
	next->next = *data;
	next->name = strndup(name,namelen);
	if ( next->name == NULL )
	{
		output_error("json_append() memory allocation failed");
		free(next);
		return false;
	}
	next->value = strndup(value,valuelen);
	if ( next->value == NULL )
	{
		output_error("json_append() memory allocation failed");
		free((void*)(next->name));
		free(next);
		return false;
	}
	*data = next;
	IN_MYCONTEXT output_debug("json_append(name='%s',value='%s')",next->name,next->value);
	return true;
}
int GldLoader::json_data(PARSER,JSONDATA **data)
{
	// this parser is for simple json "dict" data only
	// and will not accept json lists or nested data
	enum {BEGIN, OPEN, NAME, BNAME, QNAME, COLON, VALUE, BVALUE, QVALUE, CLOSE, END, ERROR} state;
	const char *name = NULL, *value = NULL;
	size_t namelen = 0, valuelen = 0;
	START;
	WHITE;
	if ( PEEK('{') )
	{
		for ( state = BEGIN ; state != END ; _m++ )
		{
			char c = *HERE;
			if ( state == BEGIN )
			{
				if ( isspace(c) ) { continue; }
				if ( c == '{' ) { state = OPEN; continue; }
				state = ERROR; break;
			}
			else if ( state == OPEN )
			{
				if ( isspace(c) ) { continue; }
				if ( c == '"' ) { name = HERE + 1; namelen = 0; state = QNAME; continue; }
				if ( c == '}' ) { state = END; continue; }
				name = HERE; namelen = 1; state = BNAME; continue;
			}
			else if ( state == BNAME )
			{
				if ( isspace(c) ) { state = COLON; continue; }
				if ( c == ':' ) {state = VALUE; continue; }
				namelen++;
				continue;
			}
			else if ( state == QNAME )
			{
				// TODO: handle escape
				if ( c == '"' ) { state = COLON; continue; }
				namelen++;
				continue;
			}
			else if ( state == COLON )
			{
				if ( isspace(c) ) { continue; }
				if ( c == ':' ) { state = VALUE; continue; }
				state = ERROR; break;
			}
			else if ( state == VALUE )
			{
				if ( isspace(c) ) { continue; }
				if ( c == '"' ) { value = HERE+1; valuelen = 0; state = QVALUE; continue; }
				state = BVALUE; value = HERE; valuelen = 1; continue;
			}
			else if ( state == BVALUE )
			{
				if ( isspace(c) ) { if ( !json_append(data,name,namelen,value,valuelen) ) break; state = CLOSE; continue; }
				if ( c == ';' || c == ',' ) { if ( !json_append(data,name,namelen,value,valuelen) ) break; state = OPEN; continue; }
				if ( c == '}' ) { if ( !json_append(data,name,namelen,value,valuelen) ) break; state = END; continue; }
				valuelen++;
				continue;
			}
			else if ( state == QVALUE )
			{
				// TODO: handle escape
				if ( c == '"') { if ( !json_append(data,name,namelen,value,valuelen) ) break; state = CLOSE; continue; }
				valuelen++;
				continue;
			}
			else if ( state == CLOSE )
			{
				if ( isspace(c) ) { continue; }
				if ( c == ';' ) { state = OPEN; continue; }
				if ( c == '}' ) { state = END; continue; }
			}
			else
			{
				// state is invalid
				break;
			}
		}
		if ( state == END )
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"JSON parse error at or near '%20s...'",HERE);
			json_free(data);
			REJECT;
		}
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::json_block(PARSER, OBJECT *obj, const char *propname)
{
	JSONDATA *data = NULL;
	START;
	if ( TERM(json_data(HERE,&data)) )
	{
		if ( object_set_json(obj,propname,data) )
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"JSON set failed");
			REJECT;
		}
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::object_properties(PARSER, CLASS *oclass, OBJECT *obj)
{
	char propname[64];
	static char propval[65536*10];
	double dval;
	complex cval;
	void *source=NULL;
	TRANSFORMSOURCE xstype = XS_UNKNOWN;
	char transformname[1024];
	char sources[4096];
	double scale=1,bias=0;
	UNIT *unit=NULL;
	OBJECT *subobj=NULL;
	START;
	if WHITE
	{
		ACCEPT;
	}
	if ( TERM(line_spec(HERE)) )
	{
		ACCEPT;
	}
	if WHITE
	{
		ACCEPT;
	}
	if ( TERM(object_block(HERE,obj,&subobj)) )
	{
		if ( WHITE,LITERAL(";") )
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"missing ; at end of nested object block",propname);
			REJECT;
		}

	}
	else if ( TERM(dotted_name(HERE,propname,sizeof(propname))) && WHITE )
	{
		LOADMETHOD *method = class_get_loadmethod(obj->oclass,propname);
		if ( method != NULL )
		{
			if ( MARKTERM(value(HERE,propval,sizeof(propval))) )
			{
				if ( method->call(obj,propval)==1 )
				{
					SAVETERM;
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"load method '%s/%s::%s' failed on value '%s'", obj->oclass->module->name,obj->oclass->name,propname,propval);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"unable to parse value for load method '%s/%s::%s'", obj->oclass->module->name,obj->oclass->name,propname);
				REJECT;
			}
		}
		else if ( MARKTERM(json_block(HERE,obj,propname)) )
		{
			SAVETERM;
			ACCEPT;
		}
		else {
			PROPERTY *prop = class_find_property(oclass,propname);
			OBJECT *subobj=NULL;
			current_object = obj; /* object context */
			current_module = obj->oclass->module; /* module context */
			char targetprop[1024];
			char targetvalue[1024];
			if ( prop!=NULL && prop->ptype == PT_object && MARKTERM(object_block(HERE,NULL,&subobj)) )
			{
				char objname[64];
				if (subobj->name) strcpy(objname,subobj->name); else snprintf(objname,sizeof(objname)-1,"%s:%d", subobj->oclass->name,subobj->id);
				if (object_set_value_by_name(obj,propname,objname))
				{
					SAVETERM;
					ACCEPT
				}
				else
				{
					syntax_error(filename,linenum,"unable to link subobject to property '%s'",propname);
					REJECT;
				}
			}
			else if ( prop == NULL && strcmp(propname,"parent")==0
					&& MARK
					&& (WHITE,LITERAL("childless")) && (WHITE,LITERAL(":"))
					&& (WHITE,TERM(name(HERE,targetprop,sizeof(targetprop))))
					&& (WHITE,LITERAL("="))
					&& (WHITE,TERM(dashed_name(HERE,targetvalue,sizeof(targetvalue)))) )
			{
				OBJECT *target;
				for ( target = object_get_first() ; target != NULL ; target = object_get_next(target) )
				{
					char value[1024];
					if ( object_get_child_count(target)==0 && !object_get_value_by_name(target,targetprop,value,sizeof(value)) && strcmp(value,targetvalue)==0 )
					{
						object_set_parent(obj,target);
						break;
					}
				}
				if ( obj==NULL )
				{
					syntax_error(filename,linenum,"no childless objects found in %s=%s (immediate)", targetprop, targetvalue);
					REJECT;
				}
				else
				{
					SAVETERM;
					ACCEPT;
				}
			}
			else if ( prop != NULL && LITERAL("inherit") )
			{
				char value[1024];
				if ( obj->parent==NULL )
				{
					syntax_error(filename,linenum,"cannot inherit from an parent that hasn't been resolved yet or isn't specified");
					REJECT;
				}
				else if ( ! object_get_value_by_name(obj->parent,propname,value,sizeof(value)) )
				{
					syntax_error(filename,linenum,"unable to get value of inherit property '%s'", propname);
					REJECT;
				}
				if ( object_set_value_by_name(obj,propname,value)<=0 )
				{
					syntax_error(filename,linenum,"unable to set value of inherit property '%s'", propname);
					REJECT;
				}
				// DPC 3/28/20: is SAVE/ACCEPT needed here?
			}
			else if ( prop != NULL && prop->ptype == PT_complex
				&& MARK && TERM(complex_unit(HERE,&cval,&unit)))
			{
				if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_complex(unit,prop->unit,&cval)==0)
				{
					syntax_error(filename,linenum,"units of value are incompatible with units of property, cannot convert from %s to %s", unit->name,prop->unit->name);
					REJECT;
				}
				else if (object_set_complex_by_name(obj,propname,cval)==0)
				{
					syntax_error(filename,linenum,"complex property %s of %s %s could not be set to complex value '%g%+gi'", propname, format_object(obj).c_str(), cval.Re(), cval.Im());
					REJECT;
				}
				else
				{
					SAVETERM;
					ACCEPT;
				}
			}
			else if ( prop != NULL && prop->ptype == PT_double
				&& MARK && TERM(expression(HERE, &dval, &unit, obj)))
			{
				if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit,prop->unit,&dval)==0)
				{
					syntax_error(filename,linenum,"units of value are incompatible with units of property, cannot convert from %s to %s", unit->name,prop->unit->name);
					REJECT;
				}
				else if (object_set_double_by_name(obj,propname,dval)==0)
				{
					syntax_error(filename,linenum,"double property %s of %s %s could not be set to expression evaluating to '%g'", propname, format_object(obj).c_str(), dval);
					REJECT;
				}
				else
				{
					SAVETERM;
					ACCEPT;
				}
			}
			else if ( prop != NULL && prop->ptype == PT_bool
				&& MARK && TERM(expression(HERE, &dval, &unit, obj)))
			{
				if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit,prop->unit,&dval)==0)
				{
					syntax_error(filename,linenum,"units of value are incompatible with units of property, cannot convert from %s to %s", unit->name,prop->unit->name);
					REJECT;
				}
				else if (object_set_value_by_name(obj,propname,dval>0?"TRUE":"FALSE")==0)
				{
					syntax_error(filename,linenum,"double property %s of %s %s could not be set to expression evaluating to '%g'", propname, format_object(obj).c_str(), dval);
					REJECT;
				}
				else
				{
					SAVETERM;
					ACCEPT;
				}
			}
			else if ( prop != NULL && prop->ptype == PT_double
				&& MARK && TERM(functional_unit(HERE,&dval,&unit)) )
			{
				if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit,prop->unit,&dval)==0)
				{
					syntax_error(filename,linenum,"units of value are incompatible with units of property, cannot convert from %s to %s", unit->name,prop->unit->name);
					REJECT;
				}
				else if (object_set_double_by_name(obj,propname,dval)==0)
				{
					syntax_error(filename,linenum,"double property %s of %s %s could not be set to double value '%g' having unit '%s'", propname, format_object(obj).c_str(), dval, unit->name);
					REJECT;
				}
				else
				{
					SAVETERM;
					ACCEPT;
				}
			}
			else if ( prop != NULL && is_int(prop->ptype)
				&& MARK && TERM(functional_unit(HERE, &dval, &unit)) )
			{
				int64 ival = 0;
				int16 ival16 = 0;
				int32 ival32 = 0;
				int64 ival64 = 0;
				int rv = 0;

				if ( unit != NULL && prop->unit != NULL && strcmp((char *)(unit), "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0 )
				{
					syntax_error(filename,linenum,"units of value are incompatible with units of property, cannot convert from %s to %s", unit->name,prop->unit->name);
					REJECT;
				}
				else
				{
					switch ( prop->ptype ) {
						case PT_int16:
							ival = ival16 = (int16)dval;
							rv = object_set_int16_by_name(obj, propname, ival16);
							break;
						case PT_int32:
							ival = ival32 = (int32)dval;
							rv = object_set_int32_by_name(obj, propname, ival32);
							break;
						case PT_int64:
							ival = ival64 = (int64)dval;
							rv = object_set_int64_by_name(obj, propname, ival64);
							break;
						default:
							output_error("function_int operating on a non-integer (we shouldn't be here)");
							REJECT;
					} /* end switch */
					if ( rv == 0 )
					{
						syntax_error(filename,linenum,"int property %s of %s %s could not be set to integer '%lld'", propname, format_object(obj).c_str(), ival);
						REJECT;
					}
					else
					{
						SAVETERM;
						ACCEPT;
					}
				} /* end unit_convert_ex else */
			}
			else if (prop!=NULL
				&& ( ( prop->ptype>=PT_double && prop->ptype<=PT_int64 ) || ( prop->ptype>=PT_bool && prop->ptype<=PT_timestamp ) || ( prop->ptype>=PT_float && prop->ptype<=PT_enduse ) )
				&& MARK && TERM(linear_transform(HERE, &xstype, &source,&scale,&bias,obj)))
			{
				void *target = (void*)((char*)(obj+1) + (int64)prop->addr);

				/* add the transform list */
				if (!transform_add_linear(xstype,(double*)source,target,scale,bias,obj,prop,(xstype == XS_SCHEDULE ? (SCHEDULE*)source : 0)))
				{
					syntax_error(filename,linenum,"schedule transform could not be created - %s", errno?strerror(errno):"(no details)");
					REJECT;
				}
				else if ( source!=NULL )
				{
					/* a transform is unresolved */
					if (first_unresolved==source)

						/* source was the unresolved entry, for now it will be the transform itself */
						first_unresolved->ref = (void*)transform_getnext(NULL);

					SAVETERM;
					ACCEPT;
				}
			}
			else if (prop!=NULL && prop->ptype==PT_double
				&& MARK && TERM(filter_transform(HERE, &xstype, sources, sizeof(sources), transformname, sizeof(transformname), obj)))
			{
				// TODO handle more than one source
				char sobj[64], sprop[64];

				int n = sscanf(sources,"%[^:,]:%[^,]",sobj,sprop);
				OBJECT *source_obj;
				PROPERTY *source_prop;

				/* get source object */
				source_obj = (n==1||strcmp(sobj,"this")==0) ? obj : object_find_name(sobj);
				if ( !source_obj )
				{
					syntax_error(filename,linenum,"filter source object '%s' not found", n==1?"this":sobj);
					REJECT;
					DONE;
				}

				/* get source property */
				source_prop = object_get_property(source_obj, n==1?sobj:sprop,NULL);
				if ( !source_prop )
				{
					syntax_error(filename,linenum,"filter source property '%s' of object '%s' not found", n==1?sobj:sprop, n==1?"this":sobj);
					REJECT;
					DONE;
				}

				/* add to external transform list */
				if ( !transform_add_filter(obj,prop,transformname,source_obj,source_prop) )
				{
					syntax_error(filename,linenum,"filter transform could not be created - %s", errno?strerror(errno):"(no details)");
					REJECT;
					DONE;
				}
				else if ( source!=NULL )
				{
					/* a transform is unresolved */
					if (first_unresolved==source)

						/* source was the unresolved entry, for now it will be the transform itself */
						first_unresolved->ref = (void*)transform_getnext(NULL);

					SAVETERM;
					ACCEPT;
				}
			}
			else if (prop!=NULL && prop->ptype==PT_double
				&& MARK && TERM(external_transform(HERE, &xstype, sources, sizeof(sources), transformname, sizeof(transformname), obj)))
			{
				// TODO handle more than one source
				char sobj[64], sprop[64];
				int n = sscanf(sources,"%[^:,]:%[^,]",sobj,sprop);
				OBJECT *source_obj;
				PROPERTY *source_prop;

				/* get source object */
				source_obj = (n==1||strcmp(sobj,"this")==0) ? obj : object_find_name(sobj);
				if ( !source_obj )
				{
					syntax_error(filename,linenum,"transform source object '%s' not found", n==1?"this":sobj);
					REJECT;
					DONE;
				}

				/* get source property */
				source_prop = object_get_property(source_obj, n==1?sobj:sprop,NULL);
				if ( !source_prop )
				{
					syntax_error(filename,linenum,"transform source property '%s' of object '%s' not found", n==1?sobj:sprop, n==1?"this":sobj);
					REJECT;
					DONE;
				}

				/* add to external transform list */
				if ( !transform_add_external(obj,prop,transformname,source_obj,source_prop) )
				{
					syntax_error(filename,linenum,"external transform could not be created - %s", errno?strerror(errno):"(no details)");
					REJECT;
					DONE;
				}
				else if ( source!=NULL )
				{
					/* a transform is unresolved */
					if (first_unresolved==source)

						/* source was the unresolved entry, for now it will be the transform itself */
						first_unresolved->ref = (void*)transform_getnext(NULL);

					SAVETERM;
					ACCEPT;
				}
			}
			else if ( MARKTERM(alternate_value(HERE,propval,sizeof(propval))) )
			{
				if (prop==NULL)
				{
					/* check for special properties */
					if (strcmp(propname,"root")==0)
						obj->parent = NULL;
					else if (strcmp(propname,"parent")==0)
					{
						if (add_unresolved(obj,PT_object,(void*)&obj->parent,oclass,propval,filename,linenum,UR_RANKS)==NULL)
						{
							syntax_error(filename,linenum,"unable to add unresolved reference to parent %s", propval);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if (strcmp(propname,"rank")==0)
					{
						if ((obj->rank = atoi(propval))<0)
						{
							syntax_error(filename,linenum,"unable to set rank to %s", propval);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if (strcmp(propname,"clock")==0)
					{
						obj->clock = atoi64(propval); // @todo convert_to_timestamp should be used
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"valid_to")==0)
					{
						obj->valid_to = atoi64(propval); // @todo convert_to_timestamp should be used
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"schedule_skew")==0)
					{
						obj->schedule_skew = atoi64(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"latitude")==0)
					{
						obj->latitude = load_latitude(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"longitude")==0)
					{
						obj->longitude = load_longitude(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"in")==0 || strcmp(propname,"in_svc")==0)
					{
						obj->in_svc = convert_to_timestamp_delta(propval,&obj->in_svc_micro,&obj->in_svc_double);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"out")==0 || strcmp(propname,"out_svc")==0)
					{
						obj->out_svc = convert_to_timestamp_delta(propval,&obj->out_svc_micro,&obj->out_svc_double);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_init")==0 )
					{
						obj->events.init = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_precommit")==0 )
					{
						obj->events.precommit = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_presync")==0 )
					{
						obj->events.presync = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_sync")==0 )
					{
						obj->events.sync = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_postsync")==0 )
					{
						obj->events.postsync = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_commit")==0 )
					{
						obj->events.commit = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"on_finalize")==0 )
					{
						obj->events.finalize = strdup(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"name")==0)
					{
						if (object_set_name(obj,propval)==NULL)
						{
							syntax_error(filename,linenum,"property name %s could not be used", propval);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if ( strcmp(propname,"heartbeat")==0 )
					{
						obj->heartbeat = convert_to_timestamp(propval);
						SAVETERM;
						ACCEPT;
					}
					else if (strcmp(propname,"groupid")==0)
					{
						memcpy(obj->groupid, propval, sizeof(obj->groupid));
					}
					else if (strcmp(propname,"flags")==0)
					{
						if(set_flags(obj,propval) == 0)
						{
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if (strcmp(propname,"library")==0)
					{
						output_warning("%s(%d): libraries not yet supported", filename, linenum);
						/* TROUBLESHOOT
							An attempt to use the <b>library</b> GLM directive was made.  Library directives
							are not supported yet.
						 */
						REJECT;
					}
					else if ( strcmp(propname,"module")==0 )
					{
						if ( strcmp(propval,obj->oclass->module->name)!=0 )
						{
							syntax_error(filename,linenum,"module '%s' does not match module of class '%s.%s'",propval,obj->oclass->module->name,obj->oclass->name);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if ( strcmp(propname,"guid")==0 )
					{
						if ( sscanf(propval,"%08llX%08llX",obj->guid,obj->guid+1) != 2 )
						{
							syntax_error(filename,linenum,"guid '%s' is not valid",propval);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else if ( strcmp(propname,"rng_state") == 0 )
					{
						if ( sscanf(propval,"%d",&obj->rng_state) != 1 )
						{
							syntax_error(filename,linenum,"rng_state '%s' is not valid",propval);
							REJECT;
						}
						else
						{
							SAVETERM;
							ACCEPT;
						}
					}
					else
					{
						syntax_error(filename,linenum,"property %s is not defined in class %s", propname, oclass->name);
						REJECT;
					}
				}
				else if (prop->ptype==PT_object)
				{	void *addr = object_get_addr(obj,propname);
					if (addr==NULL)
					{
						syntax_error(filename,linenum,"unable to get %s member %s", format_object(obj).c_str(), propname);
						REJECT;
					}
					else
					{
						add_unresolved(obj,PT_object,addr,oclass,propval,filename,linenum,UR_NONE);
						SAVETERM;
						ACCEPT;
					}
				}
				else if ( prop->ptype >= PT_char8 && prop->ptype <= PT_char1024 )
				{
					int len = object_set_value_by_name(obj,propname,propval);
					if ( len < (int)strlen(propval) )
					{
						syntax_error(filename,linenum,"property %s of %s could not be set to value '%s' (only %d bytes read)", propname, format_object(obj).c_str(), propval, len);
						REJECT;
					}
					else
					{
						SAVETERM;
						ACCEPT;
					}
				}
				else if ( object_set_value_by_name(obj,propname,propval) == 0 )
				{
					syntax_error(filename,linenum,"property %s of %s could not be set to value '%s'", propname, format_object(obj).c_str(), propval);
					REJECT;
				}
				else
				{
					ACCEPT; // @todo shouldn't this be REJECT?
				}
			}
		}
		if WHITE ACCEPT;
		if LITERAL(";") {ACCEPT;}
		else
		{
			syntax_error(filename,linenum,"expected ';' at end of property specification");
			REJECT;
		}
	}
	else if LITERAL("}") {/* don't accept yet */ DONE;}
	else { syntax_error_here(HERE); REJECT; }

	/* may be repeated */
	if TERM(object_properties(HERE,oclass,obj))
	{
		ACCEPT;
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::object_name_id(PARSER,char *classname, int64 *id)
{
	START;
	if WHITE ACCEPT;
	if TERM(dotted_name(HERE,classname,MAXCLASSNAMELEN))
	{
		*id = -1; /* anonymous object */
		if LITERAL(":")
		{
			TERM(integer(HERE,id));
		}
		ACCEPT;
		DONE;
	}
	else if TERM(name(HERE,classname,MAXCLASSNAMELEN))
	{
		*id = -1; /* anonymous object */
		if LITERAL(":")
		{
			TERM(integer(HERE,id));
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

int GldLoader::object_name_id_range(PARSER,char *classname, int64 *from, int64 *to)
{
	START;
	if WHITE ACCEPT;
	if (TERM(dotted_name(HERE,classname,MAXCLASSNAMELEN)) && LITERAL(":") && TERM(integer(HERE,from)) && LITERAL("..")) ACCEPT
	else if (TERM(name(HERE,classname,MAXCLASSNAMELEN)) && LITERAL(":") && TERM(integer(HERE,from)) && LITERAL("..")) ACCEPT
	else REJECT;
	if (TERM(integer(HERE,to))) ACCEPT else
	{
		syntax_error(filename,linenum,"expected id range end value");
		REJECT;
	}
	DONE;
}

int GldLoader::object_name_id_count(PARSER,char *classname, int64 *count)
{
	START;
	if WHITE ACCEPT;
	if (TERM(dotted_name(HERE,classname,MAXCLASSNAMELEN)) && LITERAL(":") && LITERAL("..")) ACCEPT
	else if (TERM(name(HERE,classname,MAXCLASSNAMELEN)) && LITERAL(":") && LITERAL("..")) ACCEPT
	else REJECT;
	if (TERM(integer(HERE,count))) ACCEPT else
	{
		syntax_error(filename,linenum,"expected id count");
		REJECT;
	}
	DONE;
}

int GldLoader::object_block(PARSER, OBJECT *parent, OBJECT **subobj)
{
#define NAMEOBJ  /* DPC: not sure what this does, but it doesn't seem to be harmful */
#ifdef NAMEOBJ
	static OBJECT nameobj;
#endif
	FULLNAME space;
	char classname[MAXCLASSNAMELEN+1];
	CLASS *oclass;
	OBJECT *obj=NULL;
	int64 id=-1, id2=-1;
	START;

	// @TODO push context here

	if WHITE ACCEPT;
	if (LITERAL("namespace") && (WHITE,TERM(name(HERE,space,sizeof(space)))) && (WHITE,LITERAL("{")))
	{
		if (!object_open_namespace(space))
		{
			syntax_error(filename,linenum,"namespace %s could not be opened", space);
			REJECT;
		}

		while (TERM(object_block(HERE,parent,subobj))) {LITERAL(";");}
		while (WHITE);
		if (LITERAL("}"))
		{	object_close_namespace();
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"namespace %s missing closing }", space);
			REJECT;
		}
	}

	if WHITE ACCEPT;
	if (LITERAL("object") && WHITE) ACCEPT else REJECT /* enforced whitespace */

	/* objects should not be started until all deferred schedules are done */
	if ( global_threadcount>1 )
	{
		if ( schedule_createwait()==FAILED )
		{
			syntax_error(filename,linenum,"object create cannot proceed when a schedule error persists");
			REJECT;
		}
	}

	//if WHITE ACCEPT;
	if TERM(object_name_id_range(HERE,classname,&id,&id2))
	{
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL)
		{
			syntax_error(filename,linenum,"class '%s' is not known", classname);
			REJECT;
		}
		id2++;
		ACCEPT;
	}
	else if TERM(object_name_id_count(HERE,classname,&id2))
	{
		id=-1; id2--;  /* count down to zero inclusive */
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL)
		{
			syntax_error(filename,linenum,"class '%s' is not known", classname);
			REJECT;
		}
		ACCEPT;
	}
	else if TERM(object_name_id(HERE,classname,&id))
	{
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL)
		{
			syntax_error(filename,linenum,"class '%s' is not known", classname);
			REJECT;
		}
		CLASS *aclass = class_get_class_from_classname(classname,oclass);
		if ( aclass != NULL )
		{
			syntax_error(filename,linenum,"class '%s' module reference is ambiguous (using '%s' instead of '%s')", classname,oclass->module ? oclass->module->name : "runtime", aclass->module ? aclass->module->name : "runtime");
		}
		ACCEPT;
	}
	else
	{
		syntax_error(filename,linenum,"expected object id or range");
		REJECT;
	}
	if WHITE ACCEPT;
	if ( LITERAL("{") )
	{
		ACCEPT
	}
	else
	{
		syntax_error(filename,linenum,"expected object block starting {");
		REJECT;
	}

	/* id(s) is/are valid */
#ifdef NAMEOBJ
	nameobj.name = classname;
#endif
	if ( id2 == -1 ) id2=id+1; /* create singleton */
	BEGIN_REPEAT;
	while ( id < id2 )
	{
		REPEAT;
		if ( oclass->create != NULL )
		{
#ifdef NAMEOBJ
			obj = &nameobj;
#endif
			if ( (*oclass->create)(&obj,parent) == 0 )
			{
				syntax_error(filename,linenum,"create failed for object %s:%d", classname, id);
				REJECT;
			}
			else if ( obj == NULL
#ifdef NAMEOBJ
				|| obj == &nameobj
#endif
				)
			{
				syntax_error(filename,linenum,"create failed name object %s:%d", classname, id);
				REJECT;
			}
		}
		else // need to create object here because class has no create function
		{
			obj = object_create_single(oclass);
			if ( obj==NULL )
			{
				syntax_error(filename,linenum,"create failed for object %s:%d", classname, id);
				REJECT;
			}
			object_set_parent(obj,parent);
		}
		if ( id != -1 && load_set_index(obj,(OBJECTNUM)id) == FAILED )
		{
			syntax_error(filename,linenum,"unable to index object id number for %s:%d", classname, id);
			REJECT;
		}
		else if ( TERM(object_properties(HERE,oclass,obj)) )
		{
			ACCEPT;
		}
		else
		{
			REJECT;
		}
		if (id==-1) id2--; else id++;
	}
	END_REPEAT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT else
	{
		syntax_error(filename,linenum,"expected object block closing }");
		REJECT;
	}
	if (subobj) *subobj = obj;

	// @TODO pop context here

	DONE;
}

int GldLoader::import(PARSER)
{
	char modname[32];
	char fname[1024];
	START;
	if WHITE ACCEPT;
	if (LITERAL("import") && WHITE) /* enforced whitespace */
	{
		if (TERM(name(HERE,modname,sizeof(modname))) && WHITE) /* enforced whitespace */
		{
			current_object = NULL; /* object context */
			current_module = NULL; /* module context */
			if TERM(alternate_value(HERE,fname,sizeof(fname)))
			{
				if LITERAL(";")
				{
					int result;
					MODULE *module = module_find(modname);
					if (module==NULL)
					{
						syntax_error(filename,linenum,"module %s not loaded", modname);
						REJECT;
					}
					result = module_import(module,fname);
					if (result < 0)
					{
						syntax_error(filename,linenum,"%d errors loading importing %s into %s module", -result, fname, modname);
						REJECT;
					}
					else if (result==0)
					{
						syntax_error(filename,linenum,"module %s load of %s failed; %s", modname, fname, errno?strerror(errno):"(no details)");
						REJECT;
					}
					else
					{
						IN_MYCONTEXT output_verbose("%d objects loaded to %s from %s", result, modname, fname);
						ACCEPT;
					}
				}
				else
				{
					syntax_error(filename,linenum,"expected ; after module %s import from %s statement", modname, fname);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"expected filename after module %s import statement", modname);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"expected module name after import statement");
			REJECT;
		}
	}
	DONE
}

int GldLoader::export_model(PARSER)
{
	char modname[32];
	char fname[1024];
	START;
	if WHITE ACCEPT;
	if (LITERAL("export") && WHITE) /* enforced whitespace */
	{
		if (TERM(name(HERE,modname,sizeof(modname))) && WHITE) /* enforced whitespace */
		{
			current_object = NULL; /* object context */
			current_module = NULL; /* module context */
			if TERM(alternate_value(HERE,fname,sizeof(fname)))
			{
				if LITERAL(";")
				{
					int result;
					MODULE *module = module_find(modname);
					if (module==NULL)
					{
						syntax_error(filename,linenum,"module %s not loaded", modname);
						REJECT;
					}
					if ( !load_resolve_all() )
						syntax_error(filename,linenum,"module export encountered before all object names were resolved", modname);
					result = module_export(module,fname);
					if (result < 0)
					{
						syntax_error(filename,linenum,"%d errors export %s from %s module", -result, fname, modname);
						REJECT;
					}
					else if (result==0)
					{
						syntax_error(filename,linenum,"module %s export of %s failed; %s", modname, fname, errno?strerror(errno):"(no details)");
						REJECT;
					}
					else
					{
						IN_MYCONTEXT output_verbose("%d objects export from %s to %s", result, modname, fname);
						ACCEPT;
					}
				}
				else
				{
					syntax_error(filename,linenum,"expected ; after module %s export from %s statement", modname, fname);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"expected export specification after module %s export statement", modname);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"expected module name after import statement");
			REJECT;
		}
	}
	DONE
}

int GldLoader::library(PARSER)
{
	START;
	if WHITE ACCEPT;
	if (LITERAL("library") && WHITE) /* enforced whitespace */
	{
		char libname[1024];
		if ( TERM(dotted_name(HERE,libname,sizeof(libname))) && (WHITE,LITERAL(";")))
		{
			output_warning("%s(%d): libraries not yet supported", filename, linenum);
			/* TROUBLESHOOT
				An attempt to parse a <b>library</b> GLM directive was made.  Library directives
				are not supported yet.
			 */
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"library syntax error");
			REJECT;
		}
	}
	REJECT;
}

int GldLoader::schedule(PARSER)
{
	int startline = linenum;
	char schedname[64];
	START;
	if WHITE ACCEPT;
	if (LITERAL("schedule") && WHITE && TERM(name(HERE,schedname,sizeof(schedname))) && (WHITE,LITERAL("{")))
	{
		char buffer[65536], *p=buffer;
		int nest=0;
		for (nest=0; nest>=0; _m++)
		{
			char c = *HERE;
			if (c=='\0') break;
			switch (c) {
			case '{': nest++; *p++ = c; break;
			case '}': if (nest-->0) *p++ = c; break;
			case '\n': *p++ = c; ++linenum; break;
			//case '\r': *p++ = c; ++linenum; break;
			default: *p++ = c; break;
			}
			*p = '\0';
		}
		SCHEDULE *sch = schedule_create(schedname, buffer);
		if ( sch != NULL )
		{
			sch->flags |= SN_USERDEFINED;
			ACCEPT;
		}
		else
		{
			syntax_error(filename,startline,"schedule '%s' is not valid", schedname);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

int GldLoader::linkage_term(PARSER,::instance *inst)
{
	int startline = linenum;
	char fromobj[64];
	char fromvar[64];
	char toobj[64];
	char tovar[64];
	START;
	if WHITE ACCEPT;
	if ( TERM(name(HERE,fromobj,sizeof(fromobj))) && LITERAL(":") && TERM(name(HERE,fromvar,sizeof(fromvar)))
		&& (WHITE,LITERAL("->")) && (WHITE,TERM(name(HERE,toobj,sizeof(toobj)))) && LITERAL(":") && TERM(name(HERE,tovar,sizeof(tovar)))
		&& LITERAL(";"))
	{
		if ( linkage_create_writer(inst,fromobj,fromvar,toobj,tovar) ) ACCEPT
		else {
			syntax_error(filename,startline,"linkage to write '%s:%s' to '%s:%s' is not valid", fromobj, fromvar, toobj, tovar);
			REJECT;
		}
		DONE;
	}
	OR if ( TERM(name(HERE,toobj,sizeof(toobj))) && LITERAL(":") && TERM(name(HERE,tovar,sizeof(tovar)))
		&& (WHITE,LITERAL("<-")) && (WHITE,TERM(name(HERE,fromobj,sizeof(fromobj)))) && LITERAL(":") && TERM(name(HERE,fromvar,sizeof(fromvar)))
		&& LITERAL(";"))
	{
		if ( linkage_create_reader(inst,fromobj,fromvar,toobj,tovar) ) ACCEPT
		else {
			syntax_error(filename,startline,"linkage to read '%s:%s' from '%s:%s' is not valid", fromobj, fromvar, toobj, tovar);
			REJECT;
		}
		DONE;
	}
	OR if ( LITERAL("model") && WHITE && TERM(value(HERE,inst->model,sizeof(inst->model))) && (WHITE,LITERAL(";")))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("cacheid") && WHITE && TERM(integer(HERE,(long long*)&(inst->cacheid))) && (WHITE,LITERAL(";")))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("mode") && WHITE && TERM(value(HERE,inst->cnxtypestr,sizeof(inst->cnxtypestr))) && (WHITE, LITERAL(";")))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("execdir") && WHITE && TERM(value(HERE,inst->execdir,sizeof(inst->execdir))) && (WHITE, LITERAL(";")))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("return_port") && WHITE && TERM(integer16(HERE,(short*)&(inst->return_port))) && (WHITE, LITERAL(";")))
	{
		IN_MYCONTEXT output_debug("linkage_term(): return_port = %d", inst->return_port);
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("}") )
	{
		REJECT;
	}
	OR
	{
		syntax_error(filename,startline,"unrecognized instance term at or after '%.10s...'", HERE);
		REJECT;
	}
	DONE;

}
int GldLoader::instance_block(PARSER)
{
	int startline = linenum;
	char instance_host[256];
	START;
	if WHITE ACCEPT;
	if ( LITERAL("instance") && WHITE && TERM(hostname(HERE,instance_host,sizeof(instance_host))) && (WHITE,LITERAL("{")))
	{
		::instance *inst = instance_create(instance_host);
		if ( !inst )
		{
			syntax_error(filename,startline,"unable to define an instance on %s", instance_host);
			REJECT;
			DONE;
		}
		ACCEPT;
		while ( TERM(linkage_term(HERE,inst)) ) ACCEPT;
		if ( (WHITE,LITERAL("}")) ) { ACCEPT; DONE }
		else REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::gnuplot(PARSER, GUIENTITY *entity)
{
	char *p = entity->gnuplot;
	int _n = 0;
	while ( _p[_n]!='}' )
	{
		if (_p[_n]=='\n') linenum++;
		*p++ = _p[_n++];
		if ( p>entity->gnuplot+sizeof(entity->gnuplot) )
		{
			syntax_error(filename,linenum,"gnuplot script too long");
			return _n;
		}
	}
	return _n;
}

int GldLoader::gui_entity_parameter(PARSER, GUIENTITY *entity)
{
	char buffer[1024];
	char varname[64];
	char modname[64];
	char objname[64];
	char propname[64];
	START;
	if WHITE ACCEPT;
	if LITERAL("global")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(name(HERE,modname,sizeof(modname))) && LITERAL("::") && TERM(name(HERE,varname,sizeof(varname))) && (WHITE,LITERAL(";")))
		{
			char fullname[256];
			ACCEPT;
			snprintf(fullname,sizeof(fullname)-1,"%s::%s",modname,varname);
			gui_set_variablename(entity,fullname);
			DONE;
		}
		OR if (TERM(name(HERE,varname,sizeof(varname))) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			gui_set_variablename(entity,varname);
			if (gui_get_variable(entity) || gui_get_environment(entity))
			{
				DONE;
			}
			else
			{
				syntax_error(filename,linenum,"invalid gui global variable '%s'",varname);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui global variable specification");
			REJECT;
		}
	}
	OR if LITERAL("link")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(name(HERE,objname,sizeof(objname))) && LITERAL(":") && TERM(name(HERE,propname,sizeof(propname))) && (WHITE,LITERAL(";")))
		{
			gui_set_objectname(entity,objname);
			gui_set_propertyname(entity,propname);
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui link object:property specification");
			REJECT;
		}
	}
	OR if LITERAL("value")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && (WHITE,LITERAL(";")))
		{
			gui_set_value(entity,buffer);
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui value specification");
			REJECT;
		}
	}
	OR if LITERAL("source")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && (WHITE,LITERAL(";")))
		{
			gui_set_source(entity,buffer);
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui value specification");
			REJECT;
		}
	}
	OR if LITERAL("options")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && (WHITE,LITERAL(";")))
		{
			gui_set_options(entity,buffer);
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui options specification");
			REJECT;
		}
	}	OR if LITERAL("unit")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && (WHITE,LITERAL(";")))
		{
			gui_set_unit(entity,buffer);
			if (gui_get_unit(entity))
			{
				ACCEPT;
				DONE;
			}
			else
			{
				syntax_error(filename,linenum,"invalid gui unit '%s'", buffer);
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui unit specification");
			REJECT;
		}
		ACCEPT;
		DONE;
	}
	OR if LITERAL("size")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->size)) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui size specification");
			REJECT;
		}
		ACCEPT;
		DONE;
	}
	OR if LITERAL("height")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->height)) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui height specification");
			REJECT;
		}
		ACCEPT;
		DONE;
	}
	OR if LITERAL("width")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->width)) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			DONE;
		}
		else
		{
			syntax_error(filename,linenum,"invalid gui width specification");
			REJECT;
		}
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("gnuplot") && (WHITE,LITERAL("{")) )
	{
		ACCEPT;
		if ( TERM(gnuplot(HERE,entity)) )
		{
			ACCEPT;
			if ( LITERAL("}") )
			{
				ACCEPT;
				DONE;
			}
			else
			{
				syntax_error(filename,linenum,"missing closing } after gnuplot script");
				REJECT;
			}
		}
		else
		{
			syntax_error(filename,linenum,"invalid gnuplot script");
			REJECT;
		}
	}
	OR if ( LITERAL("wait") && (WHITE,TERM(value(HERE,entity->wait_for,sizeof(entity->wait_for)))) && (WHITE,LITERAL(";"))  )
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("hold") && (WHITE,LITERAL(";")) )
	{
		ACCEPT;
		entity->hold = 1;
		DONE;
	}
	REJECT;
}

int GldLoader::gui_entity_action(PARSER, GUIENTITY *parent)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("action")
	{
		GUIENTITY *entity = gui_create_entity();
		gui_set_type(entity,GUI_ACTION);
		gui_set_srcref(entity,filename,linenum);
		entity->parent = parent;
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,entity->action,sizeof(entity->action))) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			DONE;
		}
		else {
			syntax_error(filename,linenum,"invalid gui action specification");
			REJECT;
		}
	}
	REJECT;
}

int GldLoader::gui_entity_type(PARSER, GUIENTITYTYPE *type)
{
	START;
	if WHITE ACCEPT;
	// labeling entities
	if LITERAL("title") { ACCEPT; *type = GUI_TITLE; DONE; };
	if LITERAL("status") { ACCEPT; *type = GUI_STATUS; DONE; };
	if LITERAL("text") { ACCEPT; *type = GUI_TEXT; DONE; };
	// input entities
	if LITERAL("input") { ACCEPT; *type = GUI_INPUT; DONE; };
	if LITERAL("check") { ACCEPT; *type = GUI_CHECK; DONE; };
	if LITERAL("radio") { ACCEPT; *type = GUI_RADIO; DONE; };
	if LITERAL("select") { ACCEPT; *type = GUI_SELECT; DONE; };
	// output entities
	if LITERAL("browse") { ACCEPT; *type = GUI_BROWSE; DONE; };
	if LITERAL("table") { ACCEPT; *type = GUI_TABLE; DONE; };
	if LITERAL("graph") { ACCEPT; *type = GUI_GRAPH; DONE; };
	// grouping entities
	if LITERAL("row") { ACCEPT; *type = GUI_ROW; DONE; };
	if LITERAL("tab") { ACCEPT; *type = GUI_TAB; DONE; }; // beware not to put this before "table"
	if LITERAL("page") { ACCEPT; *type = GUI_PAGE; DONE; };
	if LITERAL("group") { ACCEPT; *type = GUI_GROUP; DONE; };
	if LITERAL("span") { ACCEPT; *type = GUI_SPAN; DONE; };
	REJECT;
}

int GldLoader::gui_entity(PARSER, GUIENTITY *parent)
{
	//char buffer[1024];
	GUIENTITYTYPE type;
	START;
	if WHITE ACCEPT;
	if TERM(gui_entity_type(HERE,&type))
	{
		GUIENTITY *entity = gui_create_entity();
		gui_set_type(entity,type);
		gui_set_srcref(entity,filename,linenum);
		gui_set_parent(entity,parent);
		if WHITE ACCEPT;
		if LITERAL("{")
		{
			ACCEPT;
			while (true) {
				if WHITE ACCEPT;
				if (gui_is_grouping(entity) && TERM(gui_entity(HERE,entity)))
				{
					ACCEPT;
					continue;
				}
				if (TERM(gui_entity_parameter(HERE,entity))) { ACCEPT; continue; }
				if (TERM(gui_entity_action(HERE,entity))) { ACCEPT; continue; }
				if LITERAL("}") { ACCEPT; break; }
				syntax_error(filename,linenum,"unknown gui entity");
				REJECT;
			}
			DONE;
		}
		if TERM(gui_entity_parameter(HERE,entity)) { ACCEPT; DONE; }
		if (TERM(gui_entity_action(HERE,entity))) { ACCEPT; DONE; }
		if ( LITERAL(";" ) ) { ACCEPT; DONE; }
		REJECT;
	}
	REJECT;
}

int GldLoader::gui(PARSER)
{
	START;
	if WHITE ACCEPT;
	if (LITERAL("gui") && (WHITE,LITERAL("{")))
	{
		while TERM(gui_entity(HERE,NULL)) ACCEPT;
		if (WHITE,LITERAL("}"))
		{
			if (gui_wait()==0)
			{
				syntax_error(filename,linenum,"quit requested by user");
				REJECT;
			}
			ACCEPT;
		}
	}
	else REJECT;
	DONE;
}

int GldLoader::C_code_block(PARSER, char *buffer, int size)
{
	int n_curly = 0;
	int in_quotes = 0;
	int in_quote = 0;
	int in_comment = 0;
	int in_linecomment = 0;
	char *d = buffer;
	START;
	do
	{
		char c = *_p;
		int skip=0;
		int ignore_curly = in_quotes || in_quote || in_comment || in_linecomment;
		switch (c) {
		case '{': if (!ignore_curly) n_curly++; break;
		case '}': if (!ignore_curly) n_curly--; break;
		case '/': if (_p[1]=='*') skip=1, in_comment=1; else if (_p[1]=='/') skip=1, in_linecomment=1; break;
		case '*': if (_p[1]=='/' && in_comment) skip=1, in_comment=0; break;
		case '\n': in_linecomment=0; linenum++; break;
		default: break;
		}
		*d++ = *_p;
		if (skip) _n++,*d++=*++_p;
	} while ( *++_p!='\0' && _n++<size && n_curly>=0 );
	*--d='\0'; _n--; // don't include the last curly
	DONE;
}

int GldLoader::filter_name(PARSER, char *result, int size)
{
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while ((size>1 && isalpha(*_p)) || isdigit(*_p) || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}
int GldLoader::double_timestep(PARSER,double *step)
{
	START;
	if ( WHITE,TERM(real_value(HERE,step)) )
	{
		UNIT *from = NULL;
		if ( WHITE,TERM(unitspec(HERE,&from)) )
		{
			// convert to seconds
			UNIT *to = unit_find("s");
			if ( unit_convert_ex(from,to,step)==0)
			{
				REJECT;
			}
			else
			{
				ACCEPT;
			}
		}
		else
		{
			ACCEPT;
		}
	}
	else
	{
		REJECT;
	}
	DONE;
}
int GldLoader::filter_mononomial(PARSER,char *domain,double *a, unsigned int *n)
{
	double x[64];
	double m = -1;
	double sign = 1;
	double coeff = 1;
	int64 power = 0;
	int first = 1;
	START;
	memset(x,0,sizeof(x));
	if ( WHITE,(first||LITERAL("+")||(LITERAL("-")&&(sign=-1,true)))
		&& (WHITE,TERM(real_value(HERE,&coeff))||(coeff=1,true))
		&& ((WHITE,LITERAL(domain)&&(power=1,true) && (LITERAL("^") && TERM(integer(HERE,&power))))||true) )
	{
		first = 0;
		if ( power > 63 )
		{
			syntax_error(filename,linenum,"filter polynomial order cannot be higher than 63");
			REJECT;
		}
		else
		{
			ACCEPT;
			x[power] = sign*coeff;
			if ( power > m )
				m = power;
		}
	}
	if ( m==-1 )
	{
		syntax_error(filename,linenum,"invalid polynomial");
		REJECT;
	}
	else
	{
		// copy to result buffers
		*n = (m+1);
		memcpy(a,x,sizeof(double)*(m+1));
	}
	DONE;
}

int GldLoader::filter_polynomial(PARSER,char *domain,double *a,unsigned int *n)
{
	double x[64]; // maximum 64th order polynomial
	int m = -1; // order of polynomial
	int first = 1;
	START;
	memset(x,0,sizeof(x));
	if ( WHITE,LITERAL("(") )
	{
		ACCEPT;
		while ( !(WHITE,LITERAL(")")) )
		{
			double sign = 1;
			double coeff = 1;
			int64 power = 0;
			if ( WHITE,(first||LITERAL("+")||(LITERAL("-")&&(sign=-1,true)))
				&& (WHITE,TERM(real_value(HERE,&coeff))||(coeff=1,true))
				&& ((WHITE,LITERAL(domain)&&(power=1,true) && (LITERAL("^") && TERM(integer(HERE,&power))))||true) )
			{
				first = 0;
				if ( power > 63 )
				{
					syntax_error(filename,linenum,"filter polynomial order cannot be higher than 63");
					REJECT;
					break;
				}
				else
				{
					ACCEPT;
					x[power] = sign*coeff;
					if ( power > m )
						m = power;
				}
			}
		}
		ACCEPT;
		if ( m==-1 )
		{
			syntax_error(filename,linenum,"invalid polynomial");
			REJECT;
		}
		else
		{
			// copy to result buffers
			*n = (m+1);
			memcpy(a,x,sizeof(double)*(m+1));
		}
	}
	else if ( TERM(filter_mononomial(HERE,domain,a,n)) )
	{
		ACCEPT;
	}
	else
	{
		REJECT;
	}
	DONE;
}

int GldLoader::filter_option(PARSER, unsigned int64 *flags, unsigned int64 *resolution, double *minimum, double *maximum)
{
	START;
	if WHITE ACCEPT;
	if ( LITERAL("resolution") && (WHITE,LITERAL("=")) && (WHITE,TERM(unsigned_integer(HERE,resolution))) )
	{
		*flags |= FC_RESOLUTION;
		ACCEPT;
	}
	else if ( LITERAL("minimum") && (WHITE,LITERAL("=")) && (WHITE,TERM(real_value(HERE,minimum))) )
	{
		*flags |= FC_MINIMUM;
		ACCEPT;
	}
	else if ( LITERAL("maximum") && (WHITE,LITERAL("=")) && (WHITE,TERM(real_value(HERE,maximum))) )
	{
		*flags |= FC_MAXIMUM;
		ACCEPT;
	}
	else
	{
		syntax_error(filename,linenum,"filter option at or near '%-10.10s' is not recognized", HERE);
		REJECT;
	}
	DONE;
}

int GldLoader::filter_block(PARSER)
{
	char tfname[1024];
	START;
	if WHITE ACCEPT;
	if ( LITERAL("filter") && (WHITE,TERM(filter_name(HERE,tfname,sizeof(tfname)))) )
	{
		char domain[64];
		double timestep=1;
		double timeskew=0;
		unsigned int64 flags = 0;
 		unsigned int64 resolution = 0;
 		double minimum = 0.0;
 		double maximum = 1.0;
		if ( (WHITE,LITERAL("(")) && (WHITE,TERM(name(HERE,domain,sizeof(domain)))) )
		{
			if ( strcmp(domain,"z")==0 )
			{
				double a[64],b[64]; // polynomial coefficients
				unsigned int n,m; // polynomial orders

				// parse z-domain filter parameters
				ACCEPT;
				if ( (WHITE,LITERAL(",")) && (WHITE,TERM(double_timestep(HERE,&timestep))) ) { ACCEPT; }
				if ( (WHITE,LITERAL(",")) && (WHITE,TERM(double_timestep(HERE,&timeskew))) ) { ACCEPT; }
				while ( (WHITE,LITERAL(",")) && (WHITE,TERM(filter_option(HERE,&flags,&resolution,&minimum,&maximum))) ) { ACCEPT; }
				if ( WHITE,LITERAL(")") ) { ACCEPT; }
				else
				{
					syntax_error(filename,linenum,"filter '%s' arguments are not valid at or near '%-10.10s'", tfname, HERE);
					REJECT;
				}

				// parse z-domain filter numerator and denominator
				if ( (WHITE,LITERAL("=")) && (WHITE,TERM(filter_polynomial(HERE,domain,b,&m))) && (WHITE,LITERAL("/"))
					&& (WHITE,TERM(filter_polynomial(HERE,domain,a,&n))) )
				{
					if ( transfer_function_add(tfname,domain,timestep,timeskew,n,a,m,b)==0 )
					{
						syntax_error(filename,linenum,"unable to create transfer function'%s(%s)",tfname,domain);
						REJECT;
					}
					else if ( transfer_function_constrain(tfname,flags,resolution,minimum,maximum)==0 )
 					{
 						syntax_error(filename,linenum,"unable to constrain transfer function'%s(%s)",tfname,domain);
 						REJECT;
 					}
					else
					{
						ACCEPT;
					}
				}
				else
				{
					syntax_error(filename,linenum,"filter transfer function is not valid");
					REJECT;
				}
			}
			else {
				syntax_error(filename,linenum,"only z-domain filters are supported");
				REJECT;
			}
		}
		else
		REJECT;
	}
	else
		REJECT;
	DONE;
}

int GldLoader::extern_block(PARSER)
{
	char code[65536];
	char libname[1024];
	char fnclist[4096];

	START;
	if WHITE ACCEPT;
	if ( LITERAL("extern") && (WHITE,LITERAL("\"C\"") ))
	{
		int startline=0;
		if WHITE ACCEPT;
		if ( TERM(name(HERE,libname,sizeof(libname))) && (WHITE,LITERAL(":")) && (WHITE,TERM(namelist(HERE,fnclist,sizeof(fnclist)))) )
		{
			ACCEPT;
		}
		else
		{
			syntax_error(filename,linenum,"missing library name and/or external function list");
			REJECT;
		}
		if ( WHITE,LITERAL("{") && (WHITE,(startline=linenum),TERM(C_code_block(HERE,code,sizeof(code)))) && LITERAL("}") ) // C-code block
		{
			int rc = module_compile(libname,code,global_module_compiler_flags,
				"typedef struct { void *data, *info;} GLXDATA;\n"
				"#define GLXdouble(X) (*((double*)(X.data)))\n"
				/* TODO add external interface code before this line */,
				filename,startline-1);
			if ( rc==0 )
			{
				if ( module_load_function_list(libname,fnclist) )
				{
					ACCEPT;
				}
				else
				{
					syntax_error(filename,linenum,"unable to load inline functions '%s' from library '%s'", fnclist, libname);
					REJECT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"module_compile error encountered (rc=%d)", rc);
				REJECT;
			}
		}
		else if ( WHITE,LITERAL(";")	)
		{
			if ( module_load_function_list(libname,fnclist) )
			{
				ACCEPT;
			}
			else
			{
				syntax_error(filename,linenum,"unable to load external functions '%s' from library '%s'", fnclist, libname);
				REJECT;
			}
		}
		else
		{
			REJECT;
		}
	}
	DONE;
}

int GldLoader::global_declaration(PARSER)
{
	START;
	if ( WHITE,LITERAL("global") )
	{
		char proptype[256];
		char varname[256];
		char pvalue[1024];
		if ( (WHITE,TERM(name(HERE,proptype,sizeof(proptype))))
			&& (WHITE,TERM(name(HERE,varname,sizeof(varname))))
			)
		{
			UNIT *pUnit = NULL;
			if ( (WHITE,LITERAL("[")) && (WHITE,TERM(unitspec(HERE,&pUnit))) && (WHITE,LITERAL("]")) )
			{
			}
			else
				pUnit = NULL;
			if ( (WHITE,TERM(value(HERE,pvalue,sizeof(pvalue)))) )
			{
				PROPERTYTYPE ptype = property_get_type(proptype);
				if ( ptype == PT_void )
				{
					syntax_error(filename,linenum,"global '%s' type '%s' is not valid",varname,proptype);
					REJECT;
				}
				GLOBALVAR *var = global_create(varname,ptype,NULL,PT_SIZE,1,PT_ACCESS,PA_PUBLIC,NULL);
				if ( var==NULL )
				{
					syntax_error(filename,linenum,"global '%s %s' cannot be defined", proptype, varname);
					REJECT;
				}
				var->prop->unit = pUnit;
				if ( class_string_to_property(var->prop, var->prop->addr,pvalue)==0 )
				{
					syntax_error(filename,linenum,"global '%s %s' cannot be set to '%s'", proptype, varname, pvalue);
					REJECT;
				}
			}
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

int GldLoader::link_declaration(PARSER)
{
	START;
	if ( WHITE,LITERAL("link") )
	{
		char path[1024];
		if ( (WHITE,TERM(value(HERE,path,sizeof(path)))) && LITERAL(";") )
		{
			if ( !link_create(path) )
			{
				syntax_error(filename,linenum,"unable to link '%s'",path);
				REJECT;
			}
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

int GldLoader::script_directive(PARSER)
{
	START;
	if ( WHITE,LITERAL("script") )
	{
		char command[1024];
		if WHITE { ACCEPT; }
		if ( LITERAL("on_create") )
		{	if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_createscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_create script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_init") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_initscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_init script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_sync") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_syncscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_sync script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_precommit") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_precommitscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_precommit script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_commit") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_commitscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_commit script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_term") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_termscript(command)==0 )
				{
					syntax_error(filename,linenum,"unable to add on_term script '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("export") )
		{
			if ( WHITE,TERM(name(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
			{
				if ( exec_add_scriptexport(command)==0 )
				{
					syntax_error(filename,linenum,"unable to export '%s'",command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( TERM(value(HERE,command,sizeof(command))) && (WHITE,LITERAL(";")) )
		{
			int rc;
			IN_MYCONTEXT output_verbose("running command [%s]", command);
			rc = my_instance->subcommand("%s",command);
			if ( rc!=0 )
			{
				syntax_error(filename,linenum,"script failed - return code %d", rc);
				REJECT;
			}
			else
			{
				ACCEPT; DONE;
			}
		}
		else
		{
			REJECT;
		}
	}
	else
		REJECT;
}

int GldLoader::dump_directive(PARSER)
{
	START;
	if ( WHITE,LITERAL("dump") )
	{
		TIMESTAMP interval;
		char dumpfile[1024];
		if ( (WHITE,TERM(integer(HERE,&interval)) )
			&& (WHITE,TERM(value(HERE,dumpfile,sizeof(dumpfile))) )
			&& (WHITE,LITERAL(";")) )
		{
			if ( exec_schedule_dump(interval,dumpfile)==0 )
			{
				syntax_error(filename,linenum,"unable to schedule dump %s at interval %d second",dumpfile,interval);
				REJECT;
			}
			else
			{
				ACCEPT; DONE;
			}
		}
		else
		{
			REJECT;
		}
	}
	else
		REJECT;
}

int GldLoader::modify_directive(PARSER)
{
	START;
	if ( WHITE,LITERAL("modify") )
	{
		char oname[64], pname[64], ovalue[1024];
		if ( (WHITE,TERM(name(HERE,oname,sizeof(oname)))) && LITERAL(".") && TERM(dotted_name(HERE,pname,sizeof(pname))) && (WHITE,TERM(value(HERE,ovalue,sizeof(ovalue)))) && LITERAL(";") )
		{
			OBJECT *obj = object_find_name(oname);
			if ( obj )
			{
				if ( object_set_value_by_name(obj,pname,ovalue)<0 )
				{
					syntax_error(filename,linenum,"modify property '%s' of object '%s' couldn't not be set to '%' ", pname, oname, ovalue);
					REJECT;
				}
				else
				{
					ACCEPT;
				}
			}
			else
			{
				syntax_error(filename,linenum,"modify object '%s' not found", oname);
				REJECT;
			}
		}
	}
	else
		REJECT;
	DONE;
}


void GldLoader::loader_addhook(PARSERCALL call)
{
	LOADERHOOK *hook = new LOADERHOOK;
	if ( hook == NULL )
	{
		throw "loader_addhook(): memory allocation failed";
	}
	hook->call = call;
	hook->next = loaderhooks;
	loaderhooks = hook;
}

int GldLoader::loader_hook(PARSER)
{
	char libname[1024];
	START;
	if ( WHITE ) ACCEPT;
	if ( LITERAL("extension") && WHITE && name(HERE,libname,sizeof(libname)) )
	{
		// find the library
		char pathname[2048];
		snprintf(pathname, sizeof(pathname)-1, "%s" DLEXT, libname);
		if ( find_file(pathname, NULL, X_OK|R_OK, pathname,sizeof(pathname)) == NULL )
		{
			syntax_error(filename,linenum,"unable to locate %s in GLPATH=%s", pathname,getenv("GLPATH")?getenv("GLPATH"):"");
			REJECT;
		}
		IN_MYCONTEXT output_debug("loader extension '%s' is using library '%s", libname, pathname);

		// load the library
		void *lib = dlopen(pathname,RTLD_LAZY);
		if ( lib == NULL )
		{
			syntax_error(filename,linenum,"extension library '%s' load failed", pathname);
			syntax_error(filename,linenum,"%s", dlerror());
			REJECT;
		}
		IN_MYCONTEXT output_debug("loader extension '%s' loaded ok", pathname);

		// access and call the initialization function
		LOADERINIT init = (LOADERINIT) dlsym(lib,"init");
		if ( init )
		{
			int rc = init();
			if ( rc != 0 )
			{
				syntax_error(filename,linenum,"extension library '%s' init() failed, return code %d", pathname, rc);
				REJECT;
			}
		}
		IN_MYCONTEXT output_debug("loader extension '%s' init ok", libname);

		// find and link the parser
		void *parser = dlsym(lib,"parser");
	 	if ( parser == NULL )
	 	{
			syntax_error(filename,linenum,"extension library '%s' does not export a parser function", pathname);
			REJECT;
	 	}
		IN_MYCONTEXT output_debug("loader extension '%s' parser linked", libname);

		loader_addhook((PARSERCALL)parser);

		// add init callback
		INITCALL initcall = (INITCALL) dlsym(lib,"on_init");
		if ( initcall )
		{
			my_instance->get_exec()->add_initcall(initcall);
		}

		// add term callback
		TERMCALL termcall = (TERMCALL) dlsym(lib,"on_term");
		if ( termcall )
		{
			my_instance->get_exec()->add_termcall(termcall);
		}

		// add exit callback
		EXITCALL exitcall = (EXITCALL) dlsym(lib,"term");
		if ( exitcall )
		{
			my_instance->add_on_exit(exitcall);
		}

		ACCEPT;
		DONE;
	}
	else
	{
		for ( LOADERHOOK *hook = loaderhooks ; hook != NULL ; hook = hook->next )
		{
			if ( TERM(hook->call(HERE)) )
			{
				ACCEPT;
				DONE;
			}
		}
		REJECT;
	}
}

int GldLoader::gridlabd_file(PARSER)
{
	START;
	if WHITE {ACCEPT; DONE;}
	OR if LITERAL(";") {ACCEPT; DONE;}
	OR if TERM(line_spec(HERE)) { ACCEPT; DONE; }
	OR if TERM(object_block(HERE,NULL,NULL)) {ACCEPT; DONE;}
	OR if TERM(class_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(module_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(clock_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(import(HERE)) {ACCEPT; DONE; }
	OR if TERM(export_model(HERE)) {ACCEPT; DONE; }
	OR if TERM(library(HERE)) {ACCEPT; DONE; }
	OR if TERM(schedule(HERE)) {ACCEPT; DONE; }
	OR if TERM(instance_block(HERE)) {ACCEPT; DONE; }
	OR if TERM(gui(HERE)) {ACCEPT; DONE;}
	OR if TERM(extern_block(HERE)) {ACCEPT; DONE; }
	OR if TERM(filter_block(HERE)) {ACCEPT; DONE; }
	OR if TERM(global_declaration(HERE)) {ACCEPT; DONE; }
	OR if TERM(link_declaration(HERE)) { ACCEPT; DONE; }
	OR if TERM(script_directive(HERE)) { ACCEPT; DONE; }
	OR if TERM(dump_directive(HERE)) { ACCEPT; DONE; }
	OR if TERM(modify_directive(HERE)) { ACCEPT; DONE; }
	OR if TERM(loader_hook(HERE)) { ACCEPT; DONE; }
	OR if (*(HERE)=='\0') {ACCEPT; DONE;}
	else
	{
		syntax_error_here(HERE);
		REJECT;
	}
	DONE;
}

int GldLoader::replace_variables(char *to,char *from,int len,int warn)
{
	char *p, *e=from;
	int n = 0;
	while ((p=strstr(e,"${"))!=NULL)
	{
		char varname[1024];
		if (sscanf(p+2,"%1024[^}]",varname)==1)
		{
			char *env = getenv(varname);
			const char *var;
			int m = (int)(p-e);
			strncpy(to+n,e,m);
			n += m;
			var =  global_getvar(varname,to+n,len-n);
			if (var!=NULL)
				n+=(int)strlen(var);
			else if (env!=NULL)
			{
				strncpy(to+n,env,len-n);
				n+=(int)strlen(env);
			}
			else if ( warn )
			{
				/* this must be benign because otherwise macros that are inactive fail when they shouldn't */
				output_warning("%s(%d): variable '%s' not found", filename, linenum, varname);
				/* TROUBLESHOOT
					A macro refers to a variable that is not defined.  Correct the variable reference, or
					define the variable before using it and try again.
				 */
			}
			e = strchr(p,'}');
			if (e==NULL)
				goto Unterminated;
			e++;
		}
		else
		{
Unterminated:
			syntax_error(filename,linenum,"unterminated variable name %.10p...", p);
			return 1;
		}
	}

	if ((int)strlen(e)<len-n)
	{
		strcpy(to+n,e);
		return (int)strlen(to);
	}
	else
	{
		syntax_error(filename,linenum,"insufficient buffer space to continue");
		return -1;
	}
}

int GldLoader::buffer_read(FILE *fp, char *buffer, char *filename, int size)
{
	char line[65536];
	int n=0;
	int linenum=0;
	int startnest = nesting;
	while ( fgets(line,sizeof(line),fp) != NULL )
	{
		int len;
		char subst[65536];

		/* comments must have preceding whitespace in macros */
		char *c = ( ( line[0] != '#' ) ? strstr(line,"//") : strstr(line, " " "//") );
		linenum++;
		if ( c != NULL )
		{
			/* truncate at comment */
			strcpy(c,"\n");
		}
		len = (int)strlen(line);
		if ( len >= size-1 )
		{
			return 0;
		}

		/* expand variables */
		if ( (len=replace_variables(subst,line,sizeof(subst),suppress==0)) >= 0 )
		{
			strcpy(line,subst);
		}
		else
		{
			syntax_error(filename,linenum,"unable to continue");
			return -1;
		}

		/* expand macros */
		if ( line[0] == '#' )
		{
			/* macro disables reading */
			if ( process_macro(line,sizeof(line),filename,linenum) == FALSE )
			{
				return 0;
			}
			len = (int)strlen(line);
			strcat(buffer,line);
			buffer += len;
			size -= len;
			n += len;
		}

		/* if reading is enabled */
		else if ( suppress == 0 )
		{
			strcpy(buffer,subst);
			buffer+=len;
			size -= len;
			n+=len;
		}
	}
	if ( nesting != startnest )
	{
		syntax_error(filename,linenum,"Unbalanced #if/#endif at %s(%d) ~ started with nestlevel %i, ending %i",filename,macro_line[nesting-1], startnest, nesting);
		return -1;
	}
	return n;
}

GldLoader::FORLOOPSTATE GldLoader::for_set_state(GldLoader::FORLOOPSTATE n)
{
	const char *str[] = {"FOR_NONE","FOR_BODY","FOR_REPLAY"};
	FORLOOPSTATE m = forloopstate;
	forloopstate = n;
	if ( forloop_verbose ) output_verbose("forloop state changed from '%s' to '%s'",str[(int)m],str[(int)n]);
	return m;
}

bool GldLoader::for_is_state(GldLoader::FORLOOPSTATE n)
{
	return for_get_state() == n;
}

// Open a new instance of the forloop machine
// Returns: 'true' if forloop opened, 'false' if forloop not opened
bool GldLoader::for_open(const char *var, const char *range)
{
	if ( forloop != NULL )
	{
		syntax_error(filename,linenum,"nested forloop not supported");
		return false;
	}
	forloop = strdup(range);
	forvar = strdup(var);
	if ( forloop_verbose ) output_verbose("beginning forloop on variable '%s' in range [%s]", forvar, forloop);
	for_set_state(FOR_BODY);
	return true;
}

// Update the global variable of the forloop machine
// Returns: the next value, or NULL if not remains
const char * GldLoader::for_setvar(void)
{
	const char *value = strtok_r(forvalue==NULL?forloop:NULL," ",&lastfor);
	if ( value != NULL )
	{
		if ( forloop_verbose ) output_verbose("setting for variable '%s' to '%s'",forvar,value);
		bool old = global_strictnames;
		global_strictnames = false;
		global_setvar(forvar,value);
		global_strictnames = old;
	}
	else
	{
		if ( forloop_verbose ) output_verbose("no more values for variable '%s' after '%s'",forvar,forvalue);
	}
	return value;
}

// Capture a GLM line to the forloop machine to replay later
bool GldLoader::for_capture(const char *line)
{
	if ( strncmp(line,"#done",5) == 0 )
	{
		if ( forloop_verbose ) output_verbose("capture of forloop body done with after %d lines", forbuffer.size());
		for_set_state(FOR_REPLAY);
		return false;
	}
	else
	{
		if ( forloop_verbose ) output_verbose("capturing forloop body line %d as '%s'", forbuffer.size(), line);
		forbuffer.push_back(std::string(line));
		forbufferline = forbuffer.end();
		return true;
	}
}

// Replay the next line in the forloop
const char *GldLoader::for_replay(void)
{
	// need to get first/next value in list
	if ( forbufferline == forbuffer.end() )
	{
		if ( forloop_verbose ) output_verbose("end of replay buffer with forloop var '%s'='%s'", forvar,forvalue);
		forvalue = for_setvar();
		forbufferline = forbuffer.begin();
	}

	// no values left
	if ( forvalue == NULL )
	{
		output_verbose("forloop in var '%s' replay complete", forvar, forloop);
		if ( forloop ) free(forloop);
		lastfor = NULL;
		if ( forvar ) free(forvar);
		forvalue = NULL;
		forbuffer.clear();
		forbufferline = forbuffer.end();
		for_set_state(FOR_NONE);
		return NULL;
	}
	else
	{
		// get next line
		const char *line = (forbufferline++)->c_str();
		if ( forloop_verbose ) output_verbose("forloop replaying line '%s' with '%s'='%s'", line,forvar,forvalue);
		return line;
	}
}


void GldLoader::load_add_language(const char *name, bool (*parser)(const char*,void*context), void* (*init)(int,const char**))
{
	LANGUAGE *item = new LANGUAGE;
	item->name = strdup(name);
	item->parser = parser;
	item->init = init;
	item->next = language_list;
	language_list = item;
}

PyObject *python_loader_module = NULL;
int GldLoader::set_language(const char *name)
{
	if ( name == NULL )
	{
		if ( language == NULL )
		{
			output_warning("%s(%d): no language set",filename,linenum);
			return FALSE;
		}
		language = NULL;
		return TRUE;
	}
	for ( language = language_list ; language != NULL ; language = language->next  )
	{
		if ( strcmp(language->name,name) == 0 )
		{
			if ( strcmp(name,"python") == 0 && python_loader_module == NULL )
			{
				python_loader_module = (PyObject*)python_loader_init(0,NULL);
			}
			return TRUE;
		}
	}
	syntax_error(filename,linenum,"language '%s' not recognized", name);
	return FALSE;
}

int GldLoader::buffer_read_alt(FILE *fp, char *buffer, char *filename, int size)
{
	char line[0x4000];
	int n = 0, i = 0;
	int _linenum=0;
	int startnest = nesting;
	int bnest = 0, quote = 0;
	int hassc = 0; // has semicolon
	int quoteline = 0;
	while ( for_is_state(FOR_REPLAY) || fgets(line,sizeof(line),fp) != NULL )
	{
		int len;
		char subst[65536];

		if ( for_is_state(FOR_BODY) && for_capture(line) )
		{
			continue;
		}
		if ( for_is_state(FOR_REPLAY) )
		{
			const char *replay = for_replay();
			if ( replay )
			{
				// load next line
				strcpy(line,replay);
			}
			else
			{
				// done, resume normal processing of input stream
				continue;
			}
		}
		else
		{
			/* comments must have preceding whitespace in macros */
			char *c = line[0]!='#'?strstr(line,"//"):strstr(line, " " "//");
			_linenum++;
			if ( c != NULL ) /* truncate at comment */
			{
				strcpy(c,"\n");
			}
			len = (int)strlen(line);
			if ( len >= size-1 )
			{
				output_error("load.c: buffer exhaustion reading %i lines past line %i", _linenum, linenum);
				if ( quote != 0 )
				{
					output_error("look for an unterminated doublequote string on line %i", quoteline);
				}
				return 0;
			}
		}

		// expand variables
		if ( (len=replace_variables(subst,line,sizeof(subst),suppress==0)) >= 0 )
		{
			strcpy(line,subst);
		}
		else
		{
			syntax_error(filename,_linenum,"unable to continue");
			return -1;
		}

		/* expand macros */
		const char *m = line;
		while ( isspace(*m) ) m++;
		if ( get_language() || m[0] == '#' )
		{
			/* macro disables reading */
			if ( process_macro(line,sizeof(line),filename,linenum + _linenum - 1) == FALSE )
			{
				return 0;
			}
			else
			{
				++hassc;
			}
			strcpy(buffer,line);
			len = (int)strlen(buffer); // include anything else in the buffer, then advance
			buffer += len;
			size -= len;
			n += len;
		}

		/* if reading is enabled */
		else if ( suppress == 0 )
		{
			strcpy(buffer,subst);
			buffer+=len;
			size -= len;
			n+=len;
			for ( i = 0 ; i < len ; ++i )
			{
				if ( quote == 0 )
				{
					if ( subst[i] == '\"' )
					{
						quoteline = linenum + _linenum - 1;
						quote = 1;
					}
					else if ( subst[i] == '{' )
					{
						++bnest;
						++hassc;
						// @TODO push context
					}
					else if ( subst[i] == '}' )
					{
						--bnest;
						// @TODO pop context
					}
					else if( subst[i] == ';' )
					{
						++hassc;
					}
				}
				else
				{
					if ( subst[i] == '\"' )
					{
						quote = 0;
					}
				}
			}
		}
		else
		{
			strcpy(buffer,"\n");
			buffer+=strlen("\n");
			size -= 1;
			n += 1;
		}
		if ( bnest == 0 && hassc > 0 && nesting == startnest ) // make sure we read ALL of an #if block, if possible
		{
			/* end of block */
			return n;
		}

	}
	if ( quote != 0 )
	{
		output_warning("unterminated doublequote string");
	}
	if ( bnest != 0 )
	{
		output_warning("incomplete loader block");
	}
	if ( nesting != startnest )
	{
		syntax_error(filename,_linenum,"Unbalanced #if/#endif at %s(%d) ~ started with nestlevel %i, ending %i",filename,macro_line[nesting-1], startnest, nesting);
		return -1;
	}
	return n;
}


int GldLoader::include_file(char *incname, char *buffer, int size, int _linenum)
{
	int move = 0;
	char *p = buffer;
	int count = 0;
	char *ext = 0;
	char *name = 0;
	struct stat stat;
	char ff[1024];
	FILE *fp = 0;
	char buffer2[20480];
	unsigned int old_linenum = _linenum;
	/* check include list */
	INCLUDELIST *list;
	INCLUDELIST *my = (INCLUDELIST *)malloc(sizeof(INCLUDELIST));//={incname,include_list}; /* REALLY BAD IDEA ~~ "this" is a reserved C++ keyword */

	strcpy(my->file, incname);
	my->next = include_list;

	buffer2[0]=0;

	for (list = include_list; list != NULL; list = list->next)
	{
		if (strcmp(incname, list->file) == 0 && !global_reinclude )
		{
			syntax_error(incname,_linenum,"include file has already been included");
			return 0;
		}
	}

	/* if source file, add to header list and keep moving */
	ext = strrchr(incname, '.');
	name = strrchr(incname, '/');
	if (ext>name) {
		if(strcmp(ext, ".hpp") == 0 || strcmp(ext, ".h")==0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0){
			// append to list
			for (list = header_list; list != NULL; list = list->next){
				if(strcmp(incname, list->file) == 0){
					// normal behavior
					return 0;
				}
			}
			my->next = header_list;
			header_list = my;
		}
	} else { /* no extension */
		for (list = header_list; list != NULL; list = list->next){
			if(strcmp(incname, list->file) == 0){
				// normal behavior
				return 0;
			}
		}
		my->next = header_list;
		header_list = my;
	}

	/* open file */
	fp = find_file(incname,NULL,R_OK,ff,sizeof(ff)) ? fopen(ff, "rt") : NULL;

	if(fp == NULL){
		syntax_error(incname,_linenum,"include file open failed: %s", errno?strerror(errno):"(no details)");
		return -1;
	}
	else
	{
		IN_MYCONTEXT output_verbose("include_file(char *incname='%s', char *buffer=0x%p, int size=%d): search of GLPATH='%s' result is '%s'",
			incname, buffer, size, getenv("GLPATH") ? getenv("GLPATH") : "NULL", ff);
	}
	add_depend(incname,ff);

	old_linenum = linenum;
	linenum = 1;

	if(fstat(fileno(fp), &stat) == 0){
		if(stat.st_mtime > modtime){
			modtime = stat.st_mtime;
		}

		//if(size < stat.st_size){
			/** @todo buffer must grow (ticket #31) */
			/* buffer = realloc(buffer,size+stat.st_size); */
		//	output_message("%s(%d): unable to grow size of read buffer to include file", incname, linenum);
		//	return 0;
		//}
	} else {
		syntax_error(incname,_linenum,"unable to get size of included file");
		return -1;
	}

	IN_MYCONTEXT output_verbose("%s(%d): included file is %d bytes long", incname, old_linenum, stat.st_size);

	/* reset line counter for parser */
	include_list = my;
	//count = buffer_read(fp,buffer,incname,size); // fread(buffer,1,stat.st_size,fp);

	move = buffer_read_alt(fp, buffer2, incname, 20479);
	while(move > 0){
		count += move;
		p = buffer2; // grab a block
		while(*p != 0){
			// and process it
			move = gridlabd_file(p);
			if(move == 0)
				break;
			p += move;
		}
		if(*p != 0){
			// failed if we didn't parse the whole thing
			count = -1;
			break;
		}
		move = buffer_read_alt(fp, buffer2, incname, 20479);
	}

	//include_list = my.next;

	linenum = old_linenum;
	fclose(fp);
	return count;
}

/** @return 1 if the variable is autodefined */
static int is_autodef(char *value)
{
#ifdef WIN32
	if ( strcmp(value,"WINDOWS")==0 ) return 1;
#elif defined APPLE
	if ( strcmp(value,"APPLE")==0 ) return 1;
#elif defined LINUX
	if ( strcmp(value,"LINUX")==0 ) return 1;
#endif

#ifdef _DEBUG
	if ( strcmp(value,"DEBUG")==0 ) return 1;
#endif

#ifdef HAVE_MATLAB
	if ( strcmp(value,"MATLAB")==0 ) return 1;
#endif

	if ( strcmp(value,"PYTHON")==0 ) return 1;

	return 0;
}

/* started processes */
#include "threadpool.h"
#include "signal.h"

void GldLoader::kill_processes(void)
{
	while ( threadlist != NULL )
	{
		struct s_threadlist *next = threadlist->next;
		int sig = SIGTERM;
		int rc = pthread_kill(*(threadlist->data),sig);
		switch ( rc ) {
		case 0:
			IN_MYCONTEXT output_debug("killing thread %p", threadlist->data);
			break;
		case ESRCH:
			output_error("unable to kill thread %p (no such thread)", threadlist->data);
			break;
		case EINVAL:
			output_error("unable to kill thread %p (signal %d invalid/ignored)", threadlist->data, sig);
			break;
		default:
			output_error("unable to kill thread %p (unknown return code %d)", threadlist->data, rc);
			break;
		}
		free(threadlist->data);
		threadlist=next;
	}
}

bool load_import(const char *from, char *to, int len);

void *subcommand(void *arg)
{
	const char *cmd = (const char*)arg;
	int64 rc = my_instance->subcommand("%s",cmd);
	return (void*)rc;
}
/** @return -1 on failure, thread_id on success **/
void* GldLoader::start_process(const char *cmd)
{
	static bool first = true;
	pthread_t *pThreadInfo = (pthread_t*)malloc(sizeof(pthread_t));
	struct s_threadlist *thread = (struct s_threadlist*)malloc(sizeof(struct s_threadlist));
	char *args = (char*)malloc(strlen(cmd)+1);
	strcpy(args,cmd);
	if ( thread==NULL || pThreadInfo==NULL || pthread_create(pThreadInfo,NULL,(void*(*)(void*))subcommand,args)!=0 )
	{
		syntax_error(filename,linenum,"unable to create thread to start '%s'", cmd);
		return NULL;
	}
	else
	{
		IN_MYCONTEXT output_debug("creating thread %p for process '%s'", pThreadInfo, cmd);
	}
	thread->data = pThreadInfo;
	thread->next = threadlist;
	threadlist = thread;
	if ( first )
	{
		atexit(kill_processes);
		first = false;
	}
	return threadlist;
}

bool found_in_list(const char *tag, const char* taglist, char delim=',')
{
	if ( taglist == NULL || strcmp(taglist,"*") == 0 )
	{
		return true; // null taglist implies all tags match
	}
	if ( tag == NULL )
	{
		return false; // tag cannot match anything but null or "*" taglist
	}
	const char *b = strstr(taglist,tag); // begin of match
	const char *e = b + strlen(tag); // end of match
	if ( b == NULL )
	{
		return false; // not found in taglist
	}
	if ( ( b == taglist || b[-1] == delim ) && ( e[0] == delim || e[0] == '\0' ) )
	{
		return true; // good match
	}
	return e>b && e[-1] == '*'; // no match unless * used
}

int writefile(char *fname, char *specs)
{
	// identify filetype based on extension
	enum e_filetype {NONE, CSV, JSON, GLM} filetype = NONE;
	char *ext = strrchr(fname,'.');
	struct s_map {
		const char *ext;
		enum e_filetype type;
	} map[] = {
		{".csv",CSV},
		{".json",JSON},
		{".glm",GLM},
	};
	for ( struct s_map *m = map ; m < map + sizeof(map)/sizeof(map[0]) ; m++ )
	{
		if ( ext && strcmp(ext,m->ext) == 0 )
		{
			filetype = m->type;
			break;
		}
	}
	if ( filetype == NONE )
	{
		output_error("writefile(char *fname='%s', char *specs='%s'): unknown filetype for extension '%s'",fname,specs,ext);
	}


	// separate class and properties
	char *classes = NULL;
	char *properties = specs;
	char *colon = strchr(specs,':');
	if ( colon != NULL )
	{
		*colon = '\0';
		classes = specs;
		properties = colon+1;
	}

	// open fname for write
	FILE *fp = fopen(fname,"w");
	if ( fp == NULL )
	{
		output_error("writefile(char *fname='%s', char *specs='%s'): unable to open file for write",fname,specs);
		return -1;
	}
	else
	{
		switch (filetype)
		{
		case CSV:
			fprintf(fp,"id,class,name,%s\n",properties);
			break;
		case JSON:
			fprintf(fp,"[\n");
			break;
		case GLM:
			fprintf(fp,"// writefile(char *fname='%s', char *specs='%s')\n// modelname %s\n",fname,specs,global_modelname);
			break;
		default:
			break;
		}
	}

	// construct property list
	const size_t maxprops = 256;
	char *proplist[maxprops];
	char *last = NULL, *next = NULL;
	size_t n_props = 0;
	while ( (next=strtok_r(next?NULL:properties,",",&last)) != NULL && n_props < maxprops )
	{
		proplist[n_props++] = next;
	}
	if ( n_props == maxprops )
	{
		output_warning("writefile(char *fname='%s', char *specs='%s'): only %d properties can be written for class '%s'",fname,specs,maxprops,classes);
	}

	// process objects in model
	int found = 0;
	for ( OBJECT *obj = object_get_first() ; obj != NULL ; obj = object_get_next(obj) )
	{
		if ( found_in_list(obj->oclass->name,classes) )
		{
			switch (filetype)
			{
			case CSV:
                if ( obj->name )
                {
                    fprintf(fp,"%d,%s,\"%s\"",obj->id,obj->oclass->name,obj->name);
                }
                else
                {
                    fprintf(fp,"%d,%s,\"%s:%d\"",obj->id,obj->oclass->name,obj->oclass->name,obj->id);
                }
				break;
			case JSON:
				if ( obj->name == NULL )
				{
					fprintf(fp,"\t{\n\t\t\"name\" : \"%s:%d\"",obj->oclass->name,obj->id);
					fprintf(fp,",\n\t\t\"class\" : \"%s\"",obj->oclass->name);
					fprintf(fp,",\n\t\t\"id\" : \"%d\"",obj->id);
				}
				else
				{
					fprintf(fp,"\t{\n\t\t\"name\" : \"%s\"",obj->name);
					fprintf(fp,",\n\t\t\"class\" : \"%s\"",obj->oclass->name);
					fprintf(fp,",\n\t\t\"id\" : \"%d\"",obj->id);
				}
				break;
			case GLM:
				break;
			default:
				break;
			}
			for ( size_t n = 0 ; n < n_props ; n++ )
			{
				char value[1024] = "";
                if ( object_get_value_by_name(obj,proplist[n],value,sizeof(value)-1) < 0 )
				{
					output_warning("writefile(char *fname='%s', char *specs='%s'): unable to get value for property '%s' object '%s:%d'",fname,specs,proplist[n],obj->oclass->name,obj->id);
				}
                else
                {
    				const char *quote = ( strchr(value,',') != NULL && value[0] != '"' ) ? "\"" : "";
    				switch (filetype)
    				{
    				case CSV:
    					fprintf(fp,",%s%s%s",quote,value,quote);
    					break;
    				case JSON:
                        if ( object_get_property(obj,proplist[n],NULL) )
                        {
                            // only output properties that exist
                            fprintf(fp,",\n\t\t\"%s\" : \"%s\"",proplist[n],value);
                        }
    					break;
    				case GLM:
                        if ( ! object_get_property(obj,proplist[n],NULL) )
                        {
                            // output nothing
                        }
                        else if ( obj->name == NULL )
    					{
    						fprintf(fp,"modify %s:%d.%s \"%s\";\n",obj->oclass->name,obj->id,proplist[n],value);
    					}
    					else
    					{
    						fprintf(fp,"modify %s.%s \"%s\";\n",obj->name,proplist[n],value);
    					}
    					break;
    				default:
    					break;
    				}
                }
			}
			switch (filetype)
			{
			case CSV:
				fprintf(fp,"\n");
				break;
			case JSON:
				fprintf(fp,"\n\t}\n");
				break;
			case GLM:
				break;
			default:
				break;
			}
			found++;
		}
	}
	switch (filetype)
	{
	case CSV:
		break;
	case JSON:
		fprintf(fp,"]\n");
		break;
	case GLM:
		break;
	default:
		break;
	}
	if ( found == 0 )
	{
		output_warning("writefile(char *fname='%s', char *specs='%s'): no matches found for class '%s'",fname,specs,classes);
	}
    fclose(fp);
	return found;
}

int readfile(char *fname, char *specs, char* line, int size)
{
	output_warning("readfile(fname='%s',specs='%s',line='%s',size=%d): not implemented yet",fname,specs,line,size);
	return -1;
}

/** @return TRUE/SUCCESS for a successful macro read, FALSE/FAILED on parse error (which halts the loader) */
int GldLoader::process_macro(char *line, int size, char *_filename, int linenum)
{
	char *var, *val, *save;
	char buffer[1024];
	if ( get_language() )
	{
		const char *m = line;
		while ( isspace(*m) ) m++;
		int status;
		if ( strncmp(m,"#end",4) == 0 )
		{
			status = language->parser(NULL,NULL);
			set_language(NULL);
		}
		else
		{
			status = language->parser(line,NULL);
		}
		if ( status == true )
		{
			strcpy(line,"\n");
		}
		return status;
	}
	while ( isspace(*line) ) line++; // trim
	if (strncmp(line,"#endif",6)==0)
	{
		if (nesting>0)
		{
			// @TODO pop 'if' context
			nesting--;
			suppress &= ~(1<<nesting);
		}
		else{
			syntax_error(filename,linenum,"#endif is mismatched");
		}
		strcpy(line,"\n");

		return TRUE;
	}
	else if (strncmp(line,"#else",5)==0)
	{
		char *term;

		// @TODO pop 'if' context (old context)
		// @TODO push 'if' context (else context)

		if ( (suppress&(1<<(nesting-1))) == (1<<(nesting-1)) ){
			suppress &= ~(1<<(nesting-1));
		} else {
			suppress |= (1<<(nesting-1));
		}
		term = line+5;
		strip_right_white(term);
		if(strlen(term)!=0)
		{
			syntax_error(filename,linenum,"#else macro should not contain any terms");
			return FALSE;
		}
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#ifdef",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#ifdef macro missing term");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		if ( !is_autodef(value) && global_getvar(value, buffer, sizeof(buffer))==NULL && getenv(value)==NULL){
			suppress |= (1<<nesting);
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context

		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#ifexist",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		char path[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#ifexist macro missing term");
			return FALSE;
		}
		while(isspace((unsigned char)(*term)))
			++term;
		//if (sscanf(term,"\"%[^\"\n]",value)==1 && find_file(value, NULL, 0)==NULL)
		strcpy(value, strip_right_white(term));
		if(value[0] == '"'){
			char stripbuf[1024];
			sscanf(value, "\"%[^\"\n]", stripbuf);
			strcpy(value, stripbuf);
		}
		if (find_file(value, NULL, F_OK, path,sizeof(path))==NULL)
			suppress |= (1<<nesting);
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'file' context
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#ifndef",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#ifndef macro missing term");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		if(global_getvar(value, buffer, sizeof(buffer))!=NULL || getenv(value)!=NULL){
			suppress |= (1<<nesting);
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#if",3)==0)
	{
		char left[1024], op[4];
		const char *value;
		char right[1024];
		if ( sscanf(line+4,"\"%1023[^\"]\" %3[!<>=] \"%1023[^\"]\"",left,op,right) < 3
			&& sscanf(line+4,"%1023[^!<>= \t] %3[!<>=] \"%1023[^\"]\"",left,op,right) < 3
			&& sscanf(line+4,"\"%1023[^\"]\" %3[!<>=] %1023s",left,op,right) < 3
			&& sscanf(line+4,"%1023[^!<>= \t] %3[!<>=] %1023s",left,op,right) < 3 )
		{
			syntax_error(filename,linenum,"#if macro statement syntax error");
			strcpy(line,"\n");
			return FALSE;
		}
		value =  ( global_literal_if ? left : global_getvar(left, buffer, sizeof(buffer)) );
		if ( value==NULL )
		{
			if ( global_relax_undefined_if )
				value = "";
			else
			{
				syntax_error(filename,linenum,"%s is not defined",left);
				strcpy(line,"\n");
				return FALSE;
			}
		}
		if (strcmp(op,"<")==0) { if (!(strcmp(value,right)<0)) suppress|=(1<<nesting); }
		else if (strcmp(op,">")==0) { if (!(strcmp(value,right)>0)) suppress|=(1<<nesting); }
		else if (strcmp(op,">=")==0) { if (!(strcmp(value,right)>=0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"<=")==0) { if (!(strcmp(value,right)<=0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"==")==0) { if (!(strcmp(value,right)==0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"!=")==0) { if (!(strcmp(value,right)!=0)) suppress|=(1<<nesting); }
		else
		{
			syntax_error(filename,linenum,"operator %s is not recognized",op);
			strcpy(line,"\n");
			return FALSE;
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context
		strcpy(line,"\n");
		return TRUE;
	}

	/* handles suppressed macros */
	if (suppress!=0)
	{
		strcpy(line,"\n");
		return TRUE;
	}

	/* macros that are short for other macros */
	if ( strncmp(line,"#insert",7)==0 )
	{
		char name[1024];
		char values[1024]="";
		if ( sscanf(line+7,"%*[ \t]%[^(](%[^)])",name,values)==0 )
		{
			syntax_error(filename,linenum,"#insert syntax error -- name not found");
			return FALSE;
		}
		snprintf(line,size-1,"#include using(%s) \"%s.glm\"",values,name);
		/* fall through to normal parsing of macros */
	}
	else /* add other macro macros here */
	{
		/* fall through to normal parsing of macros */
	}

	/* these macros can be suppressed */
	if (strncmp(line,"#include",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		char oldfile[1024];
		GLOBALVAR *old_stack = NULL;
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#include macro missing term");
			strcpy(line,"\n");
			return FALSE;
		}
		while(isspace((unsigned char)(*term)))
			++term;
		if ( sscanf(term,"using(%[^)])",value)==1 )
		{
			char *token, tmp[1024], *string=tmp;
			old_stack = global_getnext(NULL);
			strcpy(tmp,value);
			while ( (token=strsep(&string, ",")) != NULL)
			{
				char var[1024], val[1024];
				if ( sscanf(token,"%[^=]=%[^\n]",var,val)!=2 )
					syntax_error(filename,linenum,"unabled to set global %s", token);
				else
					global_push(var,val);
				global_reinclude = TRUE; // must enable reinclude for this to work more than once
			}
			term+=strlen(value)+7;
		}
		while(isspace((unsigned char)(*term)))
			++term;
		if (sscanf(term,"\"%[^\"]\"",value)==1)
		{
			int len = snprintf(line,size-1,"@%s;%d\n",value,0);
			line+=len; size-=len;
			strcpy(oldfile, filename);	// push old filename
			strcpy(filename, value);	// use include file name for errors while within context
			len=(int)include_file(value,line,size,linenum);
			strcpy(filename, oldfile);	// pop include filename, use calling filename
			add_depend(filename,value);
			if (len<0)
			{
				syntax_error(filename,linenum,"#include failed");
				include_fail = 1;
				strcpy(line,"\n");
				if ( old_stack ) global_restore(old_stack);
				return FALSE;
			}
			else
			{
				len = snprintf(line,size-1,"@%s;%d\n",filename,linenum);
				line+=len; size-=len;
				return size>0;
			}
		}
		else if (sscanf(term, "<%[^>]>", value) == 1)
		{
			/* C include file */
			IN_MYCONTEXT output_verbose("added C include for \"%s\"", value);
			append_code("#include <%s>\n",value);
			strcpy(line,"\n");
			if ( old_stack ) global_restore(old_stack);
			return TRUE;
		}
		else if ( sscanf(term, "[%[^]]]", value)==1 )
		{
			char tmpname[1024];
			snprintf(tmpname,sizeof(tmpname)-1,"url_%08llx.glm",hash(value));
			try
			{
				GldCurl(value,tmpname);
			}
			catch (const char *errmsg)
			{
				output_error("%s(%d): URL include failed, %s",filename,linenum,errmsg);
				return FALSE;
			}
			catch (...)
			{
				output_error("%s(%d): URL include failed, unknown exception",filename,linenum);
				return FALSE;
			}

			/* load temp file */
			strcpy(oldfile,filename);
			strcpy(filename,tmpname);
			int len = (int)include_file(tmpname,line,size,linenum);
			strcpy(filename,oldfile);
			add_depend(filename,tmpname);
			if ( len < 0 )
			{
				output_error("%s(%d): unable to include load [%s] from temp file '%s'", filename, linenum, value,tmpname);
				if ( old_stack ) global_restore(old_stack);
				return FALSE;
			}
			else
			{
				snprintf(line+len,size-len-1,"@%s;%d\n",filename,linenum);
				if ( old_stack ) global_restore(old_stack);
				return TRUE;
			}
		}
		else if (sscanf(term, "(%[^)])", value) == 1)
		{
			/* C include file */
			IN_MYCONTEXT output_verbose("executing include shell \"%s\"", value);
			my_instance->subcommand("%s",value);
			return TRUE;
		}
		else
		{
			char *eol = term+strlen(term)-1;
			if ( *eol == '\n' ) *eol = '\0';
			syntax_error(filename,linenum,"'#include %s' failed", term);
			strcpy(line,"\n");
			if ( old_stack ) global_restore(old_stack);
			return FALSE;
		}
	}
	else if ( strncmp(line, "#output", 7) == 0 )
	{
		char name[1024];
		char options[1024] = "";
		if ( sscanf(line+7,"%*[ \t]\"%[^\"]\"%*[ \t]%[^\n]",name,options) < 1 )
		{
			output_error_raw("%s(%d): #output missing filename",filename,linenum);
			return FALSE;
		}
		save_on_exit(name,options);
		strcpy(line,"\n");
		return TRUE;
	}
	else if ( strncmp(line, "#input", 6) == 0 )
	{
		char name[1024];
		char options[1024] = "";
		if ( sscanf(line+6,"%*[ \t]\"%[^\"]\"%*[ \t]%[^\n]",name,options) < 1 )
		{
			output_error_raw("%s(%d): #input missing filename",filename,linenum);
			return FALSE;
		}
		char *ext = strrchr(name,'.');
		char oldvalue[1024] = "";
		char varname[1024] = "";
		if ( ext && strcmp(ext,".glm") != 0 )
		{
			if ( strcmp(options,"") != 0 )
			{
				snprintf(varname,sizeof(varname)-1,"%s_load_options",ext+1);
				int old_global_strictnames = global_strictnames;
				if ( global_isdefined(varname) )
				{
					global_getvar(varname,oldvalue,sizeof(oldvalue));
				}
				else
				{
					global_strictnames = FALSE;
				}
				global_setvar(varname,options,NULL);
				global_strictnames = old_global_strictnames;
			}
		}
		char glmname[1024] = "";
		if ( load_import(name,glmname,sizeof(glmname)) == FAILED )
		{
			output_error_raw("%s(%d): load of '%s' failed",filename,linenum,glmname);
			return FALSE;
		}
		if ( strcmp(varname,"") != 0 && strcmp(oldvalue,"") != 0)
		{
			global_setvar(varname,oldvalue,NULL);
		}
		output_verbose("loading converted file '%s'...", glmname);
		strcpy(line,"\n");
		return loadall_glm(glmname);
	}
	else if (strncmp(line, "#setenv",7)==0)

	{
		char *term = strchr(line+7,' ');
		char value[65536];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#setenv macro missing term");
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		var = strtok_r(value, "=", &save);
                    val = strtok_r(NULL, "=", &save);
                    setenv(var, val, 1);
		strcpy(line,"\n");
		return SUCCESS;
	}
	else if (strncmp(line,"#set",4)==0)
	{
		char *term = strchr(line+4,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#set macro missing term");
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		STATUS result;
		if (strchr(value,'=')==NULL)
		{
			syntax_error(filename,linenum,"#set missing assignment");
			return FAILED;
		}
		else
		{
			int oldstrict = global_strictnames;
			global_strictnames = TRUE;
			result = global_setvar(value);
			global_strictnames = strncmp(value,"strictnames=",12)==0 ? global_strictnames : oldstrict;
			if (result==FAILED)
				syntax_error(filename,linenum,"#set term not found");
			strcpy(line,"\n");
			return result==SUCCESS;
		}
	}
	else if (strncmp(line,"#binpath",8)==0)
	{
		output_error("#binpath is no longer supported, use PATH environment variable instead");
		return FALSE;
	}
	else if (strncmp(line,"#libpath",8)==0)
	{
		output_error("#libpath is no longer supported, use LDFLAGS environment variable instead");
		return FALSE;
	}
	else if (strncmp(line,"#incpath",8)==0)
	{
		output_error("#incpath is no longer supported, use CXXFLAGS environment variable instead");
		return FALSE;
	}
	else if (strncmp(line,"#define",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		STATUS result;
		int oldstrict = global_strictnames;
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#define macro missing term");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		if (strchr(value,'=')==NULL)
			strcat(value,"="); // void entry
		global_strictnames = FALSE;
		result = global_setvar(value,"\"\""); // extra "" is used in case value is term is empty string
		global_strictnames = oldstrict;
		if (result==FAILED)
			syntax_error(filename,linenum,"#define term not found");
		strcpy(line,"\n");
		return result==SUCCESS;
	}
	else if (strncmp(line,"#print",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#print missing message text");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		output_message("%s(%d): %s", filename, linenum, value);
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#verbose",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#print missing message text");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		output_verbose("%s(%d): %s", filename, linenum, value);
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#error",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#error missing expression");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		output_error_raw("%s(%d):\t%s", filename, linenum, value);
		strcpy(line,"\n");
		return FALSE;
	}
	else if (strncmp(line,"#warning",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#warning missing message text");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		output_warning("%s(%d): %s", filename, linenum, value);
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#debug",6)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#debug missing message text");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): %s", filename, linenum, value);
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,"#system",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#system missing system call");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): executing system(char *cmd='%s')", filename, linenum, value);
		int rc = my_instance->subcommand(value);
		if( rc == 127 || rc == -1 )
		{
			syntax_error(filename,linenum,"#system %s -- system('%s') failed with status %d", value, value, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if ( strncmp(line,"#command",8) == 0 )
	{
		char *command = strchr(line+8,' ');
		if ( command == NULL )
		{
			syntax_error(filename,linenum,"#command missing call");
			strcpy(line,"\n");
			return FALSE;
		}
		while ( isspace(*command) && *command != '\0' )
		{
			command++;
		}
		char command_line[4096];
		snprintf(command_line,sizeof(command_line)-1,"%s/gridlabd-%s",global_execdir,command);
		output_verbose("executing system(%s)", command_line);
		global_return_code = my_instance->subcommand("%s",command_line);
		if( global_return_code != 0 )
		{
			syntax_error(filename,linenum,"#command %s -- system('%s') failed with status %d", command, command_line, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if (strncmp(line,"#exec",5)==0)
	{
		char *term = strchr(line+5,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#exec missing system call");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): executing system(char *cmd='%s')", filename, linenum, value);
		global_return_code = my_instance->subcommand("%s",value);
		if( global_return_code != 0 )
		{
			syntax_error(filename,linenum,"#exec %s -- system('%s') failed with status %d", value, value, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if (strncmp(line,"#exec",5)==0)
	{
		char *term = strchr(line+5,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#system missing system call");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): executing system(char *cmd='%s')", filename, linenum, value);
		global_return_code = system(value);
		if( global_return_code != 0 )
		{
			syntax_error(filename,linenum,"error executing system(char *cmd='%s') -> non-zero exit code (status=%d)", value, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if (strncmp(line,"#gridlabd",9)==0)
	{
		char *term = strchr(line+9,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#gridlabd missing command line");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): executing system(char *cmd='%s %s')", filename, linenum, global_execname, value);
		global_return_code = my_instance->subcommand("%s %s",global_execname,value);
		if( global_return_code != 0 )
		{
			syntax_error(filename,linenum,"#gridlabd %s -- system('%s %s') failed with status %d", value, global_execname, value, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if (strncmp(line,"#start",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#start missing system call");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		IN_MYCONTEXT output_debug("%s(%d): executing system(char *cmd='%s')", filename, linenum, value);
		if( start_process(value)==NULL )
		{
			syntax_error(filename,linenum,"#start %s -- failed", value);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if ( strncmp(line,"#option",7)==0 )
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			syntax_error(filename,linenum,"#option missing command option name");
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		strcpy(line,"\n");
		return cmdarg_runoption(value)>=0;
	}
	else if ( strncmp(line,"#wget",5)==0 || strncmp(line,"#curl",5)==0 )
	{
		char url[1024], file[1024];
		size_t n = sscanf(line+5,"%s %[^\n\r]",url,file);
		strcpy(line,"\n");
		if ( n<1 )
		{
			syntax_error(filename,linenum,"#wget missing url");
			return FALSE;
		}
		else if ( n==1 )
		{
			char *basename = strrchr(url,'/');
			if ( basename==NULL )
			{
				syntax_error(filename,linenum,"unable to extract basename of URL '%s'", url);
				return FALSE;
			}
			strncpy(file,basename+1,sizeof(file)-1);
		}
		try
		{
			GldCurl(url,file);
		}
		catch (const char *msg)
		{
			output_warning("GldCurl(remote='%s', local='%s') failed: reverting to insecure http_saveas() call", url,file);
			if ( http_saveas(url,file)==0 )
			{
				syntax_error(filename,linenum,"unable to save URL '%s' as '%s'", url, file);
				return FALSE;
			}
		}
		return TRUE;
	}
	else if ( strncmp(line,"#sleep",6)==0 )
	{
		int msec = atoi(line+6);
		IN_MYCONTEXT output_debug("sleeping %.3f seconds...",msec/1000.0);
		exec_sleep(msec*1000);
		strcpy(line,"\n");
		return TRUE;
	}
	else if ( strncmp(line, "#version", 8) == 0 )
	{
		if ( ! version_check(line+9) )
		{
			syntax_error(filename,linenum,"version '%d.%d.%d-%d-%s' does not satisfy the version requirement",
				global_version_major, global_version_minor, global_version_patch, global_version_build, global_version_branch);
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(line,"\n");
		return TRUE;

	}
	else if ( strncmp(line,"#on_exit",8) == 0 )
	{
		int xc;
		char cmd[1024];
		if ( sscanf(line+8,"%d %1023[^\n]",&xc,cmd) < 2 )
		{
			syntax_error(filename,linenum,"#on_exit syntax error");
			return FALSE;
		}
		else if ( ! my_instance->add_on_exit(xc,cmd) )
		{
			syntax_error(filename,linenum,"#on_exit %d command '%s' failed", xc,cmd);
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if ( strncmp(line, "#begin",6) == 0 )
	{
		char name[256];
		if ( sscanf(line+7,"%s",name) == 0 )
		{
			syntax_error(filename,linenum,"#begin macro missing language term");
			return FALSE;
		}
		strcpy(line,"\n");
		return set_language(name);
	}
	else if ( strncmp(line, "#for",4) == 0 )
	{
		char var[64], range[1024];
		if ( sscanf(line+4,"%s in %[^\n]",var,range) == 2 )
		{
			strcpy(line,"\n");
			return for_open(var,range) ? TRUE : FALSE;
		}
		else
		{
			syntax_error(filename,linenum,"for macro syntax error");
			return FALSE;
		}
	}
	else if ( strncmp(line, "#write", 6) == 0 )
	{
		char fname[1024], specs[1024];
		if ( sscanf(line+6,"%s %s",fname,specs) == 2)
		{
			if ( writefile(fname,specs) < 0 )
			{
				syntax_error(filename,linenum,"write macro failed");
				return FALSE;
			}
			strcpy(line,"\n");
			return TRUE;
		}
		else
		{
			syntax_error(filename,linenum,"write macro syntax error");
			return FALSE;
		}
	}
	else if ( strncmp(line, "#read", 5) == 0 )
	{
		char fname[1024], specs[1024];
		if ( sscanf(line+5,"%s %s",fname,specs) == 2)
		{
			strcpy(line,"\n");
			return readfile(fname,specs,line,size-1) >= 0 ? TRUE : FALSE;
		}
		else
		{
			syntax_error(filename,linenum,"read macro syntax error");
			return FALSE;
		}
	}
	int rc = my_instance->subcommand("%s/" PACKAGE "-%s",global_execdir,strchr(line,'#')+1);
	if ( rc != 127 )
	{
		if ( rc != 0 )
		{
			syntax_error(filename,linenum,"macro subcommand failed with exit code %d",rc);
			return FALSE;
		}
		strcpy(line,"\n");
		return TRUE;
	}
	else
	{
		char tmp[1024], *p;
		strncpy(tmp,line,sizeof(tmp)-1);
		for ( p=tmp ; *p!='\0' ; p++ )
		{
			if ( isspace(*p) )
			{
				*p = '\0';
				break;
			}
		}
		syntax_error(filename,linenum,"%s macro is not recognized",tmp);
		strcpy(line,"\n");
		return FALSE;
	}
}


/**/
STATUS GldLoader::loadall_glm(const char *fname) /**< a pointer to the first character in the file name string */

{
	char file[1024];
	strcpy(file,fname);
	OBJECT *obj, *first = object_get_first();
	char *p = NULL;
	char buffer[20480];
	int fsize = 0;
	STATUS status=FAILED;
	struct stat stat;
	FILE *fp;
	int move = 0;
	errno = 0;

	fp = fopen(file,"rt");
	if (fp==NULL)
		goto Failed;
	if (fstat(fileno(fp),&stat)==0)
	{
		modtime = stat.st_mtime;
		fsize = stat.st_size;
	}
	if(fsize <= 1){
		// empty file short circuit
		return SUCCESS;
	}
	IN_MYCONTEXT output_verbose("file '%s' is %d bytes long", file,fsize);
	add_depend(filename,file);

	/* removed malloc check since it doesn't malloc any more */
	buffer[0] = '\0';

	move = buffer_read_alt(fp, buffer, file, 20479);
	while(move > 0){
		p = buffer; // grab a block
		while(*p != 0){
			// and process it
			move = gridlabd_file(p);
			if(move == 0)
				break;
			p += move;
		}
		if(*p != 0){
			// failed if we didn't parse the whole thing
			status = FAILED;
			break;
		}
		move = buffer_read_alt(fp, buffer, file, 20479);
	}

	if(p != 0){ /* did the file contain anything? */
		status = (*p=='\0' && !include_fail) ? SUCCESS : FAILED;
	} else {
		status = FAILED;
	}
	if (status==FAILED)
	{
		char *eol = NULL;
		if(p){
			eol = strchr(p,'\n');
		}
		else
		{
			static char *nulstr = NULL;
			if ( nulstr == NULL )
			{
				nulstr = strdup("");
			}
			p = nulstr;
		}
		if (eol!=NULL){
			*eol='\0';
		}
		syntax_error(file,linenum,"load failed at or near '%.12s...'",*p=='\0'?"end of line":p);
		if (p==0)
			output_error("%s doesn't appear to be a GLM file", file);
		goto Failed;
	}
	else if ((status=load_resolve_all())==FAILED)
		goto Failed;

	/* establish ranks */
	for (obj=first?first:object_get_first(); obj!=NULL; obj=obj->next)
		object_set_parent(obj,obj->parent);
	IN_MYCONTEXT output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
	goto Done;
Failed:
	if (errno!=0){
		output_error("unable to load '%s': %s", file, errno?strerror(errno):"(no details)");
		/*	TROUBLESHOOT
			In most cases, strerror(errno) will claim "No such file or directory".  This claim should be ignored in
			favor of prior error messages.
		*/
	}
Done:
	//free(buffer);
	free_index();
	linenum=1; // parser starts at one
	if (fp!=NULL) fclose(fp);
	return status;
}

TECHNOLOGYREADINESSLEVEL GldLoader::calculate_trl(void)
{
	char buffer[1024];
	CLASS *oclass;

	// start optimistically
	TECHNOLOGYREADINESSLEVEL technology_readiness_level = TRL_PROVEN;

	// examine each class loaded
	for ( oclass=class_get_first_class() ; oclass!=NULL ; oclass=oclass->next )
	{
		// if class is inferior
		if ( oclass->profiler.numobjs>0 && oclass->trl<technology_readiness_level )
		{

			// downgrade trl
			technology_readiness_level = oclass->trl;
			IN_MYCONTEXT output_verbose("class '%s' TRL is %d", oclass->name, oclass->trl);
		}
	}
	IN_MYCONTEXT output_verbose("model TRL is %s", global_getvar("technology_readiness_level",buffer,sizeof(buffer)));
	return technology_readiness_level;
}

/** convert a non-GLM file to GLM, if possible */
bool GldLoader::load_import(const char *from, char *to, int len)
{
	const char *ext = strrchr(from,'.');
	if ( ext == NULL )
	{
		output_error("load_import(from='%s',...): invalid extension", from);
		return false;
	}
	else
	{
		ext++;
	}
	char converter_name[1024], converter_path[1024];
	sprintf(converter_name,"%s2glm.py",ext);
	if ( find_file(converter_name, NULL, R_OK, converter_path, sizeof(converter_path)) == NULL )
	{
		output_error("load_import(from='%s',...): converter %s2glm.py not found", from, ext);
		return false;
	}
	if ( strlen(from) >= (size_t)(len-1) )
	{
		output_error("load_import(from='%s',...): 'from' is too long to handle", from);
		return false;
	}
	strcpy(to,from);
	char *glmext = strrchr(to,'.');
	if ( glmext == NULL )
		strcat(to,".glm");
	else
		strcpy(glmext,".glm");
	char load_options[1024] = "";
	char load_options_var[64];
	sprintf(load_options_var,"%s_load_options",ext);
	global_getvar(load_options_var,load_options,sizeof(load_options));
	char *unquoted = load_options;
	if ( load_options[0] == '"' )
	{
		int len = strlen(load_options);
		load_options[len-1] = '\0';
		unquoted++;
	}
	char *out = strncmp(unquoted,"-o ",3)==0 ? unquoted : strstr(unquoted," -o ");
	if ( out )
	{	// copy user-specified output glm name
		while ( isspace(out[0]) ) out++;
		if ( strchr(out,' ') )
		{
			strcpy(to,strchr(out,' ')+1);
			char *end = strchr(to,' ');
			if ( end ) *end = '\0';
		}
		else
		{
			output_warning("-o option filename missing");
		}
		output_verbose("changing output to '%s'", to);
	}
	int rc = my_instance->subcommand("%s %s -i %s -o %s %s",(const char*)global_pythonexec,converter_path,from,to,unquoted);
	if ( rc != 0 )
	{
		output_error("%s: return code %d",converter_path,rc);
		return false;
	}
	output_verbose("GldLoader::load_import(from='%s', to='%s', len=%d) -> OK load_options='%s'",from,to,len,load_options);
	return true;
}

STATUS GldLoader::load_python(const char *filename)
{
	extern PyObject *gridlabd_module;
	if ( gridlabd_module == NULL )
	{
		python_embed_init(0,NULL);
	}
	return python_embed_import(filename,global_pythonpath) == NULL ? FAILED : SUCCESS;
}

/** Load a file
	@return STATUS is SUCCESS if the load was ok, FAILED if there was a problem
	@todo Rollback the model data if the load failed (ticket #32)
	@todo Support nested loads and maintain context during subloads (ticket #33)
 **/
STATUS GldLoader::loadall(const char *fname)
{
	try
	{
		/* if nothing requested only config files are loaded */
		if ( fname == NULL )
		{
			return SUCCESS;
		}

		char file[1024] = "";
		if ( fname )
		{
			strcpy(file,fname);
		}
		char *ext = fname ? strrchr(file,'.') : NULL ;
		if ( ext == NULL )
		{
			output_error("'%s' is not valid", fname);
			return FAILED;
		}
		add_depend(filename,fname);

		// python script

		if ( ext != NULL && ( strcmp(ext,".py") == 0 || strncmp(ext,".py ",4) == 0 || strncmp(ext,".py\t",4) == 0 ) )
		{
			return load_python(fname);
		}

		// non-glm file
		if ( ext != NULL && strcmp(ext,".glm") != 0 )
		{
			return load_import(fname,file,sizeof(file)) ? loadall_glm(file) : FAILED;
		}

		// glm file
		unsigned int old_obj_count = object_get_count();
		char conf[1024];
		static int loaded_files = 0;
		STATUS load_status = FAILED;

		if ( old_obj_count > 1 && global_forbid_multiload )
		{
			output_error("loadall: only one file load is supported at this time.");
			return FAILED; /* not what they expected--do not proceed */
		}

		/* first time only */
		if ( loaded_files == 0 )
		{
			/* load the gridlabd.conf file */
			if (find_file("gridlabd.conf",NULL,R_OK,conf,sizeof(conf))==NULL)
			{
				output_warning("gridlabd.conf was not found");
				/* TROUBLESHOOT
					The <code>gridlabd.conf</code> was not found in the <b>GLPATH</b> environment path.
					This file is always loaded before a GLM file is loaded.
					Make sure that <b>GLPATH</b> includes the <code>.../etc</code> folder and try again.
				 */
			}
			else
			{
				add_depend(fname,"gridlabd.conf");
				strcpy(filename, "gridlabd.conf");
				if ( loadall_glm(conf)==FAILED )
				{
					return FAILED;
				}
			}

			/* load the debugger.conf file */
			if (global_debug_mode)
			{
				char dbg[1024];

				if (find_file("debugger.conf",NULL,R_OK,dbg,sizeof(dbg))==NULL)
				{
					output_warning("debugger.conf was not found");
					/* TROUBLESHOOT
						The <code>debugger.conf</code> was not found in the <b>GLPATH</b> environment path.
						This file is loaded when the debugger is enabled.
						Make sure that <b>GLPATH</b> includes the <code>.../etc</code> folder and try again.
					 */
				}
				else if (loadall_glm(dbg)==FAILED)
				{
					return FAILED;
				}
			}
		}

		/* handle default extension */
		strcpy(filename,file);
		if ( ext == NULL || ext < file+strlen(file)-5 )
		{
			ext = filename+strlen(filename);
			strcat(filename,".glm");
		}

		/* load the appropriate type of file */
		if (global_streaming_io_enabled || (ext!=NULL && isdigit(ext[1])) )
		{
			FILE *fp = fopen(file,"rb");
			if (fp==NULL || stream(fp,SF_IN)<0)
			{
				output_error("%s: unable to read stream", file);
				return FAILED;
			}
			else
			{
				load_status = SUCCESS;
			}
		}
		else if (ext==NULL || strcmp(ext, ".glm")==0)
		{
			load_status = loadall_glm(filename);
		}
		else
		{
			output_error("%s: unable to load unknown file type", filename, ext);
		}

		/* objects should not be started until all deferred schedules are done */
		if ( global_threadcount>1 )
		{
			if ( schedule_createwait()==FAILED )
			{
				syntax_error(filename,linenum,"load failed on schedule error");
				return FAILED;
			}
		}

		calculate_trl();

		loaded_files++;
		return load_status;
	}
	catch (const char *message)
	{
		output_error_raw("%s(%d): %s", filename, linenum, message);
		return FAILED;
	}
	catch (GldException *error)
	{
		output_error_raw("%s", error->get_message());
		delete error;
		return FAILED;
	}
	catch (...)
	{
		output_error_raw("%s(%d): unknown loader exception caught", filename, linenum);
		return FAILED;
	}
}

std::string GldLoader::get_depends(const char *format)
{
	if ( format == NULL || strcmp(format,"makefile") == 0 )
	{
		std::string result = std::string("# generated by gridlabd ") + global_version + "\n\n"
			+ "all: " + global_modelname + "\n\n";

		for ( DEPENDENCY_TREE::iterator item = dependency_tree.begin() ; item != dependency_tree.end() ; item++ )
		{
			result.append(item->first + ": ");
			item->second.unique();
			for ( std::list<std::string>::iterator target = item->second.begin() ; target != item->second.end() ; target++ )
				result.append(*target + " ");
			result.append("\n\n");
		}
		return result;
	}
	else if ( strcmp(format,"json") == 0 )
	{
		std::string result = std::string("{\n");
		for ( DEPENDENCY_TREE::iterator item = dependency_tree.begin() ; item != dependency_tree.end() ; item++ )
		{
			if ( item != dependency_tree.begin() )
				result.append(",\n");
			result.append("\t\"");
			result.append(item->first);
			result.append("\" : [");
			item->second.unique();
			for ( std::list<std::string>::iterator target = item->second.begin() ; target != item->second.end() ; target++ )
			{
				if ( target != item->second.begin() )
					result.append(",");
				result.append("\"");
				result.append(*target);
				result.append("\"");
			}
			result.append("]");
		}
		result.append("\n}\n");
		return result;
	}
	else
	{
		throw new GldException("GldLoader::get_depends(format='%s'): invalid format",format);
	}
}

void GldLoader::add_depend(const char *filename, const char *dependency)
{
	if ( strcmp(filename,"") == 0 )
		filename = global_modelname;

	std::list<std::string> &item = dependency_tree[filename];
	if ( strcmp(filename,dependency) != 0 )
	{
		item.push_back(dependency);
	}
}
