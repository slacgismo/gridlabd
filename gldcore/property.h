/*	File: property.h 
 	Copyright (C) 2008, Battelle Memorial Institute

	Properties are used to manage different types of values in objects.
 */

#ifndef _PROPERTY_H
#define _PROPERTY_H

#if ! defined _GLDCORE_H && ! defined _GRIDLABD_H
#error "this header may only be included from gldcore.h or gridlabd.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "platform.h"
#include "complex.h"
#include "unit.h"

typedef struct s_class_list CLASS;

#ifndef FADDR

#define FADDR /* define only once */

#ifdef __MINGW32__

#warning Temporary hard coded __int64
typedef __int64 (*FUNCTIONADDR)(void*,...);

#else // ! __MINGW32__

/*	Typedef: FUNCTIONADDR
	The entry point of a module function
 */
typedef int64 (*FUNCTIONADDR)(void*,...); 

#endif // __MINGW32__

#endif // FADDR

#ifdef HAVE_STDINT_H

#include <stdint.h>
typedef int8_t    DEPRECATED int8;     /* 8-bit integers */
typedef int16_t   DEPRECATED int16;    /* 16-bit integers */
typedef int32_t   DEPRECATED int32;    /* 32-bit integers */
typedef uint16_t  DEPRECATED uint16;	/* unsigned 16-bit integers */
typedef uint32_t  DEPRECATED uint32;   /* unsigned 32-bit integers */
typedef uint64_t  DEPRECATED uint64;   /* unsigned 64-bit integers */

#else // ! HAVE_STDINT_H

typedef char DEPRECATED int8; /** 8-bit integers */
typedef short DEPRECATED int16; /** 16-bit integers */
typedef int DEPRECATED int32; /* 32-bit integers */
typedef unsigned short DEPRECATED uint16;
typedef unsigned int DEPRECATED uint32; /* unsigned 32-bit integers */
typedef unsigned int64 DEPRECATED uint64;

#endif // HAVE_STDINT_H

#ifdef __cplusplus
/*	Class: charbuf

	General character array buffer handling class
 */
template<size_t size> class charbuf {
private:

	char buffer[size];

public:

	// Constructor: charbuf<size>
	inline charbuf<size>(void) { erase(); };

	// Constructor: charbuf<size>
	inline charbuf<size>(const char *s) { copy_from(s); };

	// Destructor: ~charbuf<size>
	inline ~charbuf<size>(void) {};

	// Method: get_size
	inline size_t get_size(void) { return size; };

	// Method: get_addr
	inline void *get_addr(void) { return (void*)buffer; };

	// Method: get_length
	inline size_t get_length(void) { return strlen(buffer); };

	// Method: get_string
	inline char *get_string(void) { return buffer; };

	// Method: erase
	inline char* erase(void) { return (char*)memset(buffer,0,size); };

	// Method: copy_to
	inline char* copy_to(char *s) { return s?strncpy(s,buffer,size):NULL; };

	// Method: copy_from
	inline char* copy_from(const char *s) { return s?strncpy(buffer,s,size):NULL; };

	// Method: operator char*
	inline operator char*(void) { return buffer; };

	// Method: operator==
	inline bool operator==(const char *s) { return strcmp(buffer,s)==0; };

	// Method: operator<
	inline bool operator<(const char *s) { return strcmp(buffer,s)==-1; };

	// Method: operator>
	inline bool operator>(const char *s) { return strcmp(buffer,s)==1; };

	// Method: operator<=
	inline bool operator<=(const char *s) { return strcmp(buffer,s)<=0; };

	// Method: operator>=
	inline bool operator>=(const char *s) { return strcmp(buffer,s)>=0; };

	// Method: find
	inline char *find(const char c) { return strchr(buffer,c); };

	// Method: find
	inline char *find(const char *s) { return strstr(buffer,s); };

	// Method: findrev
	inline char *findrev(const char c) { return strrchr(buffer,c); };

	// Method: token
	inline char *token(const char *from, const char *delim, char **context) { return ::strtok_r(from==NULL?this->buffer:NULL,delim,context); };

	// Method: format
	inline size_t format(char *fmt, ...) { va_list ptr; va_start(ptr,fmt); size_t len=vsnprintf(buffer,size,fmt,ptr); va_end(ptr); return len; };

	// Method: vformat
	inline size_t vformat(char *fmt, va_list ptr) { return vsnprintf(buffer,size,fmt,ptr); };
};

// Typedef: char1024
typedef charbuf<1025> char1024;

// Typedef: char256
typedef charbuf<257> char256;

// Typedef: char32
typedef charbuf<33> char32;

// Typedef: char8
typedef charbuf<9> char8;

#else // DEPRECATED

typedef char char1024[1025]; /**< strings up to 1024 characters */
typedef char char256[257]; /**< strings up to 256 characters */
typedef char char32[33]; /**< strings up to 32 characters */
typedef char char8[9]; /** string up to 8 characters */

#endif

// Typedef: set
typedef uint64 set;      /* sets (each of up to 64 values may be defined) */

// Typedef: enumeration
typedef uint32 enumeration; /* enumerations (any one of a list of values) */

// Typedef: object
typedef struct s_object_list* object; /* object references */

// Typedef: property
typedef class gld_property* property; /* object property references */

// Typedef: triplet
typedef double triplet[3];

// Typedef: triplex
typedef complex triplex[3];

// Typedef: STRING
typedef std::string* STRING;

#define BYREF 0x01
#include <math.h>

#ifdef __cplusplus

class double_array;

// Class: double_vector
class double_vector {
private:
	double **data;
public:
	double_vector(double **x) 
	{ 
		data=x; 
	};
	double &operator[] (const size_t n) 
	{ 
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n]; 
	};
	const double operator[] (const size_t n) const
	{
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n];
	}
};

