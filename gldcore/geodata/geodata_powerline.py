"""GridLAB-D Geodata Powerline Package

The powerline package computes geographic information about powerlines, such as
line sag, line sway, and line gallop.

All powerline package calculations require the following data:

    latitude - required column in the data

        The latitude is specified as a float with positive north and negative
        south orientation.

    longitude - required column in the data

        The longitude is specified as a float with position east and negative
        west orientation.

    pole_height - required column in the data

        The pole height is specified in meters.

    cable_type - required either in the data or provided in the options

        The cable types are listed in the file geodata_powerline_cabletypes.csv
        located in the GridLAB-D shared geodata folder, which is by default
        /usr/local/share/gridlabd/geodata.

    distance - optional column in the data, if absent it will be computed

        The distance between from the first pole in the data series.

    heading - optional column in the data, if absent it will computed

        The heading from the last pole encountered in the data series.

In addition, the following optional values are supported:

    elevation - optional column in the data, by default 0.0

    powerflow - optional column in the data, by default 0.0, may be provided in
        the options

    wind_speed - optional column in the data, by default 0.0, may be provided in
        the options

    wind_direction - optional column in the data, by default 0.0, may be
        provided in the options

    air_temperature - optional column in the data, by default 30 degC, may be
        provided in the options

    global_horizontal_irradiance - optional column in the data, by default
        1000.0 W/m^2, may be provided in the options

    ground_reflectance - optional column in the data, by default 0.3 pu, may be
        provided in the options

    ice_density - optional column in the data, by default 915.0 kg/m^3, may be
        provided in the options

    ice_thickness - optional column in the data, by default 0.0, may be
        provided in the options

The follows options may also be set:

    nominal_temperature - the temperature at which line loads are based, by
        default 15.0 degC.

LINE SAG

Line sag is the drop in line elevation above ground resulting from a
combination of the line weight, line elasticity, and line temperature. Note
that line sag may be affected by line sway (see below) insofar as line elevation
about ground is reduced as line sway is increased.

If the line sway is non-zero, the line sag will be computed according to the
formula

  linesag(linesway)^2 = linesag(0)^2 - linesway^2

LINE SWAY

Line sway is the lateral displacement of the line as a result of lateral wind
forces on the line.

LINE GALLOP

Line gallop is a vertical multi-nodal oscillation of a line resulting from the
asymetric build up of ice on the line.

"""

version = 1

import os, sys
import requests
from math import cos, sin, atan2, pi, sqrt, atan, isnan
from scipy.interpolate import interp1d
import numpy as np
from PIL import Image
import pandas
import json
from IPython.display import display

def TODO(value=float('nan')):
    """TODO default function -- this should never be called in the final product"""
    from inspect import getframeinfo, stack
    caller = getframeinfo(stack()[1][0])
    print(f"TODO({value.__repr__()}): called from {caller.function}()@{caller.filename}:{caller.lineno}")
    return value

default_options = {
    "air_temperature" : 30.0, # degC
    "wind_speed" : 0.0, # m/s
    "wind_direction" : 0.0, # direction
    "ice_thickness" : 0.0, # m
    "power_flow" : 0.0, # W
    "global_horizontal_irradiance" : 1000.0, # W/m^2
    "ground_reflectance" : 0.3, # pu
    "ice_density" : 915.0, # kg/m^3
    "nominal_temperature" : 15.0, # degC - temperature at which line loads are based
    "cable_type" : '', # cable type
    "elevation" : 0.0, # default elevation (0.0 = sea level)
    "precision" : {
        "linesag" : 1,
    },
    "margin" : {
        "vertical" : 2.0,
        "horizontal" : 2.0,
    },
}

default_config = {
    "cabletype_file" : "/usr/local/share/gridlabd/geodata_powerline_cabletypes.csv",
}

OPTIONS = default_options
CONFIG = default_config
CABLETYPES = pandas.DataFrame()

# physical constants
g = 9.81 # Gravity of Earth, unit: m/s2
k_s = 5.6704e-8 # Stefan/Boltzmann constant

def WARNING(msg):
    """Default warning message handler"""
    raise Exception(msg)

