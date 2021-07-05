[[/Module/Residential/Apartment]] - Multi-family residential building

# Synopsis

GLM:

~~~
module residential {
	overdesign_factor "<ratio> pu";
}
object apartment 
{
	parent "<meter-name>";
	storeys "<floors>";
	circulation_is_indoor {TRUE,FALSE};
	circulation_elevator_count "<count>";
	central_plant_capacity "<capacity> Btu/h";
	central_plant_efficiency "<cop> Btu/kWh";
	central_plant_type {NONE,STEAM,HOTWATER,FOURPIPE,FORCEDAIR};
	central_plant_has_cooling {TRUE,FALSE};
	central_washing_count "<washer-dryer-count>";
	parking_size "<total-parking>";
	parking_is_indoor {TRUE,FALSE};
	parking_charger_count "<total-chargers>";
	parking_charger_capacity "<charger-power> kVA";
	parking_charger_active "<active-chargers>";
	unit_count "<unit-count>";
	unit_size "<floor-area> sf";
	unit_occupancy_factor "<ratio> pu";
	unit_temperature "<temperature> degF";
	unit_perimeter_conductance "<conductance> Btu/degF/h";
	unit_thermal_mass "<thermal-mass> Btu/degF";
	unit_mass_conductance "<conductance> Btu/degF/h";
	unit_cooling_type {NONE,CENTRAL,WINDOW_AC,UNITARY_AC,HEATPUMP};
	unit_cooling_setpoint "<temperature> degF";
	unit_heating_type {NONE,CENTRAL,BASEBOARD,RESISTIVE,HEATPUMP};
	unit_heating_setpoint "<temperature> degF";
	unit_appliance_types {NONE|DISHWASHER|REFRIGERATOR|WASHER|DRYER};
}
~~~

# Description

The `apartment` model describes a multi-family residence that can include interior circulation, a central plant, parking with electric vehicle chargers, and unit heating/cooling and appliances.

## Parameters

### `central_plant_capacity`

~~~ 
double central_plant_capacity[Btu/h];
~~~

This parameter describes the total heating/cooling capacity of the central plant, if any.  This parameter is ignored if `central_plant_has_cooling` is `NONE`. The default value is calculated to exceed the peak demand by `overdesign_factor`.

If the value specified does not meet the peak heating or cooling load, a warning is generated that indicates what the peak demand is.

### `central_plant_efficiency`

~~~
double central_plant_efficiency[Btu/kWh];
~~~

This parameter describes the total heating/cooling efficiency of the central plant, if any.  This parameter is ignored if `central_plant_has_cooling` is `NONE`.

### `central_plant_has_cooling`

~~~
boolean central_plant_has_cooling;
~~~

This parameter determines whether the units are provided cooling from a central plant. The parameter is ignored when `central_plant_type` is not `FORCEDAIR`.

### `central_plant_type`

~~~
set {NONE,STEAM,HOTWATER,FOURPIPE,FORCEDAIR} central_plant_type;
~~~

This parameter determines the type of central in use, if any.  If the central plant is `FORCEDAIR` or `FOURPIPE`, then cooling is available. 

### `central_washing_count`

~~~
integer central_washing_count;
~~~

This parameter determines how many centrally located (common) washing and drying appliances are located in the building.

### `circulation_elevator_count`

~~~
integer circulation_elevator_count;
~~~

This parameter determines the number of elevators available in the building.  The default is `0`.

### `circulation_is_indoor`

~~~
boolean circulation_is_indoor;
~~~

This parameter specifies whether circulation is indoors.  Setting this to `TRUE` affects the thermal model in two significant ways: (1) the conditioned floor area is increased when a central plant is present, and (2) the `unit_perimeter_conductance` is decreased.

### `parent`

~~~
object parent;
~~~

The parent object must be a three-phase meter object (see [[/Module/Powerflow/Meter.md]]). If the parent object is not defined, a static voltage source is used.

### `parking_charger_active`

~~~
integer parking_charger_active;
~~~