// Class: double_array
class double_array { // TODO: make this a template class
public: // TODO make this private
#else
/*	Structure: s_doublearray
	n,m - dimensions (rows x cols)
	max - allocated size
	refs - count of references
	x - reference to 2D array of doubles
	f - flags (0x01: x by reference)
	name - pointer to the array name (NULL if none)
 */
typedef struct s_doublearray {
#endif
	size_t n, m; /** n rows, m cols */
	size_t max; /** current allocation size max x max */
	unsigned int *refs; /** reference count **/
	double ***x; /** pointer to 2D array of pointers to double values */
	unsigned char *f; /** pointer to array of flags: bit0=byref, */
	const char *name;
#ifndef __cplusplus
} double_array;
#else
	friend class double_vector;
private:

	inline void exception(const char *msg,...) const
	{ 
		static char buf[1024]; 
		va_list ptr;
		va_start(ptr,msg);
		sprintf(buf,"%s", name?name:""); 
		vsprintf(buf+strlen(buf), msg, ptr); 
		throw (const char*)buf;
		va_end(ptr);
	};
	inline void set_rows(const size_t i) { n=i; };
	inline void set_cols(const size_t i) { m=i; };
	inline void set_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]|=b;};
	inline void clr_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]&=~b;};
	inline bool tst_flag(const size_t r, size_t c, const unsigned char b) const {return (f[r*m+c]&b)==b;};
	double &my(const size_t r, const size_t c) 
	{ 
		if ( x[r][c]==NULL ) x[r][c] = new double;
		return (*x[r][c]); 
	};
public:
	inline double_vector operator[] (const size_t n) { return double_vector(x[n]); }
	inline const double_vector operator[] (const size_t n) const { return double_vector(x[n]); }
	double_array(const size_t rows=0, const size_t cols=0, double **data=NULL)
	{
		refs = new unsigned int;
		*refs = 0;
		m = n = max = 0;
		x = NULL;
		f = NULL;
		if ( rows>0 )
			grow_to(rows,cols);
		for ( size_t r=0 ; r<rows ; r++ )
		{
			for ( size_t c=0 ; c<cols ; c++ )
			{
				set_at(r,c, ( data!=NULL ? data[r][c] : 0.0 ) );
			}
		}
	};
	double_array(const double_array &a)
	{
		n = a.n;
		m = a.m;
		max = a.max;
		refs = a.refs;
		x = a.x;
		f = a.f;
		name = a.name;
		(*refs)++;
	};
	~double_array(void)
	{
		if ( (*refs)-- == 0 )
		{
			size_t r,c;
			for ( r=0 ; r<n ; r++ )
			{
				for ( c=0 ; c<m ; c++ )
				{
					if ( tst_flag(r,c,BYREF) )
					{
						free(x[r][c]); 
					}
				}
				free(x[r]);
			}
			free(x);
			if ( f ) free(f);
			delete refs;
		}
	};
