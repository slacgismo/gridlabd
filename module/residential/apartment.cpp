/*	File: apartment.cpp

	Copyright (C) 2020, Regents of the Leland Stanford Junior University
*/

#include "residential.h"

EXPORT_CREATE(apartment);
EXPORT_INIT(apartment);
EXPORT_PRECOMMIT(apartment);
EXPORT_SYNC(apartment);

CLASS *apartment::oclass = NULL;
apartment *apartment::defaults = NULL;

char1024 apartment::load_property = "base_power";
double apartment::maximum_temperature_update = 0.1;
TIMESTAMP apartment::maximum_timestep = 3600;

#define PASSCONFIG PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP

apartment::apartment(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"apartment",sizeof(apartment),PASSCONFIG);
		if (oclass==NULL)
			throw "unable to register class apartment";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,

			PT_object,"weather", get_weather_offset(),
				PT_DESCRIPTION,"weather object to use for outdoor conditions",

			PT_int16,"building_floors",get_building_floors_offset(), 
				PT_REQUIRED,
				PT_DEFAULT, "0",
				PT_DESCRIPTION,"number of floors in building",
			PT_double,"building_floor_depth[ft]",get_building_floor_depth_offset(), 
				PT_DEFAULT,"+2 ft", 
				PT_DESCRIPTION,"ceiling-to-floor depth",
			PT_double,"building_floor_height[ft]",get_building_floor_height_offset(), 
				PT_DEFAULT,"+8 ft", 
				PT_DESCRIPTION,"floor-to-ceiling height",
			PT_double,"building_heat_leakage[pu]",get_building_heat_leakage_offset(), 
				PT_DEFAULT,"+0.1 pu", 
				PT_DESCRIPTION,"fraction of zone heat gains that leak into mass",
			PT_double,"building_mass_temperature[degF]",get_building_mass_temperature_offset(), 
				PT_DESCRIPTION, "temperature of the building mass",
			PT_double,"building_occupancy_factor[pu]",get_building_occupancy_factor_offset(), 
				PT_DEFAULT,"+0.95 pu", 
				PT_DESCRIPTION,"fraction of building units that are occupied",
			PT_property,"building_outdoor_temperature",get_building_outdoor_temperature_offset(), 
				PT_DEFAULT,"residential::default_outdoor_temperature",
				PT_DESCRIPTION, "reference to an object containing temperature data",
			PT_property,"building_outdoor_humidity",get_building_outdoor_humidity_offset(), 
				PT_DEFAULT,"residential::default_humidity", 
				PT_DESCRIPTION, "reference to an object containing humidity data",
			PT_double,"building_overdesign_factor[pu]",get_building_overdesign_factor_offset(), 
				PT_DEFAULT, "+0.5 pu", 
				PT_DESCRIPTION,"overdesign factor for building systems",
			PT_property,"building_solar_gain",get_building_solar_gain_offset(), 
				PT_DEFAULT,"residential::default_solar", 
				PT_DESCRIPTION, "reference to an object containing solar data",

			PT_int16,"building_units",get_building_units_offset(), 
				PT_REQUIRED,
				PT_DEFAULT, "0",
				PT_DESCRIPTION,"number of units in the building",

			PT_set,"core_configuration",get_core_configuration_offset(), 
				PT_DEFAULT,"NONE", 
				PT_DESCRIPTION,"configuration of the core",
				PT_KEYWORD, "NONE", CC_NONE,
				PT_KEYWORD, "INDOOR", CC_INDOOR,
				PT_KEYWORD, "DOUBLE", CC_DOUBLE,
				PT_KEYWORD, "CONDITIONED", CC_CONDITIONED,
			PT_double,"core_cooling_setpoint[degF]",get_core_cooling_setpoint_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"cooling setpoint in the core space",
			PT_int16,"core_elevators",get_core_elevators_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"number of elevators operating in the core space",
			PT_double,"core_heating_setpoint[degF]",get_core_heating_setpoint_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"heating setpoint in the core space",
			PT_set,"core_mode",get_core_mode_offset(), 
				PT_DEFAULT,"OFF", 
				PT_DESCRIPTION,"operating mode of the core zone",
				PT_KEYWORD, "OFF", SPM_OFF,
				PT_KEYWORD, "VENTILATING", SPM_VENTILATING,
				PT_KEYWORD, "HEATING", SPM_HEATING,
				PT_KEYWORD, "COOLING", SPM_COOLING,
			PT_int16,"core_laundry_units",get_core_laundry_units_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"number of community/shared washer/dryers pairs installed in the core space",
			PT_double,"core_temperature[degF]",get_core_temperature_offset(), 
				PT_DESCRIPTION,"core zone air temperature",
			PT_double,"core_width[ft]",get_core_width_offset(),
				PT_DEFAULT,"+0 ft", 
				PT_DESCRIPTION,"width of the core space",

			PT_double,"parking_capacity_chargers[kW]",get_parking_capacity_chargers_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"installed vehicle charger capacity",
			PT_double,"parking_capacity_elevators[kW]",get_parking_capacity_elevators_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"installed elevator capacity",
			PT_double,"parking_capacity_lights[kW]",get_parking_capacity_lights_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"installed lighting capacity",
			PT_double,"parking_capacity_ventilation[kW]",get_parking_capacity_ventilation_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"installed ventilation capacity",
			PT_int16,"parking_chargers_active",get_parking_chargers_active_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"number of EV chargers active",
			PT_int16,"parking_chargers_installed",get_parking_chargers_installed_offset(), 
				PT_DEFAULT,"0", 
				PT_DESCRIPTION,"number of EV chargers installed",
			PT_enumeration,"parking_configuration",get_parking_configuration_offset(), 
				PT_DEFAULT,"OUTDOOR", 
				PT_DESCRIPTION,"parking configuration",
				PT_KEYWORD, "OUTDOOR", PC_OUTDOOR,
				PT_KEYWORD, "INDOOR", PC_INDOOR,
			PT_double,"parking_demand_chargers[pu]",get_parking_demand_chargers_offset(), 
				PT_DEFAULT,"+0 pu", 
				PT_DESCRIPTION,"parking EV charger demand as a fraction of installed capacity",
			PT_double,"parking_demand_elevators[pu]",get_parking_demand_elevators_offset(), 
				PT_DEFAULT,"+0 pu", 
				PT_DESCRIPTION,"parking elevator demand as a fraction of installed capacity",
			PT_double,"parking_demand_lights[pu]",get_parking_demand_lights_offset(), 
				PT_DEFAULT,"+0 pu", 
				PT_DESCRIPTION,"parking lighting demand as a fraction of installed capacity",
			PT_double,"parking_demand_ventilation[pu]",get_parking_demand_ventilation_offset(), 
				PT_DEFAULT,"+0 pu", 
				PT_DESCRIPTION,"parking ventilation demand as a fraction of installed capacity",
			PT_int16,"parking_size",get_parking_size_offset(), 
				PT_DEFAULT,"-1", 
				PT_DESCRIPTION,"total number of parking spots installed",

			PT_double,"power_core[kW]",get_power_core_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"power delivered to the core spaces",
			PT_double,"power_parking[kW]",get_power_parking_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"power delivered to the parking areas",
			PT_double,"power_system[kW]",get_power_system_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"power delivered to the building systems",
			PT_double,"power_total[kW]",get_power_total_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"total power delivered to the building",
			PT_double,"power_units[kW]",get_power_units_offset(), 
				PT_DEFAULT,"+0 kW", 
				PT_DESCRIPTION,"total power delivered to the apartment units",

			PT_double,"system_cooling_air_temperature[degF]",get_system_cooling_air_temperature_offset(), 
				PT_DEFAULT,"+50 degF", 
				PT_DESCRIPTION,"system cooling supply air temperature",
			PT_double,"system_cooling_capacity[kBtu/h]",get_system_cooling_capacity_offset(),
				PT_DEFAULT,"+0 kBtu/h",
				PT_DESCRIPTION,"system cooling capacity",
			PT_double,"system_cooling_design_temperature[degF]",get_system_cooling_design_temperature_offset(), 
				PT_DESCRIPTION,"system cooling design temperature",
			PT_double,"system_cooling_efficiency[kBtu/kWh]",get_system_cooling_efficiency_offset(),
				PT_DEFAULT,"+15 kBtu/kWh",
				PT_DESCRIPTION,"system cooling efficiency",
			PT_double,"system_heating_air_temperature[degF]",get_system_heating_air_temperature_offset(),
				PT_DEFAULT,"+110 degF",
				PT_DESCRIPTION,"system heating supply air temperature",
			PT_double,"system_heating_capacity[kBtu/h]",get_system_heating_capacity_offset(),
				PT_DEFAULT,"+0 kBtu/h",
				PT_DESCRIPTION,"system heating capacity",
			PT_double,"system_heating_design_temperature[degF]",get_system_heating_design_temperature_offset(), 
				PT_DESCRIPTION,"system heating design temperature",
			PT_double,"system_heating_efficiency[kBtu/kWh]",get_system_heating_efficiency_offset(),
				PT_DEFAULT,"+10 kBtu/kWh",
				PT_DESCRIPTION,"system heating efficiency",
			PT_enumeration,"system_mode",get_system_mode_offset(),
				PT_DEFAULT,"OFF",
				PT_DESCRIPTION,"operating mode of the central system",
				PT_KEYWORD, "OFF", SPM_OFF,
				PT_KEYWORD, "VENTILATING", SPM_VENTILATING,
				PT_KEYWORD, "HEATING", SPM_HEATING,
				PT_KEYWORD, "COOLING", SPM_COOLING,
			PT_set,"system_type_central",get_system_type_central_offset(),
				PT_DEFAULT,"NONE",
				PT_DESCRIPTION,"central system type, if any",
				PT_KEYWORD, "NONE", STC_NONE,
				PT_KEYWORD, "HEAT", STC_HEAT,
				PT_KEYWORD, "COOL", STC_COOL,
				PT_KEYWORD, "BOTH", STC_BOTH,
			PT_enumeration,"system_type_economizer",get_system_type_economizer_offset(),
				PT_DEFAULT,"NONE",
				PT_DESCRIPTION,"type of economizer used by the central cooling system, if any",
				PT_KEYWORD, "NONE", STE_NONE,
				PT_KEYWORD, "DRYBULB", STE_DRYBULB,
				PT_KEYWORD, "WETBULB", STE_WETBULB,
				PT_KEYWORD, "DIFFERENTIAL", STE_DIFFERENTIAL,
			PT_enumeration,"system_type_ventilation",get_system_type_ventilation_offset(),
				PT_DEFAULT,"NONE",
				PT_DESCRIPTION,"type of ventilation system, if any",
				PT_KEYWORD, "NONE", STV_NONE,
				PT_KEYWORD, "LOCAL", STV_LOCAL,
				PT_KEYWORD, "CENTRAL", STV_CENTRAL,

			PT_set,"unit_appliance_types",get_unit_appliance_types_offset(),
				PT_DEFAULT,"ALL",
				PT_DESCRIPTION,"types of appliances installed in the units",
				PT_KEYWORD,"NONE",(set)UAT_NONE,
				PT_KEYWORD,"ALL", (enumeration)UAT_ALL,
				PT_KEYWORD,"DISHWASHER",(enumeration)UAT_DISHWASHER,
				PT_KEYWORD,"REFRIGERATOR",(enumeration)UAT_REFRIGERATOR,
				PT_KEYWORD,"WASHER",(enumeration)UAT_WASHER,
				PT_KEYWORD,"DRYER",(enumeration)UAT_DRYER,
			PT_double,"unit_capacity_cooking[kW]",get_unit_capacity_cooking_offset(),
				PT_DEFAULT,"+3 kW",
				PT_DESCRIPTION,"maximum power of the cooking appliances in units",
			PT_double,"unit_capacity_dishwasher[kW]",get_unit_capacity_dishwasher_offset(),
				PT_DEFAULT,"+1.8 kW",
				PT_DESCRIPTION,"maximum power of the dishwashing appliances in units",
			PT_double,"unit_capacity_dryer[kW]",get_unit_capacity_dryer_offset(),
				PT_DEFAULT,"+3 kW",
				PT_DESCRIPTION,"maximum power demand of dryers in units",
			PT_double,"unit_capacity_lights[kW]",get_unit_capacity_lights_offset(),
				PT_DEFAULT,"+0.1 kW",
				PT_DESCRIPTION,"maximum lighting demand in units",
			PT_double,"unit_capacity_plugs[kW]",get_unit_capacity_plugs_offset(),
				PT_DEFAULT,"+0.5 kW",
				PT_DESCRIPTION,"maximum non-lighting plug load demand in units",
			PT_double,"unit_capacity_refrigerator[kW]",get_unit_capacity_refrigerator_offset(),
				PT_DEFAULT,"+0.2 kW",
				PT_DESCRIPTION,"maximum refrigeration demand in units",
			PT_double,"unit_capacity_washer[kW]",get_unit_capacity_washer_offset(),
				PT_DEFAULT,"+0.5 kW",
				PT_DESCRIPTION,"maximum washer power demand in units",
			PT_double,"unit_cooling_capacity[kBtu/h]",get_unit_cooling_capacity_offset(),
				PT_DEFAULT,"1 kW",
				PT_DESCRIPTION,"cooling capacity of the unit system, if any",
			PT_double,"unit_cooling_efficiency[kBtu/kWh]",get_unit_cooling_efficiency_offset(),
				PT_DEFAULT,"15 kBtu/kWh",
				PT_DESCRIPTION,"unit cooling system efficiency",
			PT_double,"unit_cooling_setpoint[degF]",get_unit_cooling_setpoint_offset(),
				PT_DEFAULT,"78 degF",
				PT_DESCRIPTION,"unit cooling system temperature setpoint",
			PT_double,"unit_demand_cooking[pu]",get_unit_demand_cooking_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the cooking power capacity that is active",
			PT_double,"unit_demand_dishwasher[pu]",get_unit_demand_dishwasher_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the dishwasher power capacity that is active",
			PT_double,"unit_demand_dryer[pu]",get_unit_demand_dryer_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the dryer power capacity that is active",
			PT_double,"unit_demand_lights[pu]",get_unit_demand_lights_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the lighting power capacity that is active",
			PT_double,"unit_demand_plugs[pu]",get_unit_demand_plugs_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the plugs power capacity that is active",
			PT_double,"unit_demand_refrigerator[pu]",get_unit_demand_refrigerator_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the refrigerator power capacity that is active",
			PT_double,"unit_demand_washer[pu]",get_unit_demand_washer_offset(),
				PT_DEFAULT,"+0 pu",
				PT_DESCRIPTION,"fraction of the washer power capacity that is active",
			PT_double,"unit_depth[ft]",get_unit_depth_offset(),
				PT_DEFAULT,"+40 ft",
				PT_DESCRIPTION,"interior to exterior dimension of unit",
			PT_double,"unit_door_area[sf]",get_unit_door_area_offset(),
				PT_DEFAULT,"+20 sf",
				PT_DESCRIPTION,"area of unit door",
			PT_double,"unit_heating_capacity[kBtu/h]",get_unit_heating_capacity_offset(),
				PT_DEFAULT,"+0 kBtu/h",
				PT_DESCRIPTION,"heating capacity of the unit system, if any",
			PT_double,"unit_heating_efficiency[kBtu/kWh]",get_unit_heating_efficiency_offset(),
				PT_DEFAULT,"+10 kBtu/kWh",
				PT_DESCRIPTION,"unit heating system efficiency",
			PT_double,"unit_heating_setpoint[degF]",get_unit_heating_setpoint_offset(),
				PT_DEFAULT,"+70 degF",
				PT_DESCRIPTION,"unit heating system temperature setpoint",
			PT_enumeration,"unit_mode",get_unit_mode_offset(),
				PT_DEFAULT,"OFF",
				PT_DESCRIPTION,"unit system mode",
				PT_KEYWORD, "OFF", SPM_OFF,
				PT_KEYWORD, "VENTILATING", SPM_VENTILATING,
				PT_KEYWORD, "HEATING", SPM_HEATING,
				PT_KEYWORD, "COOLING", SPM_COOLING,
			PT_set,"unit_system_type",get_unit_system_type_offset(),
				PT_DEFAULT,"NONE",
				PT_DESCRIPTION,"UNITSYSTEMTYPE",
				PT_KEYWORD, "NONE", UST_NONE,
				PT_KEYWORD, "HEAT", UST_HEAT,
				PT_KEYWORD, "COOL", UST_COOL,
				PT_KEYWORD, "BOTH", UST_BOTH,
			PT_double,"unit_temperature[degF]", get_unit_heating_efficiency_offset(),
				PT_DEFAULT, "+72 degF",
				PT_DESCRIPTION,"occupied unit air temperature",
			PT_double,"unit_width[ft]",get_unit_width_offset(),
				PT_DEFAULT,"+24 ft",
				PT_DESCRIPTION,"double",
			PT_double,"unit_window_area[sf]",get_unit_window_area_offset(),
				PT_DESCRIPTION,"area of unit door",

			PT_double,"vacant_cooling_setpoint[degF]",get_vacant_cooling_setpoint_offset(),
				PT_DEFAULT,"+120 degF",
				PT_DESCRIPTION,"vacant unit cooling setpoint",
			PT_double,"vacant_heating_setpoint[degF]",get_vacant_heating_setpoint_offset(),
				PT_DEFAULT,"+50 degF",
				PT_DESCRIPTION,"vacant unit heating setpoint",
			PT_enumeration,"vacant_mode",get_vacant_mode_offset(),
				PT_DEFAULT,"OFF",
				PT_DESCRIPTION,"vacant unit system mode",
				PT_KEYWORD, "OFF", SPM_OFF,
				PT_KEYWORD, "VENTILATING", SPM_VENTILATING,
				PT_KEYWORD, "HEATING", SPM_HEATING,
				PT_KEYWORD, "COOLING", SPM_COOLING,
			PT_double,"vacant_temperature[degF]",get_vacant_temperature_offset(),
				PT_DESCRIPTION,"vacant unit air temperature",

			PT_double,"Rext[Btu/degF/h]",get_Rext_offset(),
				PT_DEFAULT,"+19 Btu/degF/h",
				PT_DESCRIPTION,"exterior wall R-value",
			PT_double,"Rint[Btu/degF/h]",get_Rint_offset(),
				PT_DEFAULT,"+5 Btu/degF/h",
				PT_DESCRIPTION,"interior wall R-value",
			PT_double,"Rwindow[Btu/degF/h]",get_Rwindow_offset(),
				PT_DEFAULT,"+2 Btu/degF/h",
				PT_DESCRIPTION,"window R-value",
			PT_double,"Rdoor[Btu/degF/h]",get_Rdoor_offset(),
				PT_DEFAULT,"+3 Btu/degF/h",
				PT_DESCRIPTION,"door R-value",
			PT_double,"Rmass[Btu/degF/h]",get_Rmass_offset(),
				PT_DEFAULT,"+1 Btu/degF/h",
				PT_DESCRIPTION,"air-mass R-value",
			PT_double,"Rroof[Btu/degF/h]",get_Rroof_offset(),
				PT_DEFAULT,"+60 Btu/degF/h",
				PT_DESCRIPTION,"roof R-value",
			PT_double,"Rground[Btu/degF/h]",get_Rground_offset(),
				PT_DEFAULT,"+60 Btu/degF/h",
				PT_DESCRIPTION,"ground R-value",

			PT_bool,"solver_enable_dump",get_solver_enable_dump_offset(),
				PT_DEFAULT,"FALSE",
				PT_DESCRIPTION,"enable solver matrix dumps",
			PT_bool,"solver_enable_debug",get_solver_enable_debug_offset(),
				PT_DEFAULT,"FALSE",
				PT_DESCRIPTION,"enable solver debug output",
			PT_bool,"solver_enable_verbose",get_solver_enable_verbose_offset(),
				PT_DEFAULT,"FALSE",
				PT_DESCRIPTION,"enable solver verbose output",

			PT_double,"Q_AS",get_Q_AS_offset(),
			PT_double,"Q_AV",get_Q_AV_offset(),
			PT_double,"Q_AE",get_Q_AE_offset(),
			PT_double,"Q_US",get_Q_US_offset(),
			PT_double,"Q_CS",get_Q_CS_offset(),
			PT_double,"Q_CV",get_Q_CV_offset(),
			PT_double,"Econ",get_Econ_offset(),
			PT_double,"Tout",get_Tout_offset(),
			PT_double,"Tret",get_Tret_offset(),

			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
        gl_global_create("residential::load_property",PT_char1024,&load_property,NULL);
        gl_global_create("residential::maximum_temperature_update",PT_double,&maximum_temperature_update,PT_UNITS,"degF",NULL);
		}
	}
}

