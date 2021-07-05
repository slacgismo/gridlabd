import json 
import os 
import sys, getopt
from datetime import datetime 
import importlib, copy
from importlib import util

def help():
    print('Syntax: mdb2glm.py -i|--ifile <input-file> -o|--ofile <output-file> -t|--type <input-type> -p <name>=<value> ...]')
    print('  -i|--ifile     : [REQUIRED] mdb input file name.')
    print('  -o|--ofile     : [REQUIRED] glm output file name.')
    print('  -t|--type      : [REQUIRED] specify input type')
    print('  -p|--param     : [OPTIONAL] specify a conversion parameter')
    print('Input types')
    print('  cyme           : cyme input');

input_name = None
output_name = None
input_type = None
options = {}

try : 
    opts, args = getopt.getopt(sys.argv[1:],"hi:o:t:p:",["help","ifile=","ofile=","type=","param="])
except getopt.GetoptError:
    print("ERROR [mdb2glm]: command line options not valid")
    sys.exit(2)

if not opts : 
    help()
    sys.exit(1)
for opt, arg in opts:
    if opt in ("-h","--help"):
        help()
        sys.exit(0)
    elif opt in ("-i", "--ifile"):
        input_name = arg.strip()
    elif opt in ("-o", "--ofile"):
        output_name = arg.strip()
    elif opt in ("-t","--type"):
        input_type = arg
    elif opt in ("-p","--param"):
        spec = arg.split("=")
        options[spec[0]] = spec[1]

if input_type is None:
    print("ERROR [mdb2glm]: conversion type not specified (-t <type> option is missing)")
    sys.exit(1)

modname = sys.argv[0].replace("mdb2glm.py",f"mdb-{input_type}2glm.py")
if os.path.exists(modname):

    util.spec_from_file_location(input_type, modname)
    mod = importlib.import_module(f"mdb-{input_type}2glm")
    argv = copy.deepcopy(sys.argv)
    argv[0] = modname
    mod.convert(input_name,output_name,options)

else:

    print(f"ERROR [mdb2glm]: type '{input_type}' is not valid -- {modname} not found");
    sys.exit(2)