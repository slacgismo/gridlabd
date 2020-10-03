import json 
import os 
import sys, getopt
import json

config = {"input":"json","output":"md","options":{}}

def help():
    print('Syntax:')
    print(f'{config["input"]}2{config["output"]}.py -i|--ifile <input-file>[,<input-file>[,...]] -o|--ofile <output-file> [options ...]')
    print(f'  -c|--config    : [OPTIONAL] display converter configuration')
    print(f'  -i|--ifile     : [REQUIRED] {config["input"]} input file name')
    print(f'  -o|--ofile     : [REQUIRED] {config["output"]} output file name')

def error(msg):
    print(f'ERROR    [{config["input"]}2{config["output"]}]: {msg}')
    sys.exit(1)

input_file = None
output_file = None
options = {}

opts, args = getopt.getopt(sys.argv[1:],"hci:o:",["help","config","ifile=","ofile="])

if not opts : 
    help()
    sys.exit(1)

for opt, arg in opts:
    if opt in ("-h","--help"):
        help()
        sys.exit(0)
    elif opt in ("-c","--config"):
        print(json.dumps(config))
        sys.exit(0)
    elif opt in ("-i", "--ifile"):
        input_file = arg.strip()
    elif opt in ("-o", "--ofile"):
        output_file = arg.strip()
    else:
        error(f"{opt}={arg} is not a valid option");

if input_file == None:
    error("missing input file name")
elif output_file == None:
    error("missing output file name")

with open(input_file,"r") as fh:
	model = json.load(fh)
if not output_file :
	output_file = input_file.replace(".json",".md")

application = model["application"]
version = model["version"]
modules = model["modules"]
types = model["types"]
header = model["header"]
classes = model["classes"]
globals = model["globals"]
schedules = model["schedules"]
objects = model["objects"]

with open(output_file,"w") as md:

	def row(*args):
		if args:
			md.write("| " + " | ".join(args) + " |")
		md.write("\n")

	def hdr(n,str):
		md.write(f"{'#'*n} {str}\n\n")
	
	def get(x,n,y=None):
		if n in x: return x[n]
		else: return y

	hdr(1,f"Model documentation of {input_file}")
	row("Application","Version")
	row("-----------","-------")
	row(application,version)
	row()

	# list modules
	if modules:
		hdr(2,"Modules")
		row("Name","Version")
		row("----","-------")
		for name, values in modules.items():
			major = values["major"]
			minor = values["minor"]
			row(name,f"{major}.{minor}")
		row()

	# classes
	active_classes = []
	for object, properties in objects.items():
		oclass = properties["class"]
		if oclass not in active_classes:
			active_classes.append(oclass)
	if active_classes:
		hdr(2,"Classes")
	for oclass, properties in classes.items():
		if oclass not in active_classes:
			continue
		hdr(3,oclass)
		row("Property","Type","Unit","Access","Flags","Keywords","Default","Description")
		row("--------","----","----","------","-----","--------","-------","-----------")
		for property, specs in properties.items():
			if not type(specs) is dict:
				continue
			ptype = get(specs,"type","(na)")
			access = get(specs,"access","PUBLIC")
			flags = get(specs,"flags","")
			keywords = get(specs,"keywords","")
			unit = get(specs,"unit","")
			if type(keywords) is dict:
				keywords = " ".join(keywords.keys())
			default = get(specs,"default","")
			description = get(specs,"description","")
			row(property,ptype,unit,access,flags,keywords,default,description)
		row()

	# objects
	for name, properties in objects.items():
		hdr(3,name)
		row("Property","Value")
		row("--------","-----")
		for property, value in properties.items():
			row(property,value)
		row()			

	hdr(2,"Global data")

	# header data
	hdr(3,"Header data")
	row("Name","Type","Access")
	row("----","----","------")
	for name, values in header.items():
		row(name,values["type"],values["access"])
	row()

	# list system data types
	hdr(3,"Data Types")
	row("Type","Format","Default","Size","Converters","Comparators")
	row("----","-------","-------","----","----------","-----------")
	for ptype, values in types.items():
		format = values["xsdtype"]
		if "default" in values:
			default = values["default"]
		else:
			default = "(na)"
		if "strsize" in values:
			size = values["strsize"]
		else:
			size = "(na)"
		converters = []
		for key, item in values.items():
			if key[0:4] == "has_" and item: converters.append(key[4:])
		converters = " ".join(converters)
		comparators = " ".join(values["compareops"].keys())
		row(ptype,format,default,size,converters,comparators)
	row()

	# globals
	hdr(3,"Global variables")
	row("Name","Module","Type","Access","Keywords","Value")
	row("----","------","----","------","--------","-----")
	for name, specs in globals.items():
		if "::" in name:
			name = name.split("::")
			module = name[0]
			name = name[1]
		ptype = get(specs,"type","(na)")
		access = get(specs,"access","(na)")
		value = get(specs,"value","(na)")
		keywords = get(specs,"keywords","")
		if type(keywords) is dict:
			keywords = " ".join(keywords.keys())
		else:
			module = ""
		row(name,module,ptype,access,keywords,value)
	row()