public:
	void set_name(const char *v) { name = v; }; 
	inline const char *get_name(void) const { return name; };
	void copy_name(const char *v) { char *s=(char*)malloc(strlen(v)+1); strcpy(s,v); name=(const char*)s; };
	inline const size_t get_rows(void) const { return n; };
	inline const size_t get_cols(void) const { return m; };
	inline const size_t get_max(void) const { return max; };
	void set_max(const size_t size) 
	{
		if ( size<=max ) exception(".set_max(%u): cannot shrink double_array",size);
		size_t r;
		double ***z = (double***)malloc(sizeof(double**)*size);
		// create new rows
		for ( r=0 ; r<max ; r++ )
		{
			if ( x[r]!=NULL )
			{
				double **y = (double**)malloc(sizeof(double*)*size);
				if ( y==NULL ) exception(".set_max(%u): unable to expand double_array",size);
				memcpy(y,x[r],sizeof(double*)*max);
				memset(y+max,0,sizeof(double*)*(size-max));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max,0,sizeof(double**)*(size-max));
		free(x);
		x = z;
		unsigned char *nf = (unsigned char*)malloc(sizeof(unsigned char)*size);
		if ( f!=NULL )
		{
			memcpy(nf,f,max);
			memset(nf+max,0,size-max);
			free(f);
		}
		else
			memset(nf,0,size);
		f = nf;
		max=size; 
	};
	void grow_to(const size_t r, const size_t c) 
	{ 
		size_t s = (max<1?1:max);
		while ( c*r >= s ) s*=2; 
		if ( s>max )set_max(s);

		// add rows
		while ( n<r ) 
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (double**)malloc(sizeof(double*)*max);
				memset(x[n],0,sizeof(double*)*max);
			}
			n++;
		}

		// add columns
		if ( m<c )
		{
			size_t i;
			for ( i=0 ; i<n ; i++ )
			{
				double **y = (double**)malloc(sizeof(double*)*c);
				if ( x[i]!=NULL )
				{
					memcpy(y,x[i],sizeof(double**)*m);
					free(x[i]);
				}
				memset(y+m,0,sizeof(double**)*(c-m));
				x[i] = y;
			}
			m=c;
		}		
	};
	void grow_to(const size_t c) { grow_to(n>0?n:1,c); };
	void grow_to(const double_array &y) { grow_to(y.get_rows(),y.get_cols()); };
	void check_valid(const size_t r, const size_t c) const { if ( !is_valid(r,c) ) exception(".check_value(%u,%u): invalid (r,c)",r,c); };
	inline void check_valid(const size_t c) const { check_valid(0,c); };
	bool is_valid(const size_t r, const size_t c) const { return r<n && c<m; };
	inline bool is_valid(const size_t c) const { return is_valid(0,c); };
	bool is_nan(const size_t r, const size_t c)  const
	{
		check_valid(r,c);
		return ! ( x[r][c]!=NULL && isfinite(*(x[r][c])) ); 
	};
	inline bool is_nan(const size_t c) const { return is_nan(0,c); };
	bool is_empty(void) const { return n==0 && m==0; };
	void clr_at(const size_t r, const size_t c) 
	{ 
		check_valid(r,c);
		if ( tst_flag(r,c,BYREF) )
			free(x[r][c]); 
		x[r][c]=NULL; 
	};
	inline void clr_at(const size_t c) { return clr_at(0,c); };
	/// make a new matrix (row major)
	double **copy_matrix(void) 
	{   
		double **y = new double*[n];
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
		{
			y[r] = new double[m];
			unsigned int c;
			for ( c=0 ; c<m ; c++ )
				y[r][c] = *(x[r][c]);
		}
		return y;               
	};
	/// free a matrix
	void free_matrix(double **y)
	{
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
			delete [] y[r];
		delete [] y;
	};
	/// vector copy (row major)
	double *copy_vector(double *y=NULL)
	{
		if ( y==NULL ) y=new double[m*n];
		unsigned i=0;
		unsigned int r, c;
		for ( r=0 ; r<n ; r++ )
		{
			for ( c=0 ; c<m ; c++ )
				y[i++] = *(x[r][c]);
		}
		return y;
	}
	void transpose(void) {
		double ***xt = new double**[n];
		size_t i;
		for ( i=0 ; i<m ; i++ )
		{
			xt[i] = new double*[n];
			size_t j;
			for ( j=0 ; j<n ; j++ )
				xt[i][j] = x[j][i];
		}
		for ( i=0 ; i<n ; i++ )
			delete [] x[i];
		delete [] x;
		x = xt;
		size_t t=m; m=n; n=t;
	};
	inline double *get_addr(const size_t r, const size_t c) { return x[r][c]; };
	inline double *get_addr(const size_t c) { return get_addr(0,c); };
	double get_at(const size_t r, const size_t c) { return is_nan(r,c) ? QNAN : *(x[r][c]) ; };
	inline double get_at(const size_t c) { return get_at(0,c); };
	inline double &get(const size_t r, const size_t c) { return *x[r][c]; };
	inline double &get(const size_t c) { return get(0,c); };
	inline void set_at(const size_t c, const double v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, const double v) 
	{ 
		check_valid(r,c);
		if ( x[r][c]==NULL ) 
			x[r][c]=(double*)malloc(sizeof(double)); 
		*(x[r][c]) = v; 
	};
	inline void set_at(const size_t c, double *v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, double *v) 
	{ 
		check_valid(r,c);
		if ( v==NULL ) 
		{
			if ( x[r][c]!=NULL ) 
				clr_at(r,c);
		}
		else 
		{
			set_flag(r,c,BYREF);
			x[r][c] = v; 
		}
	};
	void set_ident(void)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = (r==c) ? 1 : 0;
		}
	};
	void dump(int r1=0, int r2=-1, int c1=0, int c2=-1)
	{
		if ( r2==-1 ) r2 = n-1;
		if ( c2==-1 ) c2 = m-1;
		if ( r2<r1 || c2<c1 ) exception(".dump(%u,%u,%u,%u): invalid (r,c)", r1,r2,c1,c2);
		size_t r,c;
		fprintf(stderr,"double_array %s = {\n",name?name:"unnamed"); 
		for ( r=r1 ; r<=n ; r++ )
		{
			for ( c=c1 ; c<=m ; c++ )
				fprintf(stderr," %8g", my(r,c));
			fprintf(stderr,"\n");
		}
		fprintf(stderr," }\n");
	}
	void operator= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = y;
		}
	};
	double_array &operator= (const double_array &y)
	{
		size_t r,c;
		grow_to(y);
		for ( r=0 ; r<y.get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
				my(r,c) = y[r][c];
		}
		return *this;
	};
	double_array &operator+= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y;
		}
		return *this;
	}
	double_array &operator+= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y[r][c];
		}
		return *this;
	};
	double_array &operator-= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y;
		}
		return *this;
	};
	double_array &operator-= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y[r][c];
		}
		return *this;
	};
	double_array &operator *= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) *= y;
		}
		return *this;
	};
	double_array &operator /= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) /= y;
		}
		return *this;
	};
	// binary operators
	double_array operator+ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) + y;
		return a;
	}
	double_array operator- (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) - y;
		return a;
	}
	double_array operator* (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) * y;
		return a;
	}
	double_array operator/ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) / y;
		return a;
	}
	double_array operator + (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("+%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?+?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) + y[r][c];
		return a;
	};
	double_array operator - (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("-%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?-?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) - y[r][c];
		return a;
	};
	double_array operator * (const double_array &y)
	{
		size_t r,c,k;
		if ( get_cols()!=y.get_rows() )
			exception("*%s: size mismatch",y.get_name());
		double_array a(get_rows(),y.get_cols());
		a.set_name("(?*?)");
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
			{	
				double b = 0;
				for ( k=0 ; k<get_cols() ; k++ )
					b += my(r,k) * y[k][c];
				a[r][c] = b;
			}
		}
		return a;
	};
	void extract_row(double*y,const size_t r)
	{
		size_t c;
		for ( c=0 ; c<m ; c++ )
			y[c] = my(r,c);
	}
	void extract_col(double*y,const size_t c)
	{
		size_t r;
		for ( r=0 ; r<n ; r++ )
			y[r] = my(r,c);
	}
};
#endif

#ifndef __cplusplus

/*	Structure: s_complexarray
	n,m - dimensions (rows x cols)
	max - current allocation size max x max
	x - pointer to 2D array of pointers to complex values
	f - pointer to array of flags: bit0=byref
 */
