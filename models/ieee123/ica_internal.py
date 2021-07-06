import re
import time
import gridlabd
import numpy as np
import pandas as pd
from pprint import pprint
from datetime import datetime

ica_dict = {'underground_line_conductor': {'configuration': 0,
                                            'rating.summer.continuous': 'rating',
                                           'rating.winter.continuous': 'rating'},
            'overhead_line_conductor': {'configuration': 0,
                                        'rating.summer.continuous': 'rating',
                                        'rating.winter.continuous': 'rating'},
            'transformer_configuration': {'configuration': 0,
                                          'power_rating': 'rating',
                                          'powerA_rating': 'rating',
                                          'powerB_rating': 'rating',
                                          'powerC_rating': 'rating',
                                          'primary_voltage': 'deviation',
                                          'secondary_voltage': 'deviation'},
            'regulator_configuration': {'configuration': 0,
                                        'raise_taps': 'limit',
                                        'lower_taps': 'limit'},
            'substation': {'configuration': 0,
                           'nominal_voltage': 'deviation'},
            'triplex_meter': {'configuration': 0,
                              'nominal_voltage': 'deviation'},
            'meter': {'configuration': 0,
                      'nominal_voltage': 'deviation'}}

ica_class_list = list(ica_dict.keys())

df = pd.DataFrame(columns = ["ELEMENT", 'ATTRIBUTE', 'VALUE TO COMPARE'])

comp_dict = {}

def on_init(t):
    global df
    date_time = datetime.fromtimestamp(t)
    date_time = date_time.strftime("%m/%d/%Y, %H:%M:%S")
    gridlabd_element_list = gridlabd.get("objects")
    
    i = 0

    for element in gridlabd_element_list:
        element_specs = gridlabd.get_object(element)
        element_class = element_specs.get('class')
        for ica_element in ica_class_list:
            if ica_element == element_class:
                # print(element, ica_element, element_class, i, date_time)
                ica_element_attributes = ica_dict[ica_element]
                # print(ica_element_attributes)
                i += 1
                for attribute in ica_element_attributes:
                    if attribute in element_specs:
                        # converting data to non_decimal
                        non_decimal = re.compile(r'[^\d.]+')

                        if len(element_specs[attribute].split()) >= 2:
                            simulation_value = element_specs[attribute].split()[0]
                            simulation_unit = element_specs[attribute].split()[1]
                        else:
                            simulation_value = element_specs[attribute].split()[0]
                            simulation_unit = 'N/A'
                        simulation_value = float(non_decimal.sub('', element_specs[attribute]))

                        # print(i, element, ica_element + '.' + attribute, gridlabd.get_global(ica_element + '.' + attribute))
                        
                        # this values come from the 'ica_config_file.csv' that gridlabd already processed
                        attribute_thresh = gridlabd.get_global(ica_element + '.' + attribute)
                        
                        if '%' in attribute_thresh:
                            attribute_thresh = float(attribute_thresh.strip('%'))/100
                            if ica_element_attributes[attribute] == 'rating':
                                comparative_value = simulation_value * attribute_thresh
                                # print('RATING:' , comparative_value)
                            elif ica_element_attributes[attribute] == 'deviation':
                                comparative_value = [simulation_value * (1 + i) for i in [+attribute_thresh, -attribute_thresh]]
                                # print('MAX:', max(comparative_value))
                                # print('MIN:', min(comparative_value))
                            else:
                                # print('INCONGRUENCE: magnitud with unit')
                                gridlabd.warning('Element: {}, should not have {} attribute'.format(element,element_class))
                        elif ica_element_attributes[attribute] == 'limit':
                            comparative_value = simulation_value
                            # print('LIMIT:', comparative_value)
                            
                        else:
                            comparative_value = attribute_thresh

                        
                        result = [element, attribute, comparative_value]
                        df = df.append(dict(zip(df.columns, result)), ignore_index=True)
                        
            else:
                pass
    
    pprint(df)


    return True



violation_registry = pd.DataFrame(columns = ['ELEMENT',
                                             'ATTRIBUTE',
                                             'VALUE TO COMPARE',
                                             'ACTUAL VALUE',
                                             'VIOLATION STATUS',
                                             'TIME OF THE VIOLATION'])

def on_commit(t):
    global violation_registry
    date_time = datetime.fromtimestamp(t)
    date_time = date_time.strftime("%m/%d/%Y, %H:%M:%S")
    elements = df['ELEMENT'].unique()

    for element in elements:
        element_df = df[df['ELEMENT'] == element]
        element_attributes = element_df['ATTRIBUTE']
        for attribute in element_attributes:
            try:
                result_value = gridlabd.get_value(element, attribute)
                non_decimal = re.compile(r'[^\d.]+')
                
                # if len(result_value.split()) >= 2:
                #     result_value = result_value.split()[0]
                #     result_unit = result_value.split()[1]
                # else:
                #     result_value = result_value.split()[0]
                #     result_unit = 'N/A'

                result_value = float(non_decimal.sub('', result_value))

                value_to_compare = element_df[element_df['ATTRIBUTE'] == attribute]['VALUE TO COMPARE'].values
                value_to_compare = np.max(value_to_compare[0])


                if value_to_compare != 0 and value_to_compare < result_value:
                    violation_status = True
                    result = [element, attribute, value_to_compare, result_value, violation_status, date_time]
                    violation_registry = violation_registry.append(dict(zip(violation_registry.columns, result)), ignore_index=True)
                    print(result)
                    new_value = (result_value + value_to_compare) / 2
                    new_value = round(new_value, 0)
                    gridlabd.set_value(element, attribute, str(new_value))

            except:
                raise
                
    violation_registry.to_csv('results.csv')
    for i in range(0,0):
        time.sleep(1)
        print(i+1, "\n")

    return gridlabd.NEVER
