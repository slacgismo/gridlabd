import json
import csv 
import os 
import sys, getopt
import pandas as pd

input_file = "SiteOneLine.csv"

full_dict = {'data source' : 'census', 'data year' : '2016', 'region' : 'pacific northwest'}
census_dict = {}
property_list_census = ["Total Floor Area", "Building Category","Building Type","Primary Heating Fuel","Primary Cooling System",\
"Electric Vehicles Present","Electric Vehicle Type","Electric Vehicle Qty","Electric Vehicle Charging Station","Electrical Panel Type","Electric Panel Capacity",\
"Solar Panels Present","Solar Panels Rated kW","Solar Panels Battery Backup Present","Solar Battery Backup Capacity",\
"Total Wall Area","Window Area","Whole House UA","Audio Equipment Qty","Total Installed Lighting","Dehumidifier Qty","Desktop Qty",\
"Dishwasher Qty","Dryer Qty","Freezer Qty","Game Console Qty","Laptop Qty","Large Unusual Load Qty","Power Strip Qty","Refrigerator Qty",\
"Stove/Oven Qty","Television Qty","Thermostat Qty","Washer Qty","Faucet Qty","Showerhead Qty","Annual Electric Usage (kWh)","Annual Gas Usage (Therms)"]

property_list_gridlabd = ["Total Floor Area", "Primary Heating Fuel" ,"Primary Cooling System"\
"Total Wall Area","Whole House UA"\
"Solar Panels Present","Solar Panels Rated kW","Solar Panels Battery Backup Present","Solar Battery Backup Capacity",\
"Annual Electric Usage (kWh)","Annual Gas Usage (Therms)"]

property_lookup_table = {'Total Floor Area' : 'floor_area','Primary Heating Fuel' : 'heating_system_type', 'Primary Cooling System' : 'cooling_system_type', 'Total Wall Area' : 'gross_wall_area', \
'Whole House UA' : 'envelope_UA', 'Solar Panels Present' : 'thermal_storage_present', 'Solar Battery Backup Capacity' : 'design_peak_solar', \
'Annual Electric Usage (kWh)' : 'panel.power', 'Annual Gas Usage (Therms)' : 'gas_enduses', 'Electric' : 'RESISTANCE', 'Air Source Heat Pump': 'HEAT_PUMP', 'Natural Gas' : 'GAS', 'NONE' : 'NONE', 'Electric' : 'ELECTRIC'}

property_dict_gridlabd = {"floor_area": "DECIMAL", "heating_system_type" : {'RESISTANCE','HEAT_PUMP','GAS','NONE'}, "cooling_system_type" : {'HEAT_PUMP','ELECTRIC','NONE'}, \
"gross_wall_area" : "DECIMAL", "solar_heatgain_factor" : "DECIMAL", "envelope_UA" : "DECIMAL", "thermal_storage_present" : "DECIMAL", \
"design_peak_solar" : "DECIMAL", "panel.power" : "DECIMAL", "gas_enduses" : {"WATERHEATER|RANGE|DRYER"} }


config = {"output":"json","type":["all_census_format", "reduced_census_format", "glm_format" ]}

def help():
	return """census2json.py -o <outputfile> [options...]
    -c|--config              output converter configuration data
    -h|--help                output this help
    -o|--ofile <filename>    [OPTIONAL] GLM output file name
    -t|--type 				 type of input file <all_census_format>, <reduced_census_format>, <glm_format>
"""

def main():
	filename_json = ''
	json_type = ''
	try : 
		opts, args = getopt.getopt(sys.argv[1:],"cho:",["config","help","ofile=","type="])
	except getopt.GetoptError:
		sys.exit(2)
	if not opts : 
		print('Missing command arguments')
		sys.exit(2)
	for opt, arg in opts:
		if opt in ("-c","--config"):
			print(config)
			sys.exit()
		elif opt in ("-h","--help"):
			print(help())
			sys.exit()
		elif opt in ("-o", "--ofile"):
			filename_json = arg
		elif opt in ("-t", "--type"):
			json_type = arg
		else : 
			error(f"{opt}={arg} is not a valid option")

	convert(ofile=filename_json,output_type=json_type)


def convert(ofile,output_type) :
	path = os.path.dirname(os.path.abspath(__file__))
	for root,dirs,files in os.walk(path) : 
		census_df = pd.read_csv(input_file, index_col=0, low_memory=False)
		# print(census_df.columns)
		for item in census_df.index.values : 
			for col_name in census_df.columns :
				if output_type == "all_census_format" : 
					if item not in census_dict : 
						census_dict[item]={col_name : str(census_df[col_name][item])}
					else : 
						census_dict[item][col_name] = str(census_df[col_name][item])
				elif output_type == "reduced_census_format" : 
					if item not in census_dict : 
						if str(col_name) in property_list_census and str(census_df[col_name][item]) != "nan" and str(census_df[col_name][item]) != "Unknown": 
							census_dict[item]={col_name : str(census_df[col_name][item])}
					else : 
						if str(col_name) in property_list_census and str(census_df[col_name][item]) != "nan" and str(census_df[col_name][item]) != "Unknown": 
							census_dict[item][col_name] = str(census_df[col_name][item])
				elif output_type == "glm_format" : 
					if item not in census_dict : 
						if str(col_name) in property_list_census and str(census_df[col_name][item]) != "nan" and str(census_df[col_name][item]) != "Unknown" and col_name in property_lookup_table.keys():
							if property_dict_gridlabd[property_lookup_table[col_name]]=="DECIMAL" :
								census_dict[item]={property_lookup_table[col_name] : str(census_df[col_name][item])}
							else : 
								if str(census_df[col_name][item]) in property_lookup_table.keys() :
									census_dict[item]={property_lookup_table[col_name] : property_lookup_table[str(census_df[col_name][item])]}
							# census_dict[item]={col_name : property_lookup_table[str(census_df[col_name][item])]}
					else : 
						if str(col_name) in property_list_census and str(census_df[col_name][item]) != "nan" and str(census_df[col_name][item]) != "Unknown" and col_name in property_lookup_table.keys(): 
					# 		census_dict[item][col_name] = str(census_df[col_name][item])
							if property_dict_gridlabd[property_lookup_table[col_name]]=="DECIMAL"  :
								census_dict[item][property_lookup_table[col_name]] = str(census_df[col_name][item])
							else : 
								if str(census_df[col_name][item]) in property_lookup_table.keys() :
									census_dict[item][property_lookup_table[col_name]] =  property_lookup_table[str(census_df[col_name][item])]

	full_dict['site ids'] = census_dict 
	with open(ofile, "w") as outfile:  
		json.dump(full_dict, outfile, indent=3) 	


if __name__ == '__main__':
	main()