typedef struct s_complexarray {

#else

// Class: complex_array
class complex_array { // TODO: make this a template class
public:
#endif
	size_t n, m;
	size_t max; /** current allocation size max x max */
	complex ***x; /** pointer to 2D array of pointers to complex values */
	unsigned char *f; /** pointer to array of flags: bit0=byref, */
#ifndef __cplusplus
} complex_array;
#else
private:
	inline void set_rows(size_t i) { n=i; };
	inline void set_cols(size_t i) { m=i; };
	inline void set_flag(size_t r, size_t c, unsigned char b) {f[r*m+c]|=b;};
	inline void clr_flag(size_t r, size_t c, unsigned char b) {f[r*m+c]&=~b;};
	inline bool tst_flag(size_t r, size_t c, unsigned char b) {return (f[r*m+c]&b)==b;};
public:

	// Method: get_rows
	inline size_t get_rows(void) { return n; };

	// Method: get_cols
	inline size_t get_cols(void) { return m; };

	// Method: get_max
	inline size_t get_max(void) { return max; };

	// Method: set_max
	inline void set_max(size_t size) 
	{
		if ( size<=max ) throw "cannot shrink double_array";
		size_t r;
		complex ***z = (complex***)malloc(sizeof(complex**)*size);
		// create new rows
		for ( r=0 ; r<max ; r++ )
		{
			if ( x[r]!=NULL )
			{
				complex **y = (complex**)malloc(sizeof(complex*)*size);
				if ( y==NULL ) throw "unable to expand double_array";
				memcpy(y,x[r],sizeof(complex*)*max);
				memset(y+max,0,sizeof(complex*)*(size-max));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max,0,sizeof(complex**)*(size-max));
		free(x);
		x = z;
		unsigned char *nf = (unsigned char*)malloc(sizeof(unsigned char)*size);
		if ( f!=NULL )
		{
			memcpy(nf,f,max);
			memset(nf+max,0,size-max);
			free(f);
		}
		else
			memset(nf,0,size);
		f = nf;
		max=size; 
	};

	// Method: grow_to
	inline void grow_to(size_t r, size_t c=0) 
	{ 
		size_t s = max;
		while ( c>=s || r>=s ) s*=2; 
		if ( s>max )set_max(s);

		// add rows
		while ( n<=r ) 
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (complex**)malloc(sizeof(complex*)*max);
				memset(x[n],0,sizeof(complex*)*max);
			}
			n++;
		}
		if (m<=c) m=c+1; 
	};

	// Method: check_valid
	inline void check_valid(size_t r, size_t c=0) { if ( !is_valid(r,c) ) throw "double_array col/row spec is invalid"; };

	// Method: is_valid
	inline bool is_valid(size_t r, size_t c=0) { return r<n && c<m; };

	// Method: is_nan
	inline bool is_nan(size_t r, size_t c=0) 
	{
		check_valid(r,c);
		return ! ( x[r][c]!=NULL && isfinite(x[r][c]->Re()) && isfinite(x[r][c]->Im()) ); 
	};

	// Method: is_empty
	inline bool is_empty(void) { return n==0 && m==0; };

	// Method: clr_at
	inline void clr_at(size_t r, size_t c=0) 
	{ 
		check_valid(r,c);
		if ( tst_flag(r,c,BYREF) )
			free(x[r][c]); 
		x[r][c]=NULL; 
	};

	// Method: get_addr
	inline complex *get_addr(size_t r, size_t c=0) { return x[r][c]; };

	// Method: get_at
	inline complex &get_at(size_t r, size_t c=0) { return *(x[r][c]) ; };

	// Method: set_at
	inline void set_at(size_t r, size_t c, complex &v) 
	{ 
		check_valid(r,c);
		if ( x[r][c]==NULL ) 
			x[r][c]=(complex*)malloc(sizeof(complex)); 
		*(x[r][c]) = v; 
	};

	// Method: set_at
	inline void set_at(size_t r, size_t c, complex *v) 
	{ 
		check_valid(r,c);
		if ( v==NULL ) 
		{
			if ( x[r][c]!=NULL ) 
				clr_at(r,c);
		}
		else 
		{
			set_flag(r,c,BYREF);
			x[r][c] = v; 
		}
	};

	// Method: set_ident
	inline void set_ident(void)
	{
		complex one(1), zero(0);
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				set_at(r,c,(r==c)?one:zero);
		}
	};

	// Method: operator=
	inline void operator= (complex &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				set_at(r,c,y);
		}
	};

	// Method: operator=
	inline void operator= (complex_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				set_at(r,c,y.get_at(r,c));
		}
	};

	// Method: operator+=
	inline void operator+= (complex_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
			{
				complex z = get_at(r,c) + y.get_at(r,c);
				set_at(r,c,z);
			}
		}
	};

	// Method: operator-=
	inline void operator-= (complex_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
			{
				complex z = get_at(r,c) - y.get_at(r,c);
				set_at(r,c,z);
			}
		}
	};

	// Method: operator*=
	inline void operator*= (complex &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
			{
				complex z = get_at(r,c)*y;
				set_at(r,c,z);
			}
		}
	};

	// Method: operator/=
	inline void operator/= (complex &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
			{
				complex z = get_at(r,c)/y;
				set_at(r,c,z);
			}
		}
	};

	// Method: extract_row
	inline void extract_row(complex*y,size_t r)
	{
		size_t c;
		for ( c=0 ; c<m ; c++ )
			y[c] = get_at(r,c);
	}

	// Method: extract_col
	inline void extract_col(complex*y,size_t c)
	{
		size_t r;
		for ( r=0 ; r<n ; r++ )
			y[r] = get_at(r,c);
	}
};

#endif

#ifdef REAL4
typedef float real; 
#else
typedef double real;
#endif