int apartment::create(void) 
{
	solver = NULL;
	minimum_temperature = NULL;
	maximum_temperature = NULL;
	outdoor_temperature = NULL;
	outdoor_humidity = NULL;
	global_horizontal = NULL;
	return 1; /* return 1 on success, 0 on failure */
}

int apartment::init(OBJECT *parent)
{
	// check parent
	if ( parent == NULL )
	{
		error("parent meter object is not specified");
		return 0;
	}
	gld_object *meter = get_object(parent);
	if ( ! meter->isa("meter","powerflow") )
	{
		error("parent object is not a powerflow meter");
		return 0;
	}
	else
	{
		gld_property phases(parent,"phases");
		if ( ! phases.is_valid() )
		{
			error("parent object does not publish phases");
			return 0;
		}
		if ( phases.get_set()&0x0100 ) // delta connection flag
		{
			power[0] = new gld_property(parent,"power_AB");
			power[1] = new gld_property(parent,"power_BC");
			power[2] = new gld_property(parent,"power_CA");
		}
		else
		{
			power[0] = new gld_property(parent,"power_A");
			power[1] = new gld_property(parent,"power_B");
			power[2] = new gld_property(parent,"power_C");
		}
	}

	// check weather
	if ( weather == NULL )
	{
		warning("weather object is not specified, using default temperature, humidity, and solar values");
	}
	else
	{
		minimum_temperature = new gld_property(weather,"minimum_temperature");
		if ( minimum_temperature == NULL )
		{
			warning("weather object does not publish minimum_temperature");
		}
		maximum_temperature = new gld_property(weather,"maximum_temperature");
		if ( maximum_temperature == NULL )
		{
			warning("weather object does not publish maximum_temperature");
		}
		outdoor_temperature = new gld_property(weather,"temperature");
		if ( outdoor_temperature == NULL )
		{
			warning("weather object does not publish temperature");
		}
		outdoor_humidity = new gld_property(weather,"humidity");
		if ( outdoor_humidity == NULL )
		{
			warning("weather object does not publish humidity");
		}
		global_horizontal = new gld_property(weather,"global_horizontal");
		if ( global_horizontal == NULL )
		{
			warning("weather object does not publish global_horizontal");
		}
	}

	// check for missing values
	if ( building_floors <= 1 )
	{
		exception("building_floors must be 1 or more");
	}
	if ( building_units <= 1 )
	{
		exception("building_units must be 1 or more");
	}
	if ( core_cooling_setpoint == 0 )
	{
		core_cooling_setpoint = unit_cooling_setpoint + 5.0;
	}
	if ( core_heating_setpoint == 0 )
	{
		core_heating_setpoint = unit_heating_setpoint - 5.0;
	}
	if ( core_width == 0 )
	{
		core_width = ( core_configuration&CC_DOUBLE ) ? 6.0 : 5.0;
	}
	if ( parking_size < 0 )
	{
		parking_size = building_floors*building_units;
	}
	if ( unit_cooling_capacity == 0 )
	{
		// TODO
		// Q_HA = 1.5 * ( U_OA * (Tmin_O-T_H_A) )
	}
	if ( unit_heating_capacity == 0 )
	{
		// TODO
	}
	if ( system_cooling_capacity == 0 )
	{
		// TODO
		// building_units * ()
	}
	if ( system_heating_capacity == 0 )
	{
		// TODO
	}

	solver = msolve("new");
	msolve("set",solver,"debug",(int)(solver_enable_debug?1:0));
	msolve("set",solver,"dump",(int)(solver_enable_dump?1:0));
	msolve("set",solver,"verbose",(int)(solver_enable_verbose?1:0));

	msolve("set",solver,"N",4);
	
	// building design parameters
	double N = building_floors;
	double M = building_units / N;
	double X = unit_width;
	double Y = unit_depth;
	double Z = building_floor_height;
	double W = core_width;
	double A = X*Y;
	double Aw = unit_window_area>0 ? unit_window_area : (0.5*X*Z);
	double Ad = unit_door_area>0 ? unit_door_area : 20.0;
	double Ao = X*Z - Aw;
	double Ac = X*Z - Ad;
	double Au = Y*Z;
	double V = X*Y*Z;
	if ( building_occupancy_factor <= 0.0 || building_occupancy_factor > 1.0 )
	{
		exception("building_occupancy_factor must be in the range positive and less than 1.0 (building_occupancy_factor = %g)", building_occupancy_factor);
	}
	double beta = building_occupancy_factor;
	double Kx = (core_configuration&CC_INDOOR) ? 0.0 : 1.0;
	double Kd = (core_configuration&CC_DOUBLE) ? 1.0 : 0.0;
	double F = building_floor_depth;

	// building thermal properties
	double rhoair = 0.0735;	// density of air [lb/cf]
	double Sair = 0.2402*rhoair; // volumetric heat capacity of air @ 80F [BTU/F/cf]
	double Sint = 10.0; // heat capacity furnishing per unit floor area (Btu/F/sf)
	double Cint = Sint*A;
	double Sfloor = 150.0; // density of concrete floor (lb/cf)

	// building thermal parameters
	double U_OA = N*M*beta*(Kx+1)*(Ao/Rext+Aw/Rwindow);
	double U_OU = U_OA*(1-beta)/beta;
	double U_OC = N*(1-Kx)*(2*W*Z+M*(1-Kd)*X)*(Ao/Rext+Aw/Rwindow);
	double U_OM = N*F*(M*X+2*(Kd+1)*Y+2*W)/Rext;
	double U_AU = N*M*beta*(1-beta)*Au/Rint;
	double U_AC = N*M*beta*(Ac/((1-Kx)*Rint+Kx*Rext) + Ad/Rdoor);
	double U_AM = N*M*beta*A/Rmass;
	double U_UC = U_AC*(1-beta)/beta;
	double U_UM = U_AM*(1-beta)/beta;
	double U_CM = N*M*W*(Kd+1)/Rmass;
	// TODO: add floor and roof UA values	
	msolve("copy",solver,"U",
		U_OA, U_OU, U_OC, U_OM,
		U_AU, U_AC, U_AM,
		U_UC, U_UM,
		U_CM);

	// zone capacitance
	double C_A = N*M*beta*V*Sair;
	double C_U = C_A*(1-beta)/beta;
	double C_C = N*M*X*W*Z*Sair;
	double C_M = N*M*(Sfloor*F*(1+X*W)+Cint);
	msolve("copy",solver,"C",C_A,C_U,C_C,C_M);

	msolve("copy",solver,"umin",
		-(1+building_overdesign_factor) * unit_cooling_capacity * building_units * building_occupancy_factor,
		-(1+building_overdesign_factor) * unit_cooling_capacity * building_units * (1-building_occupancy_factor),
		-(1+building_overdesign_factor) * ( system_cooling_capacity + unit_cooling_capacity * building_units ),
		0.0);

	msolve("copy",solver,"umax",
		(1+building_overdesign_factor) * unit_heating_capacity * building_units * building_occupancy_factor,
		(1+building_overdesign_factor) * unit_heating_capacity * building_units * (1-building_occupancy_factor),
		(1+building_overdesign_factor) * ( system_heating_capacity - unit_heating_capacity * building_units),
		0.0);

	// set the zone-zone leakages
	msolve("set",solver,"a",1-building_heat_leakage);

	try
	{
		update();
		msolve("solve",solver,0);
	}
	catch (const char *errmsg)
	{
		error("%s",errmsg);
#define DUMP(T,X) gl_output("%18.18s = %-12g # " T,#X,X);
		gl_output("apartment '%s' model dump follows:",get_name());
		DUMP("number of floors",N)
		DUMP("number of units per floor",M)
		DUMP("unit width (ft)",X)
		DUMP("unit depth (ft)",Y)
		DUMP("unit height (ft)",Z)
		DUMP("core width (ft)",W)
		DUMP("unit floor area (sf)",A)
		DUMP("unit window area (sf)",Aw)
		DUMP("unit door area (sf)",Ad)
		DUMP("unit wall area (sf)",Au)
		DUMP("unit exterior wall area (sf)",Ao)
		DUMP("unit core wall area (sf)",Ac)
		DUMP("unit volume (cf)",V)
		DUMP("occupancy factor (pu)",beta)
		DUMP("indoor core fraction (pu)",Kx)
		DUMP("indoor core double loading fraction (pu)",Kd)
		DUMP("floor thickness (ft)",F)
		DUMP("volumetric air density (lb/cf)",rhoair)
		DUMP("air specific heat capacity (Btu/degF/cf)",Sair)
		DUMP("interior mass loading (Btu/degF/sf)",Sint)
		DUMP("interior heat capacity (Btu/degF)",Cint)
		DUMP("density of floor mass (lb/cf)",Sfloor)

		DUMP("Exterior wall R-value",Rext);
		DUMP("Window R-value",Rwindow);
		DUMP("Interior wall R-value",Rint);
		DUMP("Door R-value",Rdoor);
		DUMP("Mass R-value",Rmass);
		DUMP("Roof R-value",Rroof);
		DUMP("Ground R-value",Rground);

		DUMP("UA outdoor occupied (Btu/degF/h)",U_OA)
		DUMP("UA outdoor unoccupied (Btu/degF/h)",U_OU)
		DUMP("UA outdoor core (Btu/degF/h)",U_OC)
		DUMP("UA outdoor mass (Btu/degF/h)",U_OM)
		DUMP("UA occupied unoccupied (Btu/degF/h)",U_AU)
		DUMP("UA occupied core (Btu/degF/h)",U_UC)
		DUMP("UA occupied mass (Btu/degF/h)",U_AM)
		DUMP("UA unoccupied core (Btu/degF/h)",U_UC)
		DUMP("UA unoccupied mass (Btu/degF/h)",U_UM)
		DUMP("UA core mass (Btu/degF/h)",U_CM)
		
		DUMP("occupied zone heat capacity (Btu/degF)",C_A)
		DUMP("unoccupied zone heat capacity (Btu/degF)",C_U)
		DUMP("core zone heat capacity (Btu/degF)",C_C)
		DUMP("mass heat capacity (Btu/degF)",C_M)
		
		DUMP("occupied zone solar gain",Q_AS);
		DUMP("occupied zone ventilation gain",Q_AV);
		DUMP("occupied zone equipment gain",Q_AE);
		DUMP("unoccupied zone solar gain",Q_US);
		DUMP("core zone solar gain",Q_CS);
		DUMP("core zone ventilation gain",Q_CV);
		
		DUMP("unit air temperature",unit_temperature);
		DUMP("vacant air temperature",vacant_temperature);
		DUMP("core air temperature",core_temperature);
		DUMP("mass temperature",building_mass_temperature);
		DUMP("occupied zone mode",unit_mode);
		DUMP("unoccupied zone mode",vacant_mode);
		DUMP("core zone mode",core_mode);
#undef DUMP
		return 0;		
	}

	return 1;
}

