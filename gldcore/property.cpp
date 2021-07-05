/** property.cpp
	Copyright (C) 2011 Battelle Memorial Institute
	@file property.c
	@addtogroup property Properties of objects
	@ingroup core
	
	GridLAB-D classes contain properties,
	which are supported by the functions in this module

 @{
 **/

#include "gldcore.h"

SET_MYCONTEXT(DMC_PROPERTY)

double complex_get_part(void *x, const char *name);

/* IMPORTANT: this list must match PROPERTYTYPE enum in property.h */
PROPERTYSPEC property_type[_PT_LAST] = {
	{"void", "string", NULL, 0, 0, convert_from_void,convert_to_void},
	{"double", "decimal", "0.0", sizeof(double), 24, convert_from_double,convert_to_double,initial_from_double,NULL,NULL,convert_to_double,{TCOPS(double)},},
	{"complex", "string", "0+0i", sizeof(complex), 48, convert_from_complex,convert_to_complex,NULL,NULL,NULL,convert_to_complex,{TCOPS(double)},complex_get_part,complex_set_part},
	{"enumeration", "string", "0", sizeof(enumeration), 1024, convert_from_enumeration,convert_to_enumeration,NULL,NULL,NULL,convert_to_enumeration,{TCOPS(uint64)},},
	{"set", "string", "0", sizeof(set), 1024, convert_from_set,convert_to_set,NULL,NULL,NULL,convert_to_set,{TCOPS(uint64)},},
	{"int16", "integer", "0", sizeof(int16), 6, convert_from_int16,convert_to_int16,NULL,NULL,NULL,convert_to_int16,{TCOPS(uint16)},},
	{"int32", "integer", "0", sizeof(int32), 12, convert_from_int32,convert_to_int32,NULL,NULL,NULL,convert_to_int32,{TCOPS(uint32)},},
	{"int64", "integer", "0", sizeof(int64), 24, convert_from_int64,convert_to_int64,NULL,NULL,NULL,convert_to_int64,{TCOPS(uint64)},},
	{"char8", "string", "", sizeof(char8), 8, convert_from_char8,convert_to_char8,NULL,NULL,NULL,convert_to_char8,{TCOPS(string)},},
	{"char32", "string", "", sizeof(char32), 32, convert_from_char32,convert_to_char32,NULL,NULL,NULL,convert_to_char32,{TCOPS(string)},},
	{"char256", "string", "", sizeof(char256), 256, convert_from_char256,convert_to_char256,NULL,NULL,NULL,convert_to_char256,{TCOPS(string)},},
	{"char1024", "string", "", sizeof(char1024), 1024, convert_from_char1024,convert_to_char1024,NULL,NULL,NULL,convert_to_char1024,{TCOPS(string)},},
	{"object", "string", NULL, sizeof(OBJECT*), 64, convert_from_object,convert_to_object,NULL,NULL,NULL,convert_to_object,{TCOPB(object)},object_get_part,object_set_part},
	{"delegated", "string", NULL, 0, PSZ_DYNAMIC, convert_from_delegated, convert_to_delegated},
	{"bool", "string", "FALSE", sizeof(bool), 6, convert_from_boolean, convert_to_boolean,NULL,NULL,NULL,convert_to_boolean,{TCOPB(bool)},},
	{"timestamp", "string", "0", sizeof(int64), 32, convert_from_timestamp_stub, convert_to_timestamp_stub,NULL,NULL,NULL,convert_to_timestamp_stub,{TCOPS(uint64)},timestamp_get_part,timestamp_set_part},
	{"double_array", "string", "", sizeof(double_array), 1024, convert_from_double_array, convert_to_double_array,NULL,double_array_create,NULL,NULL,{TCNONE},double_array_get_part,NULL},
	{"complex_array", "string", "", sizeof(complex_array), 1024, convert_from_complex_array, convert_to_complex_array,NULL,complex_array_create,NULL,NULL,{TCNONE},complex_array_get_part,NULL},
	{"real", "decimal", "0.0", sizeof(real), 24, convert_from_real, convert_to_real},
	{"float", "decimal", "0.0", sizeof(float), 24, convert_from_float, convert_to_float},
	{"loadshape", "string", NULL, sizeof(loadshape), 1024, convert_from_loadshape, convert_to_loadshape, initial_from_loadshape,loadshape_create,NULL,convert_to_double,{TCOPS(double)},},
	{"enduse", "string", NULL, sizeof(enduse), 1024, convert_from_enduse, convert_to_enduse, initial_from_enduse,enduse_create,NULL,convert_to_double,{TCOPS(double)},enduse_get_part,enduse_set_part},
	{"randomvar", "string", NULL, sizeof(randomvar), 24, convert_from_randomvar, convert_to_randomvar, initial_from_randomvar,randomvar_create,NULL,convert_to_double,{TCOPS(double)},random_get_part,random_set_part},
	{"method","string", NULL, 0, PSZ_DYNAMIC, convert_from_method,convert_to_method,initial_from_method},
	{"string", "string", "", sizeof(STRING), PSZ_AUTO, convert_from_string, convert_to_string, NULL,string_create,NULL,convert_to_string,{TCOPS(string)},},
	{"property", "string", NULL, sizeof(OBJECTPROPERTY*), 256, convert_from_propertyref, convert_to_propertyref, initial_from_propertyref, propertyref_create, NULL, convert_to_propertyref, {TCNONE},        },
	{"python", "string", "None", sizeof(PyObject**), PSZ_DYNAMIC, convert_from_python, convert_to_python, initial_from_python, python_create,NULL,convert_to_python,{TCNONE},python_get_part,NULL},
};

