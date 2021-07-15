[[/GLM/Directive/Dump]] -- Dump request directive

# Synopsis

GLM:

~~~
dump interval filename;
~~~

# Description

The `dump` directive causes the dump file `filename` to be generated at the `interval` times. 

If the filename starts with a dash, then the output is sent to stdout in `GLM` format.  Adding the extension after the dash causes the specified format to use generated (i.e., `glm` or `json`).

# See also

* [[/Global/Filesave_options]]