#ifndef __cplusplus
#ifndef true
typedef unsigned char bool;
#define true (1)
#define false (0)
#endif
#endif

/*	Typedef: DELEGATEDTYPE

	See <s_delegatedtype>
 */
typedef struct s_delegatedtype DELEGATEDTYPE;

/*	Structure: s_delegatedtype
	type - the name of the delegated type
	oclass - the class implementing the delegated type
	from_string - the function that converts from a string to the data
	to_string - the function that converts from the data to a string

	Delegated types allow module to keep all type operations private.
	This includes convert operations and allocation/deallocation.

 */
struct s_delegatedtype {
	const char *type; 
	CLASS *oclass; 
	int (*from_string)(void *addr, const char *value); 
	int (*to_string)(void *addr, char *value, int size); 
}; 

/*	Typedef: DELEGATEDVALUE

	See <s_delegatedvalue>
 */
typedef struct s_delegatedvalue DELEGATEDVALUE;

/*
	Structure: s_delegatedvalue
	data - the data that is delegated
	type - reference to the delegation specification to use
 */
struct s_delegatedvalue
{
	char *data; 
	DELEGATEDTYPE *type; 
};

/*	Typedef: delegated

	See <DELEGATEDVALUE>
 */
typedef DELEGATEDVALUE* delegated;

/*	Enum: e_propertytype
	PT_void - the type has no data
	PT_double - the data is a double-precision float
	PT_complex - the data is a complex value
	PT_enumeration - the data is an enumeration
	PT_set - the data is a set
	PT_int16 - the data is a 16-bit integer
	PT_int32 - the data is a 32-bit integer
	PT_int64 - the data is a 64-bit integer
	PT_char8 - the data is \p NULL -terminated string up to 8 characters in length
	PT_char32 - the data is \p NULL -terminated string up to 32 characters in length 
	PT_char256 - the data is \p NULL -terminated string up to 256 characters in length
	PT_char1024 - the data is \p NULL -terminated string up to 1024 characters in length
	PT_object - the data is a pointer to a GridLAB object
	PT_delegated - the data is delegated to a module for implementation
	PT_bool - the data is a true/false value, implemented as a C++ bool
	PT_timestamp - timestamp value
	PT_double_array - the data is a fixed length double[]
	PT_complex_array - the data is a fixed length complex[]
	PT_real - Single or double precision float ~ allows double values to be overriden
	PT_float - Single-precision float
	PT_loadshape - Loadshapes are state machines driven by schedules
	PT_enduse - Enduse load data
	PT_random - Randomized number
	PT_method - Method

	PT_AGGREGATE - internal use only
	PT_KEYWORD - used to add an enum/set keyword definition
	PT_ACCESS - used to specify property access rights
	PT_SIZE - used to setup arrayed properties
	PT_FLAGS - used to indicate property flags next
	PT_INHERIT - used to indicate that properties from a parent class are to be published
	PT_UNITS - used to indicate that property has certain units (which following immediately as a string)
	PT_DESCRIPTION - used to provide helpful description of property
	PT_EXTEND - used to enlarge class size by the size of the current property being mapped
	PT_EXTENDBY - used to enlarge class size by the size provided in the next argument
	PT_DEPRECATED - used to flag a property that is deprecated
	PT_HAS_NOTIFY - used to indicate that a notify function exists for the specified property
	PT_HAS_NOTIFY_OVERRIDE - as PT_HAS_NOTIFY, but instructs the core not to set the property to the value being set
	PT_DEFAULT - identifies the default value to use when creating the object property
 */
enum e_propertytype {_PT_FIRST=-1, 
	PT_void,
	PT_double,
	PT_complex,
	PT_enumeration,
	PT_set,
	PT_int16,
	PT_int32,
	PT_int64,
	PT_char8,
	PT_char32,
	PT_char256,
	PT_char1024,
	PT_object,
	PT_delegated,
	PT_bool,
	PT_timestamp,
	PT_double_array,
	PT_complex_array,
	PT_real,
	PT_float,
	PT_loadshape,
	PT_enduse,
	PT_random,
	PT_method,
	PT_string,
	PT_property,
	PT_python,
	/* add new property types here - don't forget to add them also to rt/gridlabd.h and property.c */
#ifdef USE_TRIPLETS
	PT_triple,
	PT_triplex,
#endif
	_PT_LAST, 
	/* never put these before _PT_LAST they have special uses */
	PT_AGGREGATE,
	PT_KEYWORD,
	PT_ACCESS,
	PT_SIZE,
	PT_FLAGS,
	PT_INHERIT,
	PT_UNITS,
	PT_DESCRIPTION,
	PT_EXTEND,
	PT_EXTENDBY,
	PT_DEPRECATED,
	PT_HAS_NOTIFY,
	PT_HAS_NOTIFY_OVERRIDE,
	PT_DEFAULT,
	PT_REQUIRED,
	PT_OUTPUT,
	PT_DYNAMIC,
};

// Define: MAXGLMSIZE
#define MAXGLMSIZE 65536000

// Define: MAXPATHNAMELEN
#define MAXPATHNAMELEN 1024

// Define: MAXCODEBLOCKSIZE
#define MAXCODEBLOCKSIZE 65536

/*	Typedef: PROPERTYTYPE

	See: <e_propertytype>
 */
typedef enum e_propertytype PROPERTYTYPE; 

/*	Typedef: CLASSNAME

	The name a GridLAB class 
 */
typedef const char *CLASSNAME; 

//	Define: MAXCLASSNAMELEN
#define MAXCLASSNAMELEN 64

/*	Typedef: PROPERTYADDR

	The offset of a property from the end of the OBJECT header
 */
typedef void* PROPERTYADDR; 