PROPERTYTYPE property_getfirst_type(void)
{
	return (PROPERTYTYPE)((int)PT_void+1);
}

PROPERTYTYPE property_getnext_type(PROPERTYTYPE ptype)
{
	return ptype > _PT_FIRST && ptype < _PT_LAST-1 ? (PROPERTYTYPE)((int)ptype+1) : PT_void;
}

PROPERTYSPEC *property_getspec(PROPERTYTYPE ptype)
{
	return &(property_type[ptype]);
}
const char *property_getdefault(PROPERTYTYPE ptype)
{
	return property_type[ptype].default_value;
}

/** Check whether the properties as defined are mapping safely to memory
    @return 0 on failure, 1 on success
 **/
int property_check(void)
{
	PROPERTYTYPE ptype;
	int status = 1;
	for ( ptype = PROPERTYTYPE(_PT_FIRST+1) ; ptype < _PT_LAST ; ptype = PROPERTYTYPE(ptype+1) )
	{
		size_t sz = 0;
		switch (ptype) {
		case PT_double: sz = sizeof(double); break;
		case PT_complex: sz = sizeof(complex); break;
		case PT_enumeration: sz = sizeof(enumeration); break;
		case PT_set: sz = sizeof(set); break;
		case PT_int16: sz = sizeof(int16); break;
		case PT_int32: sz = sizeof(int32); break;
		case PT_int64: sz = sizeof(int64); break;
		case PT_char8: sz = sizeof(char8); break;
		case PT_char32: sz = sizeof(char32); break;
		case PT_char256: sz = sizeof(char256); break;
		case PT_char1024: sz = sizeof(char1024); break;
		case PT_object: sz = sizeof(OBJECT*); break;
		case PT_bool: sz = sizeof(bool); break;
		case PT_timestamp: sz = sizeof(TIMESTAMP); break;
		case PT_double_array: sz = sizeof(double_array); break;
		case PT_complex_array: sz = sizeof(complex_array); break;
		case PT_real: sz = sizeof(real); break;
		case PT_float: sz = sizeof(float); break;
		case PT_loadshape: sz = sizeof(loadshape); break;
		case PT_enduse: sz = sizeof(enduse); break;
		case PT_random: sz = sizeof(randomvar); break;
		default: break;
		}
		IN_MYCONTEXT output_verbose("property_check of %s: declared size is %d, actual size is %d", property_type[ptype].name, property_type[ptype].size, sz);
		if ( sz>0 && property_type[ptype].size<sz )
		{
			status = 0;
			output_error("declared size of property %s smaller than actual size in memory on this platform (declared %d, actual %d)", property_type[ptype].name, property_type[ptype].size, sz);
		}
		else if ( sz>0 && property_type[ptype].size!=sz )
		{
			output_warning("declared size of property %s does not match actual size in memory on this platform (declared %d, actual %d)", property_type[ptype].name, property_type[ptype].size, sz);
		}
	}
	return status;
}