void apartment::update()
{
	msolve("set",solver,"debug",solver_enable_debug);
	msolve("set",solver,"dump",solver_enable_dump);
	msolve("set",solver,"verbose",solver_enable_verbose);

	if ( ! building_outdoor_temperature->is_valid() )
	{
		OBJECT *obj = building_outdoor_temperature->get_object();
		PROPERTY *prop = building_outdoor_temperature->get_property();
		exception("building_outdoor_temperature <object:%p> <property:%p> is not valid",obj,prop);
	}
	else
	{
		debug("Tout = %g", building_outdoor_temperature->get_double());
	}

	// set the heat gain
	msolve("copy",solver,"q",
		building_outdoor_temperature->get_double("degF"),
		Q_AS + Q_AV + Q_AE,
		Q_US,
		Q_CS + Q_CV);

	// set the temperatures
	msolve("copy",solver,"T",
		unit_temperature,
		vacant_temperature,
		core_temperature,
		building_mass_temperature);

	// determine the system modes
	update_modes();

	// update occupied unit setpoints
	if ( unit_mode == SPM_COOLING )
	{
		msolve("set",solver,"Tset",0,unit_cooling_setpoint);
	}
	else if ( unit_mode == SPM_HEATING )
	{
		msolve("set",solver,"Tset",0,unit_heating_setpoint);
	}
	else
	{
		// setpoint does not change
	}

	// update occupied unit setpoints
	if ( vacant_mode == SPM_COOLING )
	{
		msolve("set",solver,"Tset",1,vacant_cooling_setpoint);
	}
	else if ( vacant_mode == SPM_HEATING )
	{
		msolve("set",solver,"Tset",1,vacant_heating_setpoint);
	}
	else
	{
		// setpoint does not change
	}

	// update occupied unit setpoints
	if ( core_mode == SPM_COOLING )
	{
		msolve("set",solver,"Tset",2,core_cooling_setpoint);
	}
	else if ( core_mode == SPM_HEATING )
	{
		msolve("set",solver,"Tset",2,core_heating_setpoint);
	}
	else
	{
		// setpoint does not change
	}
}