def get_air_properties(temp):
    """Compute material properties at a given Temperature

    ARGUMENTS

        temp (float)   Air temperature at which properties are calculated

    RETURNS

        float   Specified mass of air, in kg/m^3

        float   Thermal conductivity of air, in W/m.degC

        float   Dynamic viscosity of air, N.s/m^2
    """
    # calculate the constant parameters
    # reference: http://home.eng.iastate.edu/~jdm/wind/TransmissionLineLoadingDesignCriteriaAndHTS.pdf

    specific_mass = -0.00342727*temp + 1.275 
    thermal_conductivity = 7.32727273e-05*temp + 0.02428182
    dynamic_viscosity = 4.7e-08*temp + 1.75045455e-05

    return specific_mass, thermal_conductivity, dynamic_viscosity

hold_values = {}
def hold0(name,value=None):
    """Perform zero-order hold of value

    Returns the previous value of the named variable if curretn value is None.
    Otherwise return the current value.

    ARGUMENTS

        name (str)      variable name

        value (object)  hold value (None for hold value), or 'init' to use
                        set to initial value of OPTIONS

    RETURNS

        object          current value or hold value
    """
    global hold_values
    if value == 'init':
        hold_values[name] = OPTIONS[name]
    elif value != None:
        hold_values[name] = value
        return value
    elif not name in hold_values.keys():
        return None
    else:
        return hold_values[name]

def get_line_tension_coefficient(d_hori):
    """Compute line tension coefficient for the line length

    ARGUMENTS

        d_hori (float or series)  The horizontal distance between the line ends

    RETURNS

        float or series   The line tension coefficient
    """
    # reference: IEC 60826:2017
    # fit of data is used
    # 0.50723325 - 0.29656325/(0.05*d_hori+1)
    return 0.2106718346 + 0.0003126614987*d_hori

def get_distance(pos1, pos2):
    """Compute haversine distance between two locations

    ARGUMENTS

        pos1, pos2 (float tuple)   Specifies the two geographic endpoints as a
                                   (latitude,longtitude) tuple
    """
    lat1 = pos1[0]*pi/180
    lat2 = pos2[0]*pi/180
    lon1 = pos1[1]*pi/180
    lon2 = pos2[1]*pi/180
    a = sin((lat2-lat1)/2)**2+cos(lat1)*cos(lat2)*sin((lon2-lon1)/2)**2
    return 6371e3*(2*atan2(np.sqrt(a),sqrt(1-a)))