/*	Typedef: PROPERTYNAME

	The name of a property 
*/
typedef const char *PROPERTYNAME; 

//	Define: MAXPROPNAMELEN
#define MAXPROPNAMELEN 64

/*	Typedef: FUNCTIONNAME

	The name of a function (not used)
 */
typedef const char *FUNCTIONNAME; 

//	Define: MAXPROPERTYVALUELEN
#define MAXPROPERTYVALUELEN 1024

//	Define: MAXOBJECTNAMELEN
#define MAXOBJECTNAMELEN 64

/* 	Structure: e_propertyaccess

	Property access rights (R/W apply to modules only, core always has all rights)
	PA_N = 0x00 - no access permitted
	PA_R = 0x01 - read access--modules can read the property
	PA_W = 0x02 - write access--modules can write the property
	PA_S = 0x04 - save access--property is saved to output
	PA_L = 0x08 - load access--property is loaded from input
	PA_H = 0x10 - hidden access--property is not revealed by modhelp
	PA_PUBLIC = (PA_R|PA_W|PA_S|PA_L) - property is public (readable, writable, saved, and loaded)
	PA_REFERENCE = (PA_R|PA_S|PA_L) - property is FYI (readable, saved, and loaded
	PA_PROTECTED = (PA_R) - property is semipublic (readable, but not saved or loaded)
	PA_PRIVATE = (PA_S|PA_L) - property is nonpublic (not accessible, but saved and loaded)
	PA_HIDDEN = (PA_PUBLIC|PA_H) - property is not visible 
 */
enum e_propertyaccess 
{
	PA_N = 0x00, 
	PA_R = 0x01, 
	PA_W = 0x02, 
	PA_S = 0x04, 
	PA_L = 0x08, 
	PA_H = 0x10, 
	PA_PUBLIC = (PA_R|PA_W|PA_S|PA_L), 
	PA_REFERENCE = (PA_R|PA_S|PA_L), 
	PA_PROTECTED = (PA_R), 
	PA_PRIVATE = (PA_S|PA_L), 
	PA_HIDDEN = (PA_PUBLIC|PA_H), 
}; 

/*	Typedef: PROPERTYACCESS

	See <e_propertyaccess>
 */
typedef enum e_propertyaccess PROPERTYACCESS; 

/*	TypedeF: KEYWORD

	See <s_keyword>
 */
typedef struct s_keyword KEYWORD;

/*	Structure: s_keyword
	name - the keyword name
	value - the keyword value
	next - the next keyword in the keyword list
 */
struct s_keyword 
{
	char name[32];
	uint64 value;
	struct s_keyword *next;
};

//	Define: MC_EXTRACT
//	Reads the first method record and returns the index of the next record (0 for last)
#define MC_EXTRACT (void*)0x0001

/*	Typedef: METHODCALL

	Method requests can have the following syntax

	Legacy syntax:

	- method_call(obj,NULL,0) --> returns the size of buffer needed to hold result
	- method_call(obj,NULL,size) --> returns 1 if size is larger than buffer size needed
	- method_call(obj,buffer,0) --> returns 1 if the buffer can be read into the obj
	- method_call(obj,buffer,size) --> returns 1 if the buffer can be written by the obj

	Extended syntax:

	- method_call(obj,token,...)
	
	Data extraction syntax:

	- method_call(obj,MC_EXTRACT,(char*)buffer,(size_t)size,(size_t)offset,(const char*)delimiter_chars) 
		- Returns value > offset if data extracted (value should be given as offset to next call)
		- Returns value <= offset if buffer is too small to hold resulting string
		- Returns value == 0 if no further data is available
		- Returns value == -1 if extract failed
	Note: the delimeter found is not included in the data copied to the buffer
	
	Example iterator
	--- C++ code ---
		char buffer[1024];
		int last_offset = 0, next_offset = 0;
		while ( (next_offset = method_call(obj,MC_EXTRACT,(char*)buffer,(size_t)sizeof(buffer),(int)last_offset,(const char*)",")) > last_offset )
		{
			// content of buffer is everything between last_offset (included) and delimiter (if found) or end-of-string (if delimiter not found)
			last_offset = next_offset;
		}
		// content of buffer is no longer valid
	--- end code ---
 **/
typedef int (*METHODCALL)(void *obj, ...); /**< the function that handles method requests */

// Typedef: PROPERTYFLAGS
typedef uint32 PROPERTYFLAGS;

/*  Define: PF_RECALC
	property has a recalc trigger (only works if recalc_<class> is exported)
 */
#define PF_RECALC	0x0001

/*	Define: PF_CHARSET
	Set supports single character keywords (avoids use of |) 
 */
#define PF_CHARSET	0x0002

/*	Define: PF_EXTENDED
	Indicates that the property was added at runtime
 */
#define PF_EXTENDED 0x0004 

/*	Define: PF_REQUIRED
	Indicates that the property must be set after creation
	for initialization to complete correctly
 */
#define PF_REQUIRED 0x0008

/*	Define: PF_OUTPUT
	Indicates that the property is updating internally and cannot
	be modified by external sources
 */
#define PF_OUTPUT	0x0010

/*	Define: PF_DYNAMIC
	Indicates that the property can changes in response to changes
	in other properties
 */
#define PF_DYNAMIC	0x0020

/*	Define: PF_DEPRECATED_NONOTICE
	The property is deprecated but no reference warning is desired
 */
#define PF_DEPRECATED_NONOTICE 0x4000 

/*	Define: PF_DEPRECATED
	The property is deprecated (warning will be displayed anytime it is used
 */
#define PF_DEPRECATED 0x8000 

