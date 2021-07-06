#!/usr/local/bin/python3
"""Global elevation data acquisition

REQUIREMENTS

Requires the elevation module (https://pypi.org/project/elevation/)
and the GDAL package (http://www.gdal.org).
"""
import os, subprocess
import pandas as pd
import numpy as np
import math
from PIL import Image
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

cache_folder = "/tmp"
CACHE = "/usr/local/share/gridlabd/elevation/10m"
COMMANDS = ["config","delete","get","help","index","list","path"]

def help(args=[]):
    """elevation help COMMAND - obtain help on an elevation command"""
    print("Syntax: elevation [OPTIONS] COMMAND [ARGUMENTS ...]")
    if args:
        if args[0] == "syntax":
            return
        elif args[0] in COMMANDS:
            cmd = globals()[args[0]].__doc__
            print(cmd)
            return
        else:
            error(f"command '{args[0]}' not found",E_SYNTAX)
    print("""
OPTIONS

  -h|--help           generate this output
  -v|--verbose        enable verbose output
  -w|--warning        disable warning output
  -q|--quiet          disable normal output
  -s|--silent         disable error output
  -c|--cache FOLDER   set cache folder

COMMANDS

  config get PARAMETER
         set PARAMETER VALUE
         show [PATTERN]
  delete BLOCK
  get BLOCK
  help [COMMAND]
  index [PATTERN]
  list [PATTERN]
  path -p|--points START STOP [-r|--resolution] METERS [-o|--output CSVOUT
       -i|--input CSVIN [-r|--resolution] METERS [-o|--output CSVOUT
""")
    return

VERBOSE = False # enable verbose output
def verbose(msg):
    if VERBOSE:
        print(msg, flush=True)

QUIET = False # disables normal output (except help output)
def output(msg):
    if not QUIET:
        print(msg, flush=True)

WARNING = True # enables warning messages
def warning(msg):
    if WARNING:
        print(f"WARNING [elevation]: {msg}", file=sys.stderr, flush=True)

SILENT = False # silence all errors
EXCEPTION = True # disable exit on error and raises exception instead
E_OK = 0 # ok exitcode
E_SYNTAX = 1 # syntax error exitcode
def error(msg,exitcode=None):
    if EXCEPTION:
        raise Exception(msg)
    if not SILENT:
        print(f"ERROR [elevation]: {msg}", file=sys.stderr, flush=True)
    if exitcode != None:
        exit(exitcode)

def project(data,box,lat,lon):
	x = int((lon-box["W"])/(box["E"]-box["W"])*data.shape[0])
	y = int((lat-box["N"])/(box["S"]-box["N"])*data.shape[1])
	return x,y

def get_values(specs,path,res=10,box={}):

    dd = res*39/12 # image resolution in feet
    img = Image.open(specs)
    data = np.array(img)
    result = []
    dt = 0
    p = {"dt":[],"zz":[]}
    for line in path:
        p0 = line["p0"]
        p1 = line["p1"]
        x0,y0 = project(data,box,p0["lat"],p0["lon"])
        x1,y1 = project(data,box,p1["lat"],p1["lon"])
        dx = x1-x0
        dy = y1-y0
        s = 0.0
        n = 0
        zz = []
        tt = []
        if abs(dx) > abs(dy): # iterate over x
            y = y0
            r = dy/dx
            for x in range(x0,x1+1,np.sign(dx)):
                zz.append(data[int(x),int(y)])
                tt.append(np.sqrt((x0-x)*(x0-x)+(y0-y)*(y0-y))*dd)
                y += r
        else: # iterate over y
            x = x0
            r = dx/dy
            for y in range(y0,y1+1,np.sign(dy)):
                zz.append(data[int(x),int(y)])
                tt.append(np.sqrt((x0-x)*(x0-x)+(y0-y)*(y0-y))*dd)
                x += r
        d = round(np.sqrt(dx*dx+dy*dy)*dd) # distance in feet
        zz = np.array(zz)
        t = np.array(tt)
        z = (list(map(lambda x:float(x/res/res),list(map(interp1d(t,zz),np.arange(tt[0],tt[-1],1.0))))))
        result.append({
            "from" : p0,
            "to" :p1,
            "min":zz.min(),
            "max":zz.max(),
            "avg":np.round(zz.mean(),1),
            "std":np.round(zz.std(),1),
            "len":d,
            "t" : np.arange(tt[0],tt[-1],1.0) + dt,
            "z": np.array(z).round(3)*100,
        })
        p["dt"].append(dt)
        p["zz"].append(zz[0])
        dt += d
    return result, p

