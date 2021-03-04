[[/Global/Complex_output_format]] -- Complex output representation

# Synopsis

GLM:

~~~
#set complex_output_format=[DEFAULT|RECT|POLAR_DEG|POLAR_RAD]
~~~

Shell:

~~~
shell% gridlabd -D|--define complex_output_format=[DEFAULT|RECT|POLAR_DEG|POLAR_RAD]
~~~

# Description

The `complex_output_format` global variable controls the formatting of complex numbers generated by gridlabd's core.  The formatting can be rectangular or polar. Rectangular formatting uses the `j` notation, and polar can be either in degrees or radians. 

## `DEFAULT`

All complex numbers will be formatted using the default mechanisms in GridLAB-D's core, which uses the format of the last function which operated on the complex value.

## `RECT`

All complex numbers will be formatted using the `<real>+<imag>j` format.

## `POLAR_DEG`

All complex numbers will be formatted using the `<mag>+<angle>d` format.

## `POLAR_RAD`

All complex numbers will be formatted using the `<mag>+<angle>r` format.

# See also

* [[/Global/Complex_format]]
