[[/GLM/Directive/Extern]] -- Link an externally defined function

# Syntax

GLM:
~~~
extern "C" MODULE_NAME : FUNCTION_NAME[@INDEX]
{
	SOURCE_CODE
}
~~~

# Description

The `extern` directive allow modelers to create sources for variable in GridLAB-D model.  The source code must be written in C or C++.  The function prototype is

~~~
int FUNCTION_NAME(int nlhs, GLXDATA *plhs, int nrhs, GLXDATA *prhs)
{
	// your code
	return 0; // 0 = success, -1 = failure
}
~~~

The function may be used in object definitions as a source in the same manner as a [[/GLM/Filter]]. 

# Example

The following is a function definition that applies a linear transformation to the input value.

my_example.glm:
~~~
#define A=1.23
#define B=4.56
extern "C" project : linear@1
{
	int linear(int nlhs, GLXDATA *plhs, int nrhs, GLXDATA *prhs)
	{
		if ( nlhs != 1 || nrhs != 1 ) return -1;
		plhs[0] = ${A}*GLXdouble(prhs[0]) + ${B};
		return 0;
	}
}


class test
{
	double input;
	double output;
}

object test
{
	name my_test;
	input 1.0;
	output linear(input);
}
~~~

To run this model and obtain the result, do the following:

~~~
shell% gridlabd my_example.glm -o my_example.json
shell% gridlabd json-get objects my_test output <my_example.json
5.79
~~~
