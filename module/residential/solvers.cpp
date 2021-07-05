/** $Id: solvers.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#define ARMA_DONT_PRINT_ERRORS
#include <armadillo>

#define USE_GLSOLVERS
#include "residential.h"
#include "solvers.h"

using namespace arma;

static void exception(const char *format, ...)
{
	char *message = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&message,format,ptr);
	va_end(ptr);
	throw message ? message : "memory allocation failure";
}

static void debug(const char *format, ...)
{
	char *message = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&message,format,ptr);
	va_end(ptr);
	gl_debug("(solvers) %s", message ? message : "memory allocation failure");
	if ( message ) free(message);
}

static void verbose(const char *format, ...)
{
	char *message = NULL;
	va_list ptr;
	va_start(ptr,format);
	vasprintf(&message,format,ptr);
	va_end(ptr);
	gl_verbose("(solvers) %s", message ? message : "memory allocation failure");
	if ( message ) free(message);
}

static void dump(const char *t, double *p, size_t N)
{
	cerr << t << "[" << N << "] = [ "; 
	for ( size_t n = 0 ; n < N; n++ ) 
	{
		cerr << (n>0?", ":"") << p[n]; 
	}
	cerr << "]" << endl;
}

static void dump(const char *t, mat x)
{
	cerr << t 
	     << "[" 
	     << x.n_rows 
	     << "x" 
	     << x.n_cols 
	     << "] = " 
	     << endl
	     << x 
	     << endl;
}

static void dump(const char *t, vec x)
{
	cerr << t 
	     << "[" 
	     << x.n_rows 
	     << "x" 
	     << x.n_cols 
	     << "] = " 
	     << endl
	     << x 
	     << endl;
}

//////////////////////////////////////////////////////////////////////////////
//
// ETP Solver -- single-zone thermal model solver
//
//////////////////////////////////////////////////////////////////////////////


double e2solve(double a, 
               double n, 
               double b, 
               double m, 
               double c, 
               double p, 
               double *e)
{
	static gld_solver *etp = NULL;
	static struct etpdata 
	{
		double t,a,n,b,m,c,p,e;
		unsigned int i;
	} data;

	// load the solver if not yet loaded
	if ( etp==NULL )
	{
		etp = new gld_solver("etp");
		int version;
		if ( etp->get("version",&version,NULL) == 0 || version != 1 )
		{
			exception("incorrect ETP solver version (version=%d)",version);
		}
		if ( etp->get("init",&data,NULL) == 0 )
		{
			exception("unable to initialize ETP solver data");
		}
		data.i = 100;
	}

	// solve the given problem
	data.t = 0;
	data.a = a;
	data.b = b;
	data.c = c;
	data.n = n;
	data.m = m;
	data.p = p;
	if ( etp->solve(&data) )
	{
		if ( e != NULL )
		{
			*e = data.e;
		}
		return data.t;
	}
	else
	{
		return NaN;
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// MSOLVER -- multizone thermal model solver
//
// TODO: move this to the core so it can be shared with commercial module
//
//////////////////////////////////////////////////////////////////////////////

// status update flags
#define SU_NONE 0x0000
#define SU_A    0x0001
#define SU_B1   0x0002
#define SU_B2   0x0004
#define SU_Q    0x0008
#define SU_T    0x0010
#define SU_U    0x0020

class parameter_map 
{
private:
	std::string name;
	double **array;
	size_t size;
	int modifies;
public:
	parameter_map(const char *n, double **a, size_t s, int m)
	: name(n), array(a), size(s), modifies(m) {}
	~parameter_map(void) { delete [] *array; }
public:
	inline std::string &get_name(void) { return name; };
	inline double *&get_array(void) { return *array; };
	inline double get_value(size_t n) { assert(n>0&&n<size); return (*array)[n]; };
	inline void set_value(size_t n, double x) { assert(n>0&&n<size); (*array)[n] = x; };
	inline double get_value_x(size_t n) { return (*array)[n]; };
	inline void set_value_x(size_t n, double x) { (*array)[n] = x; };
	inline void copy_values(double *a) { memcpy(*array,a,size*sizeof(double)); };
	inline size_t get_size(void) { return size; };
	inline int get_modifies(int n=0xffff) { return modifies&n; };
};

class MSolver
{
public:
	MSolver(void);
	~MSolver(void);
private:
	int N;
	mat A, B1, B2;
	mat T, q, u, Tset, dT;
	mat Ainv, B1inv, B2inv, Aeig;
	mat Tbal, Teq;
	mat mode;
	int modified;
	msolver *data;
	bool enable_dump;
	bool enable_debug;
	bool enable_verbose;
public:
	void init(int size);
	std::list<parameter_map> &get_map(void);
	parameter_map &find_map(const char *name);
	void add_map(const char *name, double **array, size_t size, int modifies);
	inline size_t get_mapsize() { return map.size(); };
	inline int get_size(void) { return N; };
	inline void set_modified(int su) { modified |= su; debug("modified |= 0x%04X -> 0x%04x",su,modified); };
	void update(bool force = false);
	void solve(double dt);
	inline msolver *get_data(void) { return data; };
	void dump(const char *heading=NULL);
	inline void set_enable_dump(bool state) { enable_dump = state; };
	inline void set_enable_debug(bool state) { enable_debug = state; };
	inline void set_enable_verbose(bool state) { enable_verbose = state; };
	inline bool is_enable_dump(void) { return enable_dump; };
	inline bool is_enable_debug(void) { return enable_debug; };
	inline bool is_enable_verbose(void) { return enable_verbose; };
private:
	std::list<parameter_map> map;
};

MSolver::MSolver()
{
	N = 0;
	data = new msolver;
	memset(data,0,sizeof(msolver));
	data->solver = this;
	modified = SU_NONE;
	enable_dump = false;
	enable_debug = false;
	enable_verbose = false;
}

MSolver::~MSolver(void)
{
	if ( data )
	{
		map.clear();
		delete data;
	}
}

void MSolver::add_map(const char *name, double **array, size_t size, int modifies)
{
	parameter_map *m = new parameter_map(name,array,size,modifies);
	*array = new double[size];
	map.insert(map.end(),*m);
}
std::list<parameter_map> &MSolver::get_map(void)
{
	return map;
}

parameter_map &MSolver::find_map(const char *name)
{
	for ( std::list<parameter_map>::iterator m = map.begin() ; m != map.end() ; m++ )
	{
		if ( m->get_name() == name )
		{
			return *m;
		}
	}
	exception("MSolver::get_map(name='%s'): parameter name not found",name);
	throw "an internal logic error occurred";
}

void MSolver::init(int size)
{
	if ( N != 0 )
		exception("cannot reinitialize solver (old size %d, new size %d)", N, size);
	N = size;
	A = mat(size,size).zeros();
	B1 = mat(size,size).zeros();
	B2 = mat(size,size).zeros();
	T = mat(size,1).zeros();
	q = mat(size,1).zeros();
	u = mat(size,1).zeros();
	Tset = mat(size,1).zeros();
	dT = mat(size,1).zeros();
	add_map("U",    &(data->U),    N*(N+1)/2, SU_A|SU_B1);
	add_map("C",    &(data->C),    N,         SU_A|SU_B1|SU_B2);
	add_map("T",    &(data->T),    N+1,       SU_NONE);
	add_map("q",    &(data->q),    N,         SU_Q);
	add_map("a",    &(data->a),    1,         SU_B1|SU_B2);
	add_map("umin", &(data->umin), N-1,       SU_U);
	add_map("umax", &(data->umax), N-1,       SU_U);
	add_map("Tset", &(data->Tset), N-1,       SU_T);
	add_map("dT",   &(data->dT),   N,         SU_NONE);
	add_map("mode", &(data->mode), N,         SU_NONE);
}

void MSolver::update(bool force)
{
	debug("MSolver::update(force=%s): modified = 0x%04X", force?"true":"false", modified);
	const char *error = NULL;
	try
	{
		// build A if needed
		if ( (modified&SU_A) | force)
		{
			if ( enable_debug ) debug("A outdated");
			int n = N;
			for ( int i = 0 ; i < N ; i++ )
			{
				A(i,i) = -data->U[i];
				for ( int j = i+1 ; j < N ; j++ )
				{
					A(i,i) -= data->U[n];
					A(i,j) = data->U[n] / data->C[i];
					A(j,i) = data->U[n] / data->C[j];
					n++;
				}
				for ( int j = 0 ; j < i ; j++ )
				{
					A(i,i) -= A(i,j) * data->C[i];
				}
				A(i,i) /= data->C[i];
			}
			if ( enable_dump ) ::dump("updating A",A);			
			vec eig = arma::real(eig_gen(A));
			if ( eig.max() >= 0 )
			{
				::dump("eig",eig);
				exception("solution matrix A is not physically feasible (non-negative eigenvalue found)");
			}
			Aeig = diagmat(eig);
			if ( enable_dump ) ::dump("updating Aeig",Aeig);
			Ainv = inv(A);
			if ( enable_dump ) ::dump("updating Ainv",Ainv);
		}

		// build B1 if needed
		if ( (modified&SU_B1) | force )
		{
			if ( enable_debug ) debug("B1 outdated");
			for ( int i = 0 ; i < N ; i++ )
			{
				B1(i,0) = data->U[i] / data->C[i];
				if ( i < N-1 )
				{
					B1(i,i+1) = data->a[0] / data->C[i];
					B1(N-1,i+1) = (1-data->a[0]) / data->C[N-1];
				}
			}
			if ( enable_dump ) ::dump("updating B1",B1);
			B1inv = inv(B1);
			if ( enable_dump ) ::dump("updating B1inv",B1inv);
		}

		// build B2 if needed
		if ( (modified&SU_B2) | force )
		{
			if ( enable_debug ) debug("B2 outdated");
			for ( int i = 0 ; i < N ; i++ )
			{
				if ( i < N-1 )
				{
					B2(i,i) = data->a[0] / data->C[i];
					B2(N-1,i) = (1-data->a[0]) / data->C[N-1];
				}
				else
				{
					B2(N-1,N-1) = 1 / data->C[N-1];
				}
			}
			if ( enable_dump ) ::dump("updating B2",B2);
			B2inv = inv(B2);
			if ( enable_dump ) ::dump("updating B2inv",B2inv);
		}

		// build q if needed
		if ( (modified&SU_Q) | force )
		{
			if ( enable_debug ) debug("q outdated");
			for ( int i = 0 ; i < N ; i++ )
			{
				q[i] = data->q[i];
			}	
			if ( enable_dump ) ::dump("updating q",q);
		}

		if ( (modified&(SU_A|SU_B1|SU_Q)) | force )
		{
			if ( enable_debug ) debug("Tbal outdated");
			Tbal = -Ainv*B1*q;
			if ( enable_dump ) ::dump("updating Tbal",Tbal);
			mode = Tbal-q[0];
			if ( enable_dump ) ::dump("updating mode",mode);
		}

		// update the control input if needed
		if ( (modified&(SU_B2|SU_A|SU_T|SU_B1|SU_Q)) | force )
		{
			if ( enable_debug ) debug("u outdated");
			u = -B2inv*(A*Tset+B1*q);
			if ( enable_dump ) ::dump("updating u",u);
			modified |= SU_U;
		}

		// constrain the control input to capacity limits (should use clamp but that doesn't compile)
		if ( (modified&SU_U) | force )
		{
			if ( enable_debug ) debug("u outdated");
			for ( int i = 0 ; i < N ; i++ )
			{
				if ( data->umin[i] > data->umax[i] ) 
				{ 
					continue; 
				}
				if ( u[i] < data->umin[i] ) 
				{ 
					u[i] = data->umin[i]; 
				}
				else if ( u[i] > data->umax[i] ) 
				{ 
					u[i] = data->umax[i]; 
				}
			}
			if ( enable_dump ) ::dump("updating u",u);
		}

		if ( (modified&(SU_A|SU_B1|SU_Q|SU_B2|SU_U)) | force )
		{
			if ( enable_debug ) debug("Teq outdated");
			Teq = Tbal - Ainv*B2*u;
			if ( enable_dump ) ::dump("updating Teq",Teq);
		}
		modified = SU_NONE;
	}
	catch (std::exception &msg)
	{
		dump(msg.what());
		error = msg.what();
	}
	if ( error )
	{
		exception("MSolver::update() failed on error %s",error);
	}
}

void MSolver::solve(double dt)
{
	const char *error = NULL;
	try
	{
		if ( enable_debug ) debug("updating T(dt=%g)",dt);
		if ( enable_dump ) ::dump("dt",&dt,1);
		dT = T - (Teq + (exp(Aeig*dt)-mat(4,4).eye())*T);
		if ( enable_dump ) ::dump("dT",dT);
		T += dT;
		if ( enable_dump ) ::dump("T",T);
	}
	catch (std::exception &msg)
	{
		dump(msg.what());
		error = msg.what();
	}
	if ( error )
	{
		exception("MSolver::solve() failed on error %s",error);
	}
}

void MSolver::dump(const char *heading)
{
	if ( enable_debug ) debug("dumping MSolver data");
	cerr << "MSolver dump " << (heading?heading:"with no context") << "" << endl;
	cerr << "  N = " << N << endl;
	cerr << "MSolver input data:" << endl;
	for ( std::list<parameter_map>::iterator m = map.begin() ; m != map.end() ; m++ )
	{
		::dump(m->get_name().c_str(),m->get_array(),m->get_size());
	}
	cerr << "MSolver problem data:" << endl;
	::dump("U",data->U,N*(N+1)/2);
	::dump("C",data->C,N);
	::dump("q",data->q,N);
	::dump("T",data->T,N);
	::dump("Tset",data->Tset,N-1);
	// ::dump("u",data->u,N-1);
	::dump("a",data->a,1);
	::dump("umin",data->umin,N-1);
	::dump("umax",data->umax,N-1);
	cerr << "MSolver internal data:" << endl;
	//::dump("Tbal",data->Tbal,N);
	//::dump("Teq",data->Teq,N);
	::dump("dT",data->dT,N);
	::dump("mode",data->mode,N-1);
	cerr << "MSolver solution data:" << endl;
	::dump("A",A);
	::dump("B1",B1);
	::dump("B2",B2);
	::dump("T",T);
	::dump("q",q);
	::dump("u",u);
	::dump("Tset",Tset);
}

msolver *msolve(const char *op, ...)
{
	msolver *data = NULL;
	MSolver *solver = NULL;
	va_list ptr;
	va_start(ptr,op);

	if ( strcmp(op,"new") == 0 )
	{
		solver = new MSolver();
		data = solver->get_data();
	}
	else if ( strcmp(op,"get") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		const char *param = va_arg(ptr,const char *);
		double **ref = va_arg(ptr,double**);
		*ref = solver->find_map(param).get_array();
	}
	else if ( strcmp(op,"set") == 0 || strcmp(op,"copy") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		const char *param = va_arg(ptr,const char*);

		// process changes to N
		if ( strcmp(op,"set") == 0 ) 
		{
			if ( strcmp(param,"N") == 0 )
			{
				data->N = va_arg(ptr,int);
				if ( data->N < 2 )
				{
					exception("msolver N must be greater than 1 (N=%d)",solver->get_size());
				}
				solver->init(data->N);
				verbose("msolver(op='set',param='%s',value=%d): ok",param,data->N);
			}
			else if ( strcmp(param,"dump") == 0 )
			{
				int value = va_arg(ptr,int);
				solver->set_enable_dump(value?true:false);
				verbose("msolver(op='set',param='%s',value=%d): enable_dump <- %s",param,value,solver->is_enable_dump()?"true":"false");
			}
			else if ( strcmp(param,"debug") == 0 )
			{
				int value = va_arg(ptr,int);
				solver->set_enable_debug(value?true:false);
				verbose("msolver(op='set',param='%s',value=%d): enable_debug <- %s",param,value,solver->is_enable_debug()?"true":"false");
			}
			else if ( strcmp(param,"verbose") == 0 )
			{
				int value = va_arg(ptr,int);
				solver->set_enable_verbose(value?true:false);
				verbose("msolver(op='set',param='%s',value=%d): enable_verbose <- %s",param,value,solver->is_enable_verbose()?"true":"false");
			}
			else
			{
				parameter_map &m = solver->find_map(param);
				int n = 0;
				if ( m.get_size() > 1 )
				{
					n = va_arg(ptr,int);
				}
				double value = va_arg(ptr,double);
				m.set_value(n,value);
				solver->set_modified(m.get_modifies());
				verbose("msolver(op='set',param='%s',index=%d,value=%g): ok",param,n,value);
			}
		}
		else // copy
		{
			parameter_map &m = solver->find_map(param);
			for ( size_t n = 0 ; n < m.get_size() ; n++ )
			{
				double value = va_arg(ptr,double);
				m.set_value(n,value);
				if ( solver->is_enable_verbose() ) verbose("msolver(op='copy',param='%s',index=%d,value=%g): ok",param,n,value);
			}
			solver->set_modified(m.get_modifies());
			verbose("msolver(op='copy',param='%s',range=%d..%d): ok",param,0,m.get_size()-1);
		}
	}
	else if ( strcmp(op,"update") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		solver->update();
	}
	else if ( strcmp(op,"solve") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		solver->update();
		double dt = va_arg(ptr,double);
		solver->solve(dt);
	}
	else if ( strcmp(op,"delete") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		delete solver;
		data = NULL;
	}
	else if ( strcmp(op,"dump") == 0 )
	{
		data = va_arg(ptr,msolver*);
		solver = (MSolver*)(data->solver);
		assert(solver->get_data()==data);
		solver->dump();
	}
	else 
	{
		exception("msolve(op='%s'): operation is invalid ",op);
	}

	va_end(ptr);
	return data;
}

