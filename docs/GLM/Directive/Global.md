[[/GLM/Directive/Global]] -- Define a global variable

# Synopsis

GLM:

~~~
global TYPE NAME[[UNIT]] VALUE [UNIT]; 
~~~

# Description

# Example

The following example defines a complex global variable named `my_value` with units `kV` and defines it as having magnitude `12` and angle `1 deg`.

~~~
global complex my_value[kV] 12+1d kV;
~~~


# Caveat

Not all data types are supported.  For instance, `set` and `enumerate` types cannot be defined using this syntax yet.

# See also

* [[/GLM/Macro/Define]]
* [[/GLM/Macro/Set]]