PROPERTY *property_malloc(PROPERTYTYPE proptype, CLASS *oclass, const char *name, void *addr, DELEGATEDTYPE *delegation)
{
	char unitspec[1024];
	char propname[1024];
	PROPERTY *prop = (PROPERTY*)malloc(sizeof(PROPERTY));

	if (prop==NULL)
	{
		output_error("property_malloc(oclass='%s',...): memory allocation failed", oclass->name, name);
		/*	TROUBLESHOOT
			This means that the system has run out of memory while trying to define a class.  Trying freeing
			up some memory by unloading applications or configuring your system so it has more memory.
		 */
		errno = ENOMEM;
		goto Error;
	}
	memset(prop, 0, sizeof(PROPERTY));
	prop->ptype = proptype;
	prop->size = 0;
	prop->width = property_type[proptype].size;
	prop->access = PA_PUBLIC;
	prop->oclass = oclass;
	prop->flags = 0;
	prop->keywords = NULL;
	prop->description = NULL;
	prop->unit = NULL;
	prop->notify = 0;
	prop->notify_override = false;
	prop->default_value = NULL;
	if (sscanf(name,"%[^[][%[^]]]",propname,unitspec)==2)
	{
		/* detect when a unit is associated with non-double/complex property */
		if (prop->ptype!=PT_double && prop->ptype!=PT_complex)
			output_error("property_malloc(oclass='%s',...): property %s cannot have unit '%s' because it is not a double or complex value",oclass->name, prop->name,unitspec);
			/*	TROUBLESHOOT
				Only <b>double</b> and <b>complex</b> properties can have units.  
				Either change the type of the property or remove the unit specification from the property's declaration.
			 */

		/* verify that the requested unit exists or can be derived */
		else 
		{
			if ((prop->unit = unit_find(unitspec))==NULL)
				throw_exception("property_malloc(oclass='%s',...): property %s unit '%s' is not recognized",oclass->name, prop->name,unitspec);
				/*	TROUBLESHOOT
					A class is attempting to publish a variable using a unit that is not defined.  
					This is caused by an incorrect unit specification in a variable publication (in C++) or declaration (in GLM).
					Units are defined in the unit file located in the GridLAB-D <b>etc</b> folder.  
				 */
		}
	}
	prop->name = strdup(propname);
	prop->addr = addr;
	prop->delegation = delegation;
	prop->next = NULL;

	/* check for already existing property by same name */
	if (oclass!=NULL && class_find_property(oclass,prop->name))
		output_warning("property_malloc(oclass='%s',...): property name '%s' is defined more than once", oclass->name, prop->name);
		/*	TROUBLESHOOT
			A class is attempting to publish a variable more than once.  
			This is caused by an repeated specification for a variable publication (in C++) or declaration (in GLM).
		 */
	return prop;
Error:
	free(prop);
	return NULL;
}

/** Get the size of a single instance of a property
	@return the size in bytes of the a property
 **/
uint32 property_size(PROPERTY *prop)
{
	if (prop && prop->ptype > _PT_FIRST && prop->ptype < _PT_LAST)
	{
		return property_type[prop->ptype].size;
	}
	else
	{
		return 0;
	}
}

/** Get the physical address of a property
    If obj==NULL, the property address is assume to be for a global
    @return the physical location in memory of the property
 **/
void *property_addr(OBJECT *obj, PROPERTY *prop) 
{
	return (void*)((char *)(obj!=NULL?(obj+1):NULL)+(int64)(prop->addr));
}

uint32 property_size_by_type(PROPERTYTYPE type)
{
	return property_type[type].size;
}