This parameter determines how many parking chargers are active at any moment in time.

### `parking_charger_capacity`

~~~
complex parking_charger_capacity[kVA];
~~~

This parameters determines the charging capacity of each charger.

### `parking_charger_count`

~~~
integer parking_charger_count;
~~~

This parameter determines the total number of vehicle chargers installed.

### `parking_is_indoor`

~~~
boolean parking_is_indoor;
~~~

This parameter determines whether the parking garage is considered "indoors", e.g., covered by the principal building structure, and therefore includes lighting and ventilation loads. 

### `parking_size`

~~~
integer parking_size;
~~~

This parameter determines the number of parking spots installed.

### `storeys`

~~~
integer storeys;
~~~

This parameter specifies the number of floors. This determines the exterior wall area to interior volume/mass ratios used for the thermal model. 

### `unit_appliance_types`

~~~
enumeration {NONE,DISHWASHER,REFRIGERATOR,WASHER,DRYER,ALL};
~~~

This parameter determines which appliances are included in the apartment units. 

**Caveat**: that the meaning of `ALL` changes from one version to another if the list of available appliances differs.

### `unit_cooling_type`

~~~
set {NONE,CENTRAL,WINDOW_AC,UNITARY_AC,HEATPUMP} unit_cooling_type;
~~~

This parameter determines what type of cooling is available in apartment units.

### `unit_count`

~~~
integer unit_count;
~~~

This parameter determines how many apartment units are in the building.

### `unit_heating_type`

~~~
set {NONE,CENTRAL,BASEBOARD,RESISTIVE,HEATPUMP} unit_heating_type;
~~~

This parameter determines what type of heating is available in apartment units.

### `unit_mass_conductance`

~~~
double unit_mass_conductance [Btu/degF/h];
~~~

This parameter determines the thermal conductance between unit mass and unit air.

### `unit_cooling_setpoint`

~~~
double unit_cooling_setpoint[degF];
~~~

This parameter indicates the mean cooling setpoint for the occupied units.

### `unit_heating_setpoint`

~~~
double unit_heating_setpoint[degF];
~~~

This parameter indicates the mean heating setpoint for the occupied units.

### `unit_temperature`

~~~
double unit_temperature[degF];
~~~

This parameter indicates the mean temperature of the occupied units.

### `unit_occupancy_factor`

~~~
double unit_occupancy_factor[pu];
~~~

This parameter determines the total unit occupancy factor. Unoccupied units do not contribute to the electric load, but their thermal response to conditions is modeled.

### `unit_perimeter_conductance`

~~~
double unit_perimeter_conductance [Btu/degF/h];
~~~

This parameter determines the thermal conductance between the interior air of the unit and exterior air temperature.

### `unit_size`

~~~
double unit_size[sf];
~~~

This parameter specified the mean size of units in the building.

### `unit_thermal_mass`

~~~
double unit_thermal_mass[Btu/degF];
~~~

This parameter specifies the mean thermal mass of units in the building.

## Model

There are four principal models in an `apartment` object: the thermal model, the equiment model, the appliance model, and the electrical model.

### Thermal Model

The thermal model uses an aggregate two-node thermal model for the apartment units. This assumes that all units have nearly the same temperature and setpoints. The duty cycle of heating/cooling system is determined by the fraction of the total system capacity required to meet the heating/cooling load at the current outdoor air temperature, given the current indoor air temperature. This duty cycle determines the total heating/cooling energy delivered to the units.

### Equipment Model

The default central plant capacity is computed based on the greater of the peak whole building heating and cooling load, multiplied by the `overdesign_factor`, less the individual unit heating/cooling load served by unit heating/cooling systems.  The size of the individual unit equipment is computed based on the greater of the peak unit heating and cooling load, multiplied by the `overdesign_factor`.

### Appliance Model

Unit appliance models are based on aggregate end-use loadshapes available in the `residential` module.

### Electrical Model

The total electric load is the sum of the central plant, parking, circulation, and individual unit loads.

# See also

* [[/Module/Powerflow/Meter.md]]