def linesag(data):
    """Linesag calculations

    ARGUMENTS

        data (DataFrame)   Data at positions along powerline path where sag
                           must be computed.  Required columns are as follows:

            cable_type      Specifies the cable type as listed in
                           $GLD_ETC/geodata_powerline_cabletypes.csv. If NA,
                           then the last cable_type is used.  The first cable_type
                           must be specified.

            air_temperature    Outdoor air temperature in degC. If missing, sag will
                           be computed for 30 degC.

            wind_speed     Wind speed in m/s.  If missing, sag will be computed
                           without considering effect of wind speed

            wind_direction Wind direction in degree. If missing, sag will be
                           computed without consider effect of wind direction

            ice_thickness  Ice thickness on powerline. If missing, sag will be
                           computed without considering effect of ice loading.

            distance       Line run distance is calculated if missing.

            heading        Line run heading is calculated if missing.

    RETURNS

        DataFrame   The data input dataframe is returned with the "linesag"
                    column added.  If the line sag cannot be computed at a given
                    location, a "nan" value will be stored. If any "nan" values
                    are encountered, a warning will be generating explaining the
                    cause of the "nan" value(s).
    """

    # Sources:
    #  https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5795940/
    #

    data['linesag'] = float('nan') # default result
    result = data['linesag'].to_dict()

    # read cable specs from cable type
    global OPTIONS
    if not 'cable_type' in data.columns and not 'cable_type' in OPTIONS.keys():
        WARNING("cannot compute line sag without any cable type")
        return data['linesag']

    # read optional initial hold values
    for name,value in OPTIONS.items():
        if not name in data.columns:
            hold0(name,'init')
    air_temperature = hold0('air_temperature')
    wind_speed = hold0('wind_speed')
    wind_direction = hold0('wind_direction')
    ice_thickness = hold0('ice_thickness')
    power_flow = hold0('power_flow')
    global_horizontal_irradiance = hold0('global_horizontal_irradiance')
    ground_reflectance = hold0('ground_reflectance')
    ice_density = hold0('ice_density')
    cable_type = hold0('cable_type')
    elevation = hold0('elevation')

    # check lat,lon
    if not 'latitude' in data.columns or not 'longitude' in data.columns:
        WARNING("cannot compute line sag without latitude and longitude fields")
        return data['linesag']

    # TODO: vectorize this loop
    p0 = None # start pole lat,lon
    p1 = None # end pole lat,lon
    z0 = None # start pole sag (elevation+height)
    z1 = None # end pole sag height (elevation+height)
    ld = {} # line data at waypoints
    for id, line in data.iterrows():
        if 'cable_type' in data.columns:
            cable_type = hold0('cable_type',line['cable_type'])
        global CABLETYPES
        if not cable_type:
            WARNING(f"cable_type not specified")
            break
        elif not cable_type in CABLETYPES.index:
            WARNING(f"cable_type={cable_type.__repr__()} not found")
            break
        else:
            cable = CABLETYPES.loc[cable_type]

        # air Temperature
        if 'air_temperature' in data.columns:
            air_temperature = hold0('air_temperature',line['air_temperature'])

        # wind speed
        if 'wind_speed' in data.columns:
            wind_speed = hold0('wind_speed',line['wind_speed'])

        # wind direction
        if 'wind_direction' in data.columns:
            wind_direction = hold0('wind_direction',line['wind_direction'])

        # ice thickness
        if 'ice_thickness' in data.columns:
            ice_thickness = hold0('ice_thickness',line['ice_thickness'])

        # power flow
        if 'power_flow' in data.columns:
            power_flow = hold0('power_flow',line['power_flow'])

        # ground reflectance
        if 'ground_reflectance' in data.columns:
            ground_reflectance = hold0('ground_reflectance',line['ground_reflectance'])

        # elevation
        if 'elevation' in data.columns:
            elevation = hold0('elevation',line['elevation'])

        # pole height - only present for pole, absent at waypoints
        # breakpoint()
        if 'pole_height' in data.columns and not isnan(line['pole_height']):

            if not p0: # start of a line segment

                p0 = [line['latitude'],line['longitude']]
                d0 = 0.0
                z0 = elevation + line['pole_height']
                sag = z0 - elevation
                result[id] = sag

            else: # end of a line segment

                p1 = [line['latitude'],line['longitude']]
                d1 = get_distance(p0,p1)
                z1 = elevation + line['pole_height']
                sag = z1 - elevation
                result[id] = sag

                # compute linesag at waypoints
                for n,l in ld.items():
                    p = [l['latitude'],l['longitude']]
                    d_hori = get_distance(p0,p)
                    try:
                        elevation = l['elevation']
                    except:
                        pass
                    sag = get_sag_value(d_hori,line,cable,p0,p,z0,z1,
                        power_flow,global_horizontal_irradiance,ground_reflectance,
                        ice_thickness,wind_direction,air_temperature,wind_speed,ice_density)
                    sag = sag - elevation
                    result[n] = round(sag,OPTIONS["precision"]["linesag"])

                # reset for next segment
                p0 = p1
                z0 = z1
                ld = {}


        elif p0: # continue a line segment

            # end last segment
            ld[id] = line

        else:

            WARNING("ignoring waypoints before first pole")


    if ld:
        WARNING("ignoring waypoints after last pole")

    return pandas.DataFrame(result.values(),index=result.keys())