int property_read(PROPERTY *prop, void *addr, const char *string)
{
	if ( prop->ptype > _PT_FIRST && prop->ptype < _PT_LAST )
	{
		return property_type[prop->ptype].string_to_data ? property_type[prop->ptype].string_to_data(string,addr,prop) : 0;
	}
	else
	{
		output_error("gldcore/property.c:property_read(prop='%s', addr=%p, string=%p): read operation not supported",
			prop->name, addr, string);
		/*	TROUBLESHOOT
			The property in question cannot be converted from a string. Either the property does not have
			an underlying XSD type that can be represented as a string, or the property does not support
			the required operation(s).
		 */
		return 0;
	}
}

int property_write(PROPERTY *prop, void *addr, char *string, size_t size)
{
	if ( prop->ptype > _PT_FIRST && prop->ptype < _PT_LAST && property_type[prop->ptype].data_to_string != NULL )
	{
		return property_type[prop->ptype].data_to_string(string,string?size:0,addr,prop);
	}
	else
	{
		output_error("gldcore/property.c:property_write(prop='%s', addr=%p, string=%p, size=%u): write operation not supported",
			prop->name, addr, string, size);
		/*	TROUBLESHOOT
			The property in question cannot be converted to a string. Either the property does not have
			an underlying XSD type that can be represented as a string, or the property does not support
			the required operation(s).
		 */
		return 0;
	}
}

