
'''
There are 3 ways to set thresholds.
1) Read the user-inputted value from the config.csv
    a. Into main .py function using csv converter -OR-
    b. Directly into this .py file (can tailor csv more, but inaccessible outside)
2) Read a default values from the config.csv

Load default values first. Load modifies (if necessary). Just different csv files. This is an overwrite, not delete.
first do input defaults, then input user settings.
3) Read in defaults based on library properties

These will be TWO choices in the config.csv
'''



'''
ON_INIT
1) Based on user-selected option, defines thresholds for each parameter.
2) Creates a general dictionary of properties to check for each class and their thresholds.
3) Populates a global master dictionary with each object, properties to check, and their thresholds.
'''
import pandas as pd

def on_init(t):
    #If Option 1, global thresholds will have already been set through csv converter
    #TODO: Add 'ica_' tag to all globals
    
    #If Option 2, read config file directly into this .py script. Create a global for each entry.
    if gridlabd.get_global('ica_option') == 2:
        config_globals = pd.read_csv("ica_config.csv")
        for index in range(len(config_globals)):
            gridlabd.set_global("ica_" + config_globals.iloc[0,index],config_globals.iloc[1,index])
    
    #Create a dict of objs where user may have set threshold as a %
    #RATING: set the threshold as a % of the max rating. DEVIATION: set the threshold as a +-% from the nominal rating.
    #Note: If an obj is NOT in this dict, that means the user should NOT have input a % as a threshold. 
    percent_map_dict = {'ica_underground_line':{'configuration':1,'rating_summer_continuous':'rating','rating_winter_continuous':'rating'},\
                    'ica_overhead_line':{'configuration':1,'rating_summer_continuous':'rating','rating_winter_continuous':'rating'},\
                    'ica_transformer':{'configuration':1,'power_rating':'rating','powerA_rating':'rating','powerB_rating':'rating','powerC_rating':'rating','primary_voltage':'deviation','secondary_voltage':'deviation'},\
                    'ica_substation':{'configuration':0,'nominal_voltage':'deviation'},\
                    'ica_triplex_meter':{'configuration':0,'nominal_voltage':'deviation'},\
                    'ica_meter':{'configuration':0,'nominal_voltage':'deviation'}}
    
    global_list = gridlabd.get("globals")
    
    
    for g in global_list:
        if 'ica_' in g:
            val = gridlabd.get_global(g)
            #TODO: Check that value makes sense
            if '%' in val:
                global_string = g.split('.')
                obj = global_string[0]
                prop = global_string[1]

                if obj in percent_map_dict.keys():
                    percent = gridlabd.get_global(g).strip('%')/100    
                    obj_dict = percent_map_dict.get(obj)
                    #TODO: Make sure obj names all either do or do not have ica_
                    if obj_dict.get('configuration') == 0:
                        lib_prop = gridlabd.get_value(obj,prop)
                    else:
                        config = gridlabd.get_value(obj,'configuration')
                        lib_prop = gridlabd.get_value(config,prop)

                    if obj_dict.get(prop) == 'rating':
                        gridlabd.set_global(obj,lib_prop*percent)
                    else:
                        gridlabd.set_global(obj+'max',lib_prop*(1.0+percent))
                        gridlabd.set_global(obj+'min',lib_prop*(1.0-percent))
                            
                else:
                    print('%s should not have a percentage as a threshold.' % obj)
                        

#    
#    #If the user set any thresholds as percentages, use object libraries to properly define thresholds
#    if len(library_list) > 0:
#        #QUESTION: Should we use this method to set thresholds ONLY for properties where input was a %? Yes.
#        for l in library_list:
#            global_string = l.split('.')
#            obj = global_string[0]
#            prop = global_string[1]
#            percent = gridlabd.get_global(l).strip('%')/100
#            config_list = ['underground_line','overhead_line','transformer','regulator']
#            rating_list = ['underground_line','overhead_line','transformer']
#            
#            if obj in config_list:
#                config = gridlabd.get_value(obj,'configuration')
#                if obj in rating_list:
#                    #Set max rating
#                    #TODO: Clean this up. String -> float -> string
#                    gridlabd.set_global(l,str(float(gridlabd.get_value(config,prop))*percent))
#                else:
#                    #Set max voltage
#                    #Set min voltage
#                    #QUESTION: Can set_global create, not just modify? We need to set a max AND a min. Alternatively, can use % to calculate max and min in on_commit.
#                    pass
#            else:
#                #Set max voltage
#                #Set min voltage
#                pass
            
            
            
            
                
    
    #create a dictionary of properties to check for each class & their thresholds
    #example below for a single class:
    gen_dict = {'substation':{'current_A':swing_I_thresh,'current_B':swing_I_thresh,'current_C':swing_I_thresh}}

    #create master dict
    #key = obj; val = dict {property1:thresh1,property2:thresh2}
    objects = gridlabd.get("objects")
    for obj in objects:
        #only get the classes of interest
        obj_class = gridlabd.get_object(obj)['class']
        if obj_class in gen_dict.keys():
            #append to master dict
            obj_dict.update({obj:gen_dict.get(obj_class)})

    return True
    
        

'''
ON_COMMIT
For each key in global dictionary, get the value for each property and compare it to the threshold.
If threshold is exceeded, record the object, property, and value, and exit.
'''

def on_commit(t):
    return True