def get_sag_value(d_hori,line,cable,p0,p1,z0,z1,
        power_flow,global_horizontal_irradiance,ground_reflectance,
        ice_thickness,wind_direction,air_temperature,wind_speed,ice_density):
    # p1 is the location (lat, lon) of interested point, while p0 is the previous pole
    """Calculate line sag values"""
    global OPTIONS
    d_vert = abs(z0-z1)
    k_init = get_line_tension_coefficient(d_hori)
    span = sqrt(d_hori*d_hori + d_vert*d_vert)
    rts = cable['rated_tensile_strength']
    unit_weight = cable['unit_weight']
    # if d_vert > 30:
    #     k_init = min(k_init, 3.0*unit_weight*span*span/(2*d_vert*rts))
    H_init = rts*k_init
    sag_init = unit_weight*span*span/(8*H_init)
    P_rated = power_flow
    Vll_rated = cable['voltage_rating']
    Irms =  P_rated/(sqrt(3)*Vll_rated)
    R_20C = cable['nominal_resistance']
    coeff_Al = cable['resistivity']
    Q_I_coeff_first = Irms*Irms*R_20C*coeff_Al
    Q_I_coeff_constant = Irms*Irms*R_20C*(1-coeff_Al*(20.0+273.0))
    k_a = 1.0 - cable['reflectivity'] # solar radiation absorption coefficient
    GHI = global_horizontal_irradiance # unit: W/m2
    k_g = ground_reflectance # ground reflect
    diameter = cable['diameter']
    Q_S_constant = k_a*(diameter+2*ice_thickness)*(1+k_g)*GHI

    try:
        line_angle = line['heading']
    except:
        line_angle = 180*atan2(p1[0]-p0[0],p1[1]-p0[1])/np.pi

    phi = (wind_direction - line_angle)*np.pi/180
    k_angle = 1.194 - cos(phi) + 0.194*cos(2*phi) + 0.368*sin(2*phi)
    air_mass, k_f, air_viscosity = get_air_properties(air_temperature)
    Nre = wind_speed * (diameter + 2*ice_thickness) * air_mass / air_viscosity
    if wind_speed < 0.81:
        Q_C_coeff_first = k_angle*(1.01+1.35*Nre**0.52)*k_f
        Q_C_constant = -k_angle*(1.01+1.35*Nre**0.52)*k_f*(air_temperature+273.0)
    else:
        Q_C_coeff_first = k_angle*0.754*(Nre**0.6)*k_f
        Q_C_constant = -k_angle*0.754*(Nre**0.6)*k_f*(air_temperature+273.0)
    # for Q_R
    k_e = cable['emissivity']
    Q_R_constant = -5.6704e-8*k_e*(diameter+2.0*ice_thickness)*(air_temperature+273.0)**4
    Q_R_coeff_fourth = 5.6704e-8*k_e*(diameter+2.0*ice_thickness)
    # for new conductor temp under loading
    coef_sag = [-Q_R_coeff_fourth,0.0,0.0,Q_I_coeff_first-Q_C_coeff_first,Q_I_coeff_constant+Q_S_constant-Q_C_constant-Q_R_constant]
    r = np.roots(coef_sag)
    r = r[~np.iscomplex(r)]
    temp_load = np.absolute(r[r > 0.0]) # unit: K
    temp_load = temp_load - 273.0 # unit: DegC
    # calculate the new line sag at loaded condition
    ice_unit_weight = ice_density*np.pi*ice_thickness*(diameter+ice_thickness)*g
    wind_unit_weight = 0.5*air_mass*(wind_speed*sin(phi))**2 *(diameter+2*ice_thickness)
    total_unit_weight = sqrt(wind_unit_weight**2+(unit_weight+ice_unit_weight)**2)

    area = cable['conductor_crosssection_area']
    elasticity = cable['elasticity']
    temp_init = OPTIONS['nominal_temperature']
    coef_thermal = cable['thermal_expansion']
    H_load_second = (unit_weight*d_hori)**2 *area*elasticity/(24*H_init**2)-H_init+(temp_load-temp_init)*coef_thermal*area*elasticity
    H_load_constant = -(total_unit_weight*d_hori)**2 *area*elasticity/24
    coef_H = [1, float(H_load_second), 0.0, float(H_load_constant)]
    r = np.roots(coef_H)
    r = r[~np.iscomplex(r)]
    H_load = np.absolute(r[r > 0.0])
    #
    sag_load = total_unit_weight*span*span/(8*H_load)
    sag_angle = atan(wind_unit_weight/(ice_unit_weight+unit_weight))
    C_catenary = H_load / (ice_unit_weight+unit_weight)
    if z0 > z1:
        d0_hori = d_hori*(1+d_vert/(4*sag_load))/2
        sag0 = total_unit_weight*d0_hori**2 /(2*H_load)
        dt = get_distance(p0,p1)
        sag0_cosh = sag0 - C_catenary*(np.cosh((dt-d0_hori)/C_catenary)-1)
        sag_elevation = z0 - sag0_cosh*cos(sag_angle)
    else:
        d0_hori = d_hori*(1-d_vert/(4*sag_load))/2
        sag0 = total_unit_weight*d0_hori**2 /(2*H_load)
        dt = get_distance(p0,p1)
        sag0_cosh = sag0 - C_catenary*(np.cosh((dt-d0_hori)/C_catenary)-1)
        sag_elevation = z0 - sag0_cosh*cos(sag_angle)
    result = sag_elevation[0]
    # print(f"get_sag_value(d_hori={round(d_hori).__repr__()},p0={p0.__repr__()},p1={p1.__repr__()},z0={z0.__repr__()},z1={z1.__repr__()},...) --> {result}")
    return result