/*	Structure: s_property_map

	oclass - class implementing the property
	name - property name
	ptype - property type
	size - property array 
	width - property size
	access - property access flags
	unit - property unit (only for underlying doubles)
	addr - property location (offset from header for <OBJECT> property, absolute for <GLOBALVAR>, NULL for methods)
	delegation - property delegation, if any (NULL for none)
	keywords - keywords used by the <enumeration> or set <property> types
	description - description of the property type
	next - next property in the property type list
	flags - property control flags
	notify - property update notification function (or NULL if none)
	method - method call to use when addr is NULL
	notify_override - (reserved)
	default_value - the default value to use when creating this property (overwritten later by object default and/or user input)
 */
struct s_property_map {
	CLASS *oclass; 
	PROPERTYNAME name;
	PROPERTYTYPE ptype;
	uint32 size; 
	int32 width; // negative values have special significance (see PSZ_*)
	PROPERTYACCESS access; 
	UNIT *unit; 
	PROPERTYADDR addr; 
	DELEGATEDTYPE *delegation; 
	KEYWORD *keywords; 
	const char *description; 
	struct s_property_map *next;
	PROPERTYFLAGS flags; 
	FUNCTIONADDR notify;
	METHODCALL method; 
	bool notify_override;
	const char *default_value; 
};

/*  Define: PSZ_DYNAMIC 

	Indicates that the size of property varies according to the value (e.g., long strings)
 */
#define PSZ_DYNAMIC (-1)

/*  Define: PSZ_AUTO 

	Indicates that the size of the property is determined at runtime by the underlying implementation (e.g, std::string().size())
*/
#define PSZ_AUTO (-2)

/*	Typedef: PROPERTY
	
	See <s_property_map>.
 */
typedef struct s_property_map PROPERTY; 

/*	Typedef: PROPERTYSTRUCT
		See <s_property_struct>
 */
typedef struct s_property_struct PROPERTYSTRUCT;

/*
	Structure: s_property_struct
	prop - reference to the property
	part - name of the property part
 */
struct s_property_struct
{
	PROPERTY *prop;
	PROPERTYNAME part;
};

/*	Enum: e_propertycompareop
	TCOP_EQ = 0 - property are equal to a
	TCOP_LE = 1 - property is less than or equal to a 
	TCOP_GE = 2 - property is greater than or equal a 
	TCOP_NE = 3 - property is not equal to a 
	TCOP_LT = 4 - property is less than a 
	TCOP_GT = 5 - property is greater than a 
	TCOP_IN = 6 - property is between a and b (inclusive) 
	TCOP_NI = 7 - property is not between a and b (inclusive) 
	TCOP_NOP = 8
	TCOP_ERR = -1

	Property comparison operators
 */
enum e_propertycompareop
{
	_TCOP_FIRST = 0,
	TCOP_EQ = _TCOP_FIRST,
	TCOP_LE = 1,
	TCOP_GE = 2,
	TCOP_NE = 3,
	TCOP_LT = 4,
	TCOP_GT = 5,
	TCOP_IN = 6,
	TCOP_NI = 7,
	_TCOP_LAST,
	TCOP_NOP = 8,
	TCOP_ERR=-1
};

/*	Typedef: PROPERTYCOMPAREOP
		See <e_propertycompareop>
 */
typedef enum e_propertycompareop PROPERTYCOMPAREOP;

//	Typedef: PROPERTYCOMPAREFUNCTION
typedef int PROPERTYCOMPAREFUNCTION(void*,void*,void*);

/*	Structure: s_property_specs
	name - property name
	xsdname - XML style type name
	default_value - default value
	size - size of one instance
	csize - size of instance when converted (not including string terminator)
	data_to_string - function to convert type to a string
	string_to_data - function to convert type from a string
	create - function to create an instance
	stream - function to stream data
	compare - comparison functions (see <TCOPD>)
	get_part - function to retrieve a data subitem
	set_part - function to set a data subitem

	The property type conversion specifications.
 */
struct s_property_specs 
{ 	const char *name; 
	const char *xsdname;
	const char *default_value;
	unsigned int size; 
	int csize; 
	int (*data_to_string)(char *,int,void*,PROPERTY*); 
	int (*string_to_data)(const char *,void*,PROPERTY*); 
	int (*to_initial)(char *,int,void*,PROPERTY*); 
	int (*create)(void*);
	size_t (*stream)(FILE*,int,void*,PROPERTY*);
	int (*string_to_compare)(const char*,void*,PROPERTY*);
	struct {
		PROPERTYCOMPAREOP op;
		char str[16];
		PROPERTYCOMPAREFUNCTION* fn;
		int trinary;
	} compare[_TCOP_LAST]; 
	double (*get_part)(void*,const char *name); 
	int (*set_part)(void*,const char *name,const char *value);
};

/*	Typedef: PROPERTYSPEC
		See <s_property_specs>.
 */
typedef struct s_property_specs PROPERTYSPEC;

/*	Structure: s_object_property
	object - object reference
	property - property reference
 */
struct s_object_property
{
	PROPERTY *prop;
	PROPERTYNAME part;
	struct s_object_list *obj;
};

/*	Typedef: OBJECTPROPERTY
		See <s_object_property>
 */
typedef struct s_object_property OBJECTPROPERTY;

/* 	Function: double_array_create

	Returns: 
	int - non-zero on success
*/
DEPRECATED int double_array_create(void *a);

/* 	Function: get_double_array_value
	
	Returns: 
	double - the value
*/
DEPRECATED double get_double_array_value(double_array*,unsigned int n, unsigned int m);

/* 	Function: set_double_array_value
 */
DEPRECATED void set_double_array_value(double_array*,unsigned int n, unsigned int m, double x);

/* 	Function: set_double_array_value
	
	Returns: 
	double* - a reference to the stored value
*/
DEPRECATED double *get_double_array_ref(double_array*,unsigned int n, unsigned int m);

