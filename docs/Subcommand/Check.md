[[/Subcommand/Check]] -- Subcommand to perform a detailed check of the validity of a GridLAB-D model

# Synopsis

~~~
bash$ gridlabd check [OPTIONS] FILE …
~~~

# Description

The `check` subcommand runs detailed checks on GLM and JSON models.  If the file is GLM, it is loaded and converted 
to JSON automatically. Any load errors or warning are displayed.

The JSON file check is a comprehensive model check system that examines each object and it's relationship to other
objects in the model.  See the module's check routine for details.

# Options

## `debug`
~~~
	-d|--debug
~~~
Enable python traceback output when problems are encountered
## `fix`
~~~
	-f|--fix=FILE
~~~
Fixes problems (if possible) are writes to the specified file
## `quiet`
~~~
	-q|--quiet
~~~ 
Suppress all non-error output
## `verbose`
~~~
	-v|--verbose
~~~
Enable additional output

## `strict`

~~~
	-s|--strict
~~~

Causes warnings to be treated as errors with exit code 1.