def get_elevation(bottom,left):
    right = left + 1
    top = bottom + 1
    if left < 0:
        lon = f"{-left}W"
    elif left > 0:
        lon = f"{left}E"
    else:
        lon = "0"
    if bottom < 0:
        lat = f"{-bottom}S"
    elif bottom > 0:
        lat = f"{bottom}N"
    else:
        lat = "0"
    tif_file = f"{cache_folder}/{lat}_{lon}.tif"
    if not os.path.exists(tif_file):
        # unfortunately the python API call to elevation.clip does not work correctly
        result = subprocess.run(["/usr/local/bin/eio",
            "--product","SRTM1",
            "clip",
            "-o",tif_file,
            "--bounds",str(left),str(bottom),str(right),str(top)],
            capture_output=True)
        if result.returncode:
            return {
                "error" : result.returncode,
                "stderr" : result.stderr.decode("utf-8"),
                "stdout" : result.stdout.decode("utf-8"),
            }
    else:
        result = None
    return result, tif_file

def get_data(csv_file): # default resolution is 1 ft
    """Get elevation data for region around path in CSV file.

    PARAMETERS

      csv_file: path along which elevation data is desired
                latitude,longitude columns must be labeled in first row
    """
    line_data = pd.read_csv(csv_file)

    bottom = math.floor(line_data["latitude"].min())
    left = math.floor(line_data["longitude"].min())
    try:
        result, tif_file = get_elevation(bottom,left)
    except Exception as err:
        return {
            "error" : -1,
            "stderr" : "unable to get elevation data",
            "stdout" : None,
        }

    lats = line_data["latitude"]
    lons = line_data["longitude"]
    start = {"lat" : lats[0], "lon" : lons[0]}
    path = []
    for line in range(1,len(lats)):
        stop = {"lat" : lats[line], "lon" : lons[line]}
        path.append({"p0":start,"p1":stop})
        start = stop
    results, poles = get_values(tif_file,path,10,{"N":bottom+1,"E":left+1,"W":left,"S":bottom})

    data = []
    for segment in results:
        data.append(pd.DataFrame({"t":segment["t"],"z":segment["z"]}).set_index("t"))
    pd.concat(data).to_csv(csv_file.replace(".csv","_elevation.csv"))

    return {
        "filename" : tif_file,
        "left" : left,
        "bottom" : bottom,
        "right" : left+1,
        "top" : bottom+1,
        "elevation" : results,
        "poles" : poles,
        }

def download_all(lats, lons):
    for lat in lats:
        for lon in lons:
            print(f"Processing {lat},{lon}...")
            get_elevation(lat,lon)

def main(argc,argv):
    if argc < 2:
        help("syntax")
        exit(E_SYNTAX)
    n = 1
    while n < argc:
        if argv[n] in ["-h","--help"]:
            help()
            exit(E_OK)
        elif argv[n] in ["-v","--verbose"]:
            VERBOSE = not VERBOSE
            n += 1
        elif argv[n] in ["-q","--quiet"]:
            QUIET = not QUIET
            n += 1
        elif argv[n] in ["-s","--silent"]:
            SILENT = not SILENT
            n += 1
        elif argv[n] in ["-w","--warning"]:
            WARNING = not WARNING
            n += 1
        elif argv[n] in ["-c","--cache"]:
            try:
                CACHE = argv[n+1]
            except:
                error(f"missing cache folder name",E_SYNTAX)
            n += 2
        elif argv[n] in COMMANDS:
            if argc >= n:
                args = argv[n+1:]
            else:
                args = []
            exit(globals()[argv[n]](args))
        elif argv[n] == "delete":
            exit(delete(argv[n+1:]))
        else:
            error(f"option '{argv[n]}' is invalid",E_SYNTAX)

if not os.path.exists(CACHE):
    os.makedirs(CACHE)

if __name__ == '__main__':
    import sys
    EXCEPTION = False
    main(len(sys.argv),sys.argv)

    # results = get_data("module/resilience/docs/230kV_example.csv")
    # if "error" in results.keys():
    #     print(results["stderr"])
    # else:
    #     img = Image.open(results["filename"])
    #     img.show()
    # plt.figure(1,figsize=(15,5))
    # elevation = results["elevation"]
    # poles = results["poles"]
    #
    # t = []
    # z = []
    # for result in elevation:
    #     t.extend(result["t"])
    #     z.extend(result["z"])
    # plt.plot(t,z,"-k")
    # plt.plot(poles["dt"],poles["zz"],"or")
    # plt.xlabel('Line location (ft)')
    # plt.ylabel('Elevation (m)')
    # plt.grid()
    # start = [round(elevation[0]["from"]["lat"],4),round(elevation[0]["from"]["lon"],4)]
    # stop = [round(elevation[-1]["to"]["lat"],4),round(elevation[-1]["to"]["lon"],4)]
    # plt.title(f"{results['filename']} from {start} to {stop}")
    # plt.show()