/* 	Function: double_array_get_part
	
	Returns: 
	double - the value
*/
DEPRECATED double double_array_get_part(void *x, const char *name);

/* 	Function: complex_array_create
	
	Returns: 
	int - non-zero on success
*/
DEPRECATED int complex_array_create(void *a);

/* 	Function: get_complex_array_value
	
	Returns: 
	complex* - a reference the stored value
*/
DEPRECATED complex *get_complex_array_value(complex_array*,unsigned int n, unsigned int m);

/* 	Function: set_complex_array_value
*/
DEPRECATED void set_complex_array_value(complex_array*,unsigned int n, unsigned int m, complex *x);

/* 	Function: get_complex_array_ref

	Returns: 
	complex* - a reference the stored value
*/
DEPRECATED complex *get_complex_array_ref(complex_array*,unsigned int n, unsigned int m);

/* 	Function: complex_array_get_part
	
	Returns: 
	double - the value of the part
*/
DEPRECATED double complex_array_get_part(void *x, const char *name);

#ifdef __cplusplus
extern "C" {
#endif

/* 	Function: property_check

	Check whether the property as defined is mapped safely to memory
	
	Returns: 
	0 - check failed
	1 - check passed
*/
DEPRECATED int property_check(void);

/* 	Function: property_getspec
	
	Returns: 
	PROPERTYSPEC* - reference to the property's specifications
*/

DEPRECATED PROPERTYSPEC *property_getspec(PROPERTYTYPE ptype);

/* 	Function: property_getdefault

	Returns: 
	const char* - pointer to a string buffer containing the default value
*/
DEPRECATED const char *property_getdefault(PROPERTYTYPE ptype);

/* 	Function: property_getfirst_type
	
	Returns: 
	PROPERTYTYPE - a reference to the first property type
*/
DEPRECATED PROPERTYTYPE property_getfirst_type(void);

/* 	Function: property_getnext_type

	Returns: 
	PROPERTYTYPE - a reference to the next property type
*/
DEPRECATED PROPERTYTYPE property_getnext_type(PROPERTYTYPE ptype);

/* 	Function: property_malloc

	Returns: 
	PROPERTY* - a reference to the new property
*/
DEPRECATED PROPERTY *property_malloc(PROPERTYTYPE, CLASS *, const char *, void *, DELEGATEDTYPE *);

/* 	Function: property_size

	Returns: 
	uint32 - the size of the property in memory (bytes)
*/
DEPRECATED uint32 property_size(PROPERTY *);

/* 	Function: property_size_by_type	
	
	Returns: 
	uint32 - the size of the property in memory (bytes)
*/
DEPRECATED uint32 property_size_by_type(PROPERTYTYPE);

/* 	Function: property_minimum_buffersize
	
	Returns: 
	size_t - the minimum string buffer size needed to hold the property when converted
*/
DEPRECATED size_t property_minimum_buffersize(PROPERTY *);

/* 	Function: property_create

	Returns: 
	int - non-zero of success
*/
DEPRECATED int property_create(PROPERTY *, void *);

/* 	Function: property_compare_basic

	Returns: 
	bool - result of comparison
*/
DEPRECATED bool property_compare_basic(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void *x, void *a, void *b, const char *part);

/* 	Function: property_compare_basic_str

	Returns: 
	bool - result of comparison
*/
DEPRECATED bool property_compare_basic_str(PROPERTY *prop, PROPERTYCOMPAREOP op, void *x, const char *a, const char *b, const char *part);

/* 	Function: property_compare_op

	Returns: 
	PROPERTYCOMPAREOP - comparison operator function call
*/

DEPRECATED PROPERTYCOMPAREOP property_compare_op(PROPERTYTYPE ptype, const char *opstr);

/* 	Function: property_get_type

	Returns: 
	PROPERTYTYPE - the property type
*/
DEPRECATED PROPERTYTYPE property_get_type(const char *name);

/* 	Function: property_get_part

	Returns: 
	double - the property part value
*/
DEPRECATED double property_get_part(struct s_object_list *obj, PROPERTY *prop, const char *part);

/* 	Function: property_is_default
	
	Returns: 
	bool - tests whether the property is set to the default value 
*/
DEPRECATED bool property_is_default(struct s_object_list *obj, PROPERTY *prop);

/* 	Function: property_addr
	
	Returns: 
	void* - the address of the property
*/
DEPRECATED void *property_addr(struct s_object_list *obj, PROPERTY *prop);

/* 	Function: property_read
	
	Returns: 
	int - the number of bytes read from the buffer (negative on failure)
*/
DEPRECATED int property_read(PROPERTY *prop, void *addr, const char *string);

/* 	Function: property_write

	Returns: 
	int - the number of bytes written to the buffer (negative on failure)
*/
DEPRECATED int property_write(PROPERTY *prop, void *addr, char *string, size_t size);

/*	Function: string_create

	Returns:
	string* - success
	NULL - failure
 */
int string_create(void *ptr);

/*	Function: convert_to_string

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int convert_to_string(const char *s, void *data, PROPERTY *p);

/*	Function: convert_from_string

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int convert_from_string(char *buffer, int len, void *data, PROPERTY *p);

/*	Function: propertyref_create

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int propertyref_create(void *ptr);

/*	Function: convert_to_propertyref

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int convert_to_propertyref(const char *s, void *data, PROPERTY *p);

/*	Function: convert_from_propertyref

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int convert_from_propertyref(char *buffer, int len, void *data, PROPERTY *p);

/*	Function: initial_from_propertyref

	Returns:
	>0 - success
	=0 - no data
	<0 - failure
 */
int initial_from_propertyref(char *buffer, int len, void *data, PROPERTY *p);

#ifdef __cplusplus
}
#endif

#endif //_PROPERTY_H