def linesway(data):
    """TODO"""
    data['linesway'] = 0.0 # default result

    if not 'wind_speed' in data.columns or not 'wind_direction' in data.columns:
        return data['linesway']

    WARNING("linesway not implemented yet")
    data['linesway'] = TODO() # default result

    return data['linesway']

def linegallop(data):
    """TODO"""
    #
    # Galloping due to icing effect
    #
    # Frequency calculation
    #
    #   F (Hz) = sqrt(T*g/W) * 0.5*n/L
    #
    # where:
    #
    #   T = conductor tension (N)
    #   g = 9.81 m/s^2 (gravitational acceleration)
    #   W = conductor weight (N/m)
    #   L = conductor length
    #   n = standing wave node count (>=1)
    #
    # See https://preformed.com/images/pdfs/Energy/Transmission/Motion_Control/Air_Flow_Spoiler/Conductor_Galloping_Basics-EN-ML-1166.pdf
    #
    # Magnitude calculation
    #
    # Mitigation strategies
    #
    #  1) Detuning pendulum
    #  2) Airflow spoiler
    #
    # Source: https://preformed.com/images/pdfs/Energy/Transmission/Motion_Control/Air_Flow_Spoiler/Conductor_Galloping_Basics-EN-ML-1166.pdf
    #
    data['linegallop'] = 0.0 # default result

    if not 'ice_thickness' in data.columns:
        return data['linegallop']

    WARNING("linegallop not implemented yet")
    data['linegallop'] = TODO() # default result

    return data['linegallop']

def contact(data, options=default_options, config=default_config, warning=print):

    contact = ( data['linesag'] - data['height'] < options['margin']['vertical'] ) * data['cover']

    return contact

def apply(data, options=default_options, config=default_config, warning=print):

    global CABLETYPES
    CABLETYPES = pandas.read_csv(config['cabletype_file'],index_col='id')

    global WARNING
    WARNING = warning

    global OPTIONS
    OPTIONS = options

    global CONFIG
    CONFIG = config

    result = pandas.DataFrame(data)
    result["linesag"] = linesag(data)
    result["linesway"] = linesway(data)
    result["linegallop"] = linegallop(data)
    result["contact"] = contact(result)

    return result

# perform validation tests
if __name__ == '__main__':

    import unittest

    CABLETYPES = pandas.read_csv(sys.argv[0].replace('geodata_powerline.py','geodata_powerline_cabletypes.csv'),index_col='id')

    NA = float('nan')
    data = pandas.DataFrame({
        'latitude' : [37.41504514,37.41487158,37.41469802],
        'longitude' : [-122.2056472,-122.20706735,-122.2084875],
        'pole_height' : [18,NA,20],
        'elevation' : [88.45,93.7875,99.125],
        'cable_type' : ['TACSR/AC 610mm^2',None,None],
    })

    class TestPowerline(unittest.TestCase):

        def test_linesag(self):
            result = linesag(pandas.DataFrame(data))
            self.assertEqual(result.round(1).to_list(),[18.0, 12.3, 20.0])

        def test_linesway(self):
            result = linesway(pandas.DataFrame(data))
            self.assertEqual(result.to_list(),[0,0,0])

        def test_linegallop(self):
            result = linegallop(pandas.DataFrame(data))
            self.assertEqual(result.to_list(),[0,0,0])

    unittest.main()