void apartment::update_modes(void)
{
	double mode[4];
	msolve("get",solver,"mode",&mode);

	// core mode
	if ( system_type_central&STC_HEAT && mode[2] > 0 )
	{	
		core_mode = SPM_HEATING;
	}
	else if ( system_type_central&STC_COOL && mode[2] > 0 )
	{
		core_mode = SPM_COOLING;
	}
	else if ( system_type_ventilation&STV_CENTRAL )
	{
		core_mode = SPM_VENTILATING;
	}

	// TODO: unit/vacant mode
#warning "unit/vacant mode not implemented"
	unit_mode = mode[0]<0 ? SPM_COOLING : (mode[0]>0 ? SPM_HEATING : SPM_OFF);
	vacant_mode = mode[1]<0 ? SPM_COOLING : (mode[1]>0 ? SPM_HEATING : SPM_OFF);
}

TIMESTAMP apartment::precommit(TIMESTAMP t1)
{
	double dt = t1 - my()->clock;
	debug("current time is %d",my()->clock);
	debug("advancing %g seconds to %d",dt,t1);

	// solve for the state at time t1
	msolve("solve",solver,dt);
	double *dT;
	msolve("get",solver,"dT",&dT);
	double dTmax = abs(dT[0]);
	for ( int n = 1; n < 4; n++ )
	{
		double x = abs(dT[n]);
		if ( abs(x) > dTmax )
		{
			dTmax = x;
		}
	}
	debug("dTmax = %g",dTmax);

	// calculate the power 

	// calculate the time to the next required solution

	// return min(a,b);
	debug("elasped time is %g",dt);
	TIMESTAMP a = ((TIMESTAMP)(t1/maximum_timestep+1))*maximum_timestep; 
	debug("maximum update to %d",a);
	if ( maximum_timestep > 0 )
	{ 
		TIMESTAMP b = (dt==0 || dTmax == 0) ? TS_NEVER : (TIMESTAMP)(t1+3600*maximum_temperature_update/dTmax);
		if ( b <= t1 ) b = t1+1;
		debug("model update %d",b);
		if ( b < a ) a = b;
	}
	debug("next update in %d sec",a-t1);
	return a;
}

TIMESTAMP apartment::presync(TIMESTAMP t1)
{
	complex power0 = 0.0;
	if ( power[0] ) power[0]->setp(power0);
	if ( power[1] ) power[1]->setp(power0);
	if ( power[2] ) power[2]->setp(power0);

	return TS_NEVER;
}

TIMESTAMP apartment::sync(TIMESTAMP t1)
{
	// TODO convert plant work to power load
	power_core = 0.0;
	power_parking = 0.0;
	power_system = gl_random_uniform(&(my()->rng_state),0,1); // Q_AE + Q_CE + E/mu * u.abs();
	power_units = 0.0;
	power_total = power_core + power_parking + power_system + power_units;

	complex power3 = power_total/3.0;
	if ( power[0] ) 
	{
		complex power0 = power[0]->get_complex()+power3;
		power[0]->setp(power0);
	}
	if ( power[1] ) 
	{
		complex power1 = power[1]->get_complex()+power3;
		power[1]->setp(power1);
	}
	if ( power[2] ) 
	{
		complex power2 = power[2]->get_complex()+power3;
		power[2]->setp(power2);
	}

	return TS_NEVER;
}

TIMESTAMP apartment::postsync(TIMESTAMP t1)
{
	return TS_NEVER;
}