int property_create(PROPERTY *prop, void *addr)
{
	if (prop && prop->ptype>_PT_FIRST && prop->ptype<_PT_LAST)
	{
		if ( property_type[prop->ptype].create != NULL )
		{
			int result = property_type[prop->ptype].create(addr);
			if ( result != 0 )
			{
				const char *tmp = prop->default_value ? prop->default_value : property_type[prop->ptype].default_value;
				if ( tmp != NULL )
				{
					if ( property_read(prop,addr,tmp) == 0 )
					{
						output_error("property '%s' default value '%s' is invalid", prop->name, tmp);
						return 0;
					}
					else
					{
						return 1;
					}
				}
			}
			return result;
		}
		else if ( (int)property_type[prop->ptype].size > 0 )
		{
			const char *tmp = prop->default_value ? prop->default_value : property_type[prop->ptype].default_value;
			if ( tmp != NULL )
			{
				if ( property_read(prop,addr,tmp) == 0 )
				{
					output_error("property '%s' default value '%s' is invalid", prop->name, tmp);
					return 0;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				memset(addr,0,property_type[prop->ptype].size);
				return 1;
			}
		}
		else // zero-size object
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

size_t property_minimum_buffersize(PROPERTY *prop)
{
	size_t size = property_type[prop->ptype].csize;
	if ( size>0 )
	{
		return size;
	}
	switch (prop->ptype) {
	/* @todo dynamic sizing */
	default:
		return 0;
	}
	return 0;
}

PROPERTYCOMPAREOP property_compare_op(PROPERTYTYPE ptype, const char *opstr)
{
	PROPERTYCOMPAREOP n;
	for ( n = _TCOP_FIRST ; n < _TCOP_LAST ; n = PROPERTYCOMPAREOP(n+1) )
	{
		if ( strcmp(property_type[ptype].compare[n].str,opstr) == 0 )
		{
			return n;
		}
	}
	return TCOP_ERR;
}

bool property_compare_basic(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void *x, void *a, void *b, const char *part)
{
	if ( part==NULL && property_type[ptype].compare[op].fn!=NULL )
	{
		return property_type[ptype].compare[op].fn(x,a,b);
	}
	else if ( property_type[ptype].get_part!=NULL && part != NULL )
	{
		double d = property_type[ptype].get_part ? property_type[ptype].get_part(x,part) : QNAN ;
		if ( isfinite(d) )
		{
			// parts always double (for now)
			return property_type[PT_double].compare[op].fn((void*)&d,a,b);
		}
		else
		{
			output_error("part %s is not defined for type %s", part, property_type[ptype].name);
			return 0;
		}
	}
	else // no comparison possible
	{
		output_warning("property type '%s' does not support comparison operations or parts", property_type[ptype].name);
		return 0;
	}
}

bool property_compare_basic_str(PROPERTY *prop, PROPERTYCOMPAREOP op, void *x, const char *a, const char *b, const char *part)
{
	// convert a and b to their respective underlying property values
	char *xa[property_type[prop->ptype].size];
	char *xb[property_type[prop->ptype].size];
	if ( property_type[prop->ptype].string_to_compare == NULL )
		return false;
	if ( property_type[prop->ptype].string_to_compare(a,(void*)xa,prop) < 0 )
	{
		output_warning("property type '%s' cannot compare to value '%s'", property_type[prop->ptype].name,a);
		return false;
	}
	else if ( b != NULL && property_type[prop->ptype].string_to_compare(b,(void*)xb,prop) < 0 )
	{
		output_warning("property type '%s' cannot compare to value '%s'", property_type[prop->ptype].name,b);
		return false;
	}
	else
	{
		return property_compare_basic(prop->ptype,op,x,(void*)xa,(void*)xb,part);
	}
}

PROPERTYTYPE property_get_type(const char *name)
{
	PROPERTYTYPE ptype;
	for ( ptype = PROPERTYTYPE(_PT_FIRST+1) ; ptype < _PT_LAST ; ptype = PROPERTYTYPE(ptype+1) )
	{
		if ( strcmp(property_type[ptype].name,name)==0)
			return ptype;
	}
	return PT_void;
}

double property_get_part(OBJECT *obj, PROPERTY *prop, const char *part)
{
	PROPERTYSPEC *spec = property_getspec(prop->ptype);
	if ( spec && spec->get_part )
	{
		return spec->get_part(GETADDR(obj,prop),part);
	}
	else
		return QNAN;
}

bool property_is_default(OBJECT *obj, PROPERTY *prop)
{
	void *a = (char*)(obj ? (obj+1) : 0) + (size_t)prop->addr;
	char b[4096]="";
	bool result = false;
	if ( prop->default_value != NULL )
	{
		property_read(prop,b,prop->default_value);
		result = property_compare_basic(prop->ptype,TCOP_EQ,a,b,NULL,NULL);
	}
#ifdef DEBUG
	if ( global_debug_output )
	{
		char buf1[1024] = "<?" "?" "?>", buf2[1024] = "<?" "?" "?>"; // avoid trigraph usage
		property_write(prop,a,buf1,sizeof(buf1));
		property_write(prop,b,buf2,sizeof(buf2));
		if ( obj->name == NULL )
		{
			IN_MYCONTEXT output_debug("comparing %s:%d.%s [%s] == %s:default.%s [%s] --> %s",
				obj->oclass->name, obj->id, prop->name, buf1,
				obj->oclass->name, prop->name, buf2,
				result ? "true" : "false");
		}
		else
		{
			IN_MYCONTEXT output_debug("comparing %s.%s [%s] == %s:default.%s [%s] --> %s",
				obj->name, prop->name, buf1,
				obj->oclass->name, prop->name, buf2,
				result ? "true" : "false");
		}
	}
#endif
	return result;
}

/*********************************************************
 * PROPERTY PARTS
 *********************************************************/
double complex_get_part(void *x, const char *name)
{
	complex *c = (complex*)x;
	if ( strcmp(name,"real")==0) 
	{
		return c->Re();
	}
	if ( strcmp(name,"imag")==0) 
	{
		return c->Im();
	}
	if ( strcmp(name,"mag")==0) 
	{
		return c->Mag();
	}
	if ( strcmp(name,"arg")==0) 
	{
		return c->Arg();
	}
	if ( strcmp(name,"ang")==0) 
	{
		return c->Ang();
	}
	return QNAN;
}
int complex_set_part(void *x, const char *name, const char *value)
{
	complex *c = (complex*)x;
	if ( strcmp(name,"real")==0 ) { c->Re(atof(value)); return 1; } else 
	if ( strcmp(name,"imag")==0 ) { c->Im( atof(value)); return 1; } else
	if ( strcmp(name,"mag") ==0 ) { c->Mag(atof(value));  return 1; } else
	if ( strcmp(name,"arg") ==0 ) { c->Arg(atof(value));  return 1; } else
	if ( strcmp(name,"ang") ==0 ) { c->Ang(atof(value));  return 1; } else
	return 0;
}
int complex_from_string(void *x, const char *str)
{
	double a,b = 0.0;
	char notation[2] = "i";
	complex *c = (complex*)x;
	size_t n = sscanf(str,"%lg%lg%1[ijdr]",&a,&b,notation);
	if ( n == 1 || n == 3 )
	{
		switch (notation[0])
		{
			case 'i':
			case 'j':
				c->SetRect(a,b);
				return 1;
			case 'r':
				c->SetPolar(a,b);
				return 1;
			case 'd':
				c->SetPolar(a,b*180/PI);
				return 1;
			default:
				output_warning("complex_from_string(void *x=%p, char *str='%s') notation '%s' is invalid", x,str,notation);
				return 0;
		}
	}
	IN_MYCONTEXT output_debug("complex_from_string(void *x=%p, char *str='%s') complex parse failed (n=%d)", x,str, n);
	return 0;
}

/*********************************************************
 * DOUBLE ARRAYS
 *********************************************************/
int double_array_create(void*ptr)
{
	double_array *a = (double_array*)ptr;
	a->n = a->m = 0;
	a->max = 1;
	a->x = (double***)malloc(sizeof(double**)*a->max);
	a->f = (unsigned char*)malloc(sizeof(unsigned char)*a->max);
	if ( a->x==NULL || a->f==NULL )
	{
		return 0;
	}
	memset(a->x,0,sizeof(double**)*a->max);
	memset(a->f,0,sizeof(unsigned char)*a->max);
	return 1;
}
double get_double_array_value(double_array*a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
	{
		return *(a->x[n][m]);
	}
	else
	{
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return QNAN;
	}
}
void set_double_array_value(double_array*a,unsigned int n, unsigned int m, double x)
{
	if ( a->n>n && a->m>m )
	{
		*(a->x[n][m])=x;
	}
	else
	{
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return;
	}
}
double *get_double_array_ref(double_array*a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
	{
		return a->x[n][m];
	}
	else
	{
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return NULL;
	}
}
double double_array_get_part(void *x, const char *name)
{
	size_t n,m;
	if (sscanf(name,"%lu.%lu",&n,&m)==2)
	{
		double_array *a = (double_array*)x;
		if ( n<a->n && m<a->m && a->x[n][m]!=NULL )
			return *(a->x[n][m]);
	}
	return QNAN;
}

/*********************************************************
 * COMPLEX ARRAYS
 *********************************************************/
int complex_array_create(void *ptr)
{
	complex_array *a = (complex_array*)ptr;
	a->n = a->m = 0;
	a->max = 1;
	a->x = (complex***)malloc(sizeof(complex**)*a->max);
	a->f = (unsigned char*)malloc(sizeof(unsigned char)*a->max);
	if ( a->x==NULL || a->f==NULL )
	{
		return 0;
	}
	memset(a->x,0,sizeof(complex**)*a->max);
	memset(a->f,0,sizeof(unsigned char)*a->max);
	return 1;
}
complex *get_complex_array_value(complex_array *a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
	{
		return a->x[n][m];
	}
	else
	{
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return NULL;
	}
}
void set_complex_array_value(complex_array *a,unsigned int n, unsigned int m, complex *x)
{
	if ( a->n>n && a->m>m )
	{
		*(a->x[n][m]) = *x;
	}
	else
	{
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return;
	}
}
complex *get_complex_array_ref(complex_array *a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
	{
		return a->x[n][m];
	}
	else
	{
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
		return NULL;
	}
}
double complex_array_get_part(void *x, const char *name)
{
	size_t n,m;
	char subpart[32];
	if (sscanf(name,"%lu.%lu.%31s",&n,&m,subpart)==2)
	{
		complex_array *a = (complex_array*)x;
		if ( n<a->n && m<a->m && a->x[n][m]!=NULL )
		{
			if ( strcmp(subpart,"real")==0 ) 
			{
				return a->x[n][m]->Re();
			}
			else if ( strcmp(subpart,"imag")==0 ) 
			{
				return a->x[n][m]->Im();
			}
			else 
			{
				return QNAN;
			}
		}
	}
	return QNAN;
}

int string_create(void *ptr)
{
	STRING str = new std::string();
	if ( str != NULL )
	{
		*(STRING*)ptr = str;
		return 1;
	}
	else
	{
		output_error("string_create(void *ptr=%p): memory allocation failed",ptr);
		return 0;
	}
}

int convert_to_string(const char *s, void *data, PROPERTY *p)
{
	STRING *str = (STRING*)data;
	**str = s;
	return (*str)->size()+1;
}

int convert_from_string(char *buffer, int len, void *data, PROPERTY *p)
{
	STRING *str = (STRING*)data;
	int n = snprintf(buffer,(size_t)len,"%s",(*str)->c_str());
	return n;
}

int propertyref_create(void *ptr)
{
	OBJECTPROPERTY **ref = (OBJECTPROPERTY**)ptr;
	*ref = (OBJECTPROPERTY*)malloc(sizeof(OBJECTPROPERTY));
	memset(*ref,0,sizeof(OBJECTPROPERTY));
	return sizeof(OBJECTPROPERTY*);
}

int convert_to_propertyref(const char *s, void *data, PROPERTY *p)
{
	output_debug("convert_to_propertyref(s='%s',data=0x%X,p='%s')",s,data,p?p->name:"<none>");
	OBJECTPROPERTY *ref = *(OBJECTPROPERTY**)data;
	char oname[64];
	char pname[64];
	OBJECT *obj;
	PROPERTY *prop;
	
	// attempt to use global reference resolution
	const char *global_token = strstr(s,"::");
	if ( global_token != NULL )
	{
		GLOBALVAR *var = global_find(global_token==s?s+2:s);
		if ( var != NULL )
		{
			ref->obj = NULL;
			ref->prop = var->prop;
			ref->part = NULL;
			return strlen(s);
		}
		else
		{
			output_error("property reference to global '%s' not found",s);
			return -1;
		}
	}

	// attempt to use object reference resolution
	else if ( sscanf(s,"%[^:]:%s",oname,pname) == 2 )
	{
		if ( (obj=object_find_name(oname)) == NULL )
		{
			output_error("object '%s' not found",oname);
			return -1;
		}
		else if ( (prop=object_get_property(obj,pname,NULL)) == NULL )
		{
			output_error("property '%s' not found in object '%s'", pname, oname);
			return -1;
		}
		else
		{
			ref->obj = obj;
			ref->prop = prop;
			return strlen(s);
		}
	}
	else if ( data != NULL )
	{
		output_error("cannot assign value '%s' through property reference '%s'", s, p->name);
		return -1;
	}
	else
	{
		output_error("'%s' is not a invalid property reference", s);
		return -1;
	}
}

int convert_from_propertyref(char *buffer, int len, void *data, PROPERTY *p)
{
	output_debug("convert_from_propertyref(buffer=0x%X,len=%d,data=0x%X,p='%s')",buffer,len,data,p?p->name:"<none>");
	OBJECTPROPERTY *ref = *(OBJECTPROPERTY**)data;
	int result = -1;
	if ( ref->obj == NULL )
	{
		// null reference
		if ( ref->prop == NULL )
		{
			if ( len > 0 )
			{	
				buffer[0] = '\0';
			}
			result = 0;
		}

		// global reference
		else
		{
			// TODO
			result = property_write(ref->prop,data,buffer,len);
		}
	}
	else
	{
		void *addr = (void *)((char *)(ref->obj + 1) + (int64)(ref->prop->addr));
		result = object_get_value_by_addr(ref->obj, addr, buffer, len, ref->prop);
	}
	output_debug("convert_from_propertyref(buffer='%s',len=%d,data=0x%X,p='%s') -> %d",buffer,len,data,p?p->name:"<none>",result);
	return result;
}

int initial_from_propertyref(char *buffer, int len, void *data, PROPERTY *p)
{
	output_debug("initial_from_propertyref(buffer=0x%X,len=%d,data=0x%X,p='%s')",buffer,len,data,p?p->name:"<none>");
	OBJECTPROPERTY *ref = (OBJECTPROPERTY*)data;
	int result = -1;
	if ( ref->obj == NULL )
	{
		// TODO
		return result;
	}
	else
	{
		// object property
		// TODO
		return result;
	}
}

// EOF
