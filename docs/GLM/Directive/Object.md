[[/GLM/Directive/Object]] -- GLM directive to define one or more objects

# Synopsis

GLM:

~~~
object [MODULE.]CLASS[:..COUNT|:FIRST..LAST]
{
	PROPERTY_1 VALUE_1;
	PROPERTY_2 VALUE_2;
	...
	PROPERTY_N VALUE_N;
	[NESTED_OBJECTS ...]
}
~~~

# Description

The `object` directive is used to instantiate one or more objects in a GridLAB-D model. 

To instantiate a single object:

~~~
object [MODULE.]CLASS
{
	...
}
~~~

To instantiate multiple objects:

~~~
object [MODULE.]CLASS[:..COUNT]
{
	...
}
~~~

To instantiate multiple objects with a specified range of object ids:

~~~
object [MODULE.]CLASS[:FIRST..LAST]
{
	...
}
~~~

To instantiate an object with nested objects:

~~~
object [MODULE.]CLASS
{
	...
	object [MODULE.]CLASS
	{
		...
	}
}
~~~

# Caveat

When nesting objects, it is often necessary to refer to properties of the parent object.  The backtic syntax is used to embed properties in strings, e.g.,

~~~
class test 
{
	char32 output;
}

object test
{
	object test 
	{
		name "my_test";
		output `id_{id}`; 
	};
}
~~~

To run this example, use the following commands:

~~~
shell% gridlabd /tmp/test.glm -o /tmp/test.json 
shell% gridlabd json-get objects my_test output </tmp/test.json
id_1
~~~

# See also

* [[/GLM/Directive/Class]]
