[[/GLM/Directive/Modify]] -- Modify object properties

# Synopsis

GLM:

~~~
modify NAME.PROPERTY VALUE;
~~~

# Description

The `modify` directive changes the values of properties in objects that have already been loaded.

# Example

The following example modifies the value the property

~~~
class test 
{
	double output;
}

object test
{
	name "my_test";
	output "1.0";
}

modify my_test.output 2.0;
~~~

To run this example, do the following:

~~~
shell% gridlabd /tmp/test.glm -o /tmp/test.json
shell% gridlabd json-get objects my_test output </tmp/test.json
2
~~~

# See also

* [[/GLM/Directive/Class]]
* [[/GLM/Directive/Object]]
