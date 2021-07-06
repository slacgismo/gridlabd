[[/Use_cases/Integration_capacity_analysis]] -- Integration capacity analysis


# Background
In California, Investor Owned Utilities (IOUs) are required to complete system-wide integration capacity analysis (ICA) to determine the maximum node level hosting capacity for a circuit to remain within key power system criteria. System ICA results are used to expedite interconnection permitting for distributed energy resource (DER) additions under the Rule 21 interconnection process (Interconnection Use Case). Results must be updated monthly and data must be mapped for review, with hourly capacity limitation data available for download by users. ICA with DER growth scenarios are also required for all annual IOU distribution system planning processes (Planning Use Case). The CPUC provided guidance to IOUs in late 2017 to complete ICA system wide in response to completed IOU demonstration projects and the DER WG final report. 

# Introduction

The ICA quantifies the potential DER generation which can interconnect without violating thermal, power quality/voltage, protection, and safety/reliability criteria throughout the distribution system. The DER interconnection use case is required to be completed using iterative power flow modeling system-wide. **The iterative methodology requires varying DER generation level at each node, independently, until a system criteria violation occurs somewhere within a feeder.** The ICA value for that node is then the minimum power injection associated with any violation criteria. All three phase nodes and line sections in radial distribution feeders are to be evaluated.  ICA must be conducted assuming "technology agnostic" hourly generation profiles with multiple configuration scenarios. Utilities have successfully implemented technology agnostic ICA with 576 hourly profile and accounting for operation of voltage regulators and pre-existing conditions.


**ICA System Criteria**

- Thermal Loading:  maximum load or generation which can be connected to a node without exceeding the thermal value of any equipment in the feeder. 
  - Iterative power flows  to determine the maximum value which can be connected at each node before a thermal overload occurs. 
 
- Steady State Voltage (SSV): maximum load or generation which can be connected to a node without moving the voltage at any part of feeder outside the range of +/-5% of nominal (For 120V, the range would be 114V to 126V).
  - Iterative power flows to determine the maximum value which can be connected at each node before the voltage deviates from the range 

- Protection systems:  maximum load or generation which can be connected to a node without reducing the protection device’s ability to detect faults (reduction of reach).
  - Perform a fault flow analysis to evaluate the reduction in fault current flow at protection sensing devices due to DER interconnected downstream from the protection sensing device. Protection limit is when to when DER reduces fault at sensing relay to less than 2.3*Minimum Trip Setting

- Voltage Fluctuation & Power Quality:  maximum load or generation which can be connected to a node without changing the voltage by more than 3%.
  - Simulate the DER turning on and turning off and compare the voltages before and after to determine change in voltage and % change. 

- Operational Flexibility: maximum load or generation which can be connected to a node without exceeding loading beyond an automated SCADA switching devices.  
  - Determine the load connected beyond the automated SCADA switching device, the ICA value considering operational flexibility is limited to the amount of load beyond the SCADA switching device.  
  - Both operational flexibility values must be evaluated:
  1. ICA values not accounting for Operational Flexibility-reverse flow up to substation low-side
  2. ICA values accounting for Operational Flexibility-no reverse flow up to substation low-side

# Methodology 
- Define feeder network 
- Identify all three phase nodes and relevant line sections in radial distribution feeders (circuits) 
- Iterate analysis over every node with interconnection potential
- Vary generation or load level at each node until violations occur in an ICA category


| Category    | Violation Limit  |
| ------------- |:-------------:|
| Steady State Voltage      | +/- 5% nominal |
| Voltage Fluctuation    | <3% △    |
| Thermal Loading  |   <Equipment Rating    |
| Protection  |   <2.3 Trip Setting  |
|Operational Flexibility  |   <loading at SCADA switching devices  |


# Data Inputs 
- Feeder network data including all equipment, settings, and configuration information
- Electrical characteristics such as thermal ratings for all 3-phase conductors in the feeder, resistance of each 3-phase line section, line-neutral voltages at each node, current, and fault duties. 
- 576 hourly profiles (monthly min and max day) based on localized load shapes
- Sequence of power injection or load quantities at node

# Configurations
- Node reduction, minimizing the number of nodes analyzed, and limitation category reduction for strong feeders are acceptable methods for computational efficiency
- Voltage regulating devices must be allowed to float in power flow simulations
- Pre-existing conditions should only be included when adding DER degrades the pre-existing condition, but ICA values should not reflect this limit if adding DER improves pre-existing condition
- ICA values should be assessed for the following scenarios for each node considered:
  - Uniform Generation
  - Uniform Load
  - Fixed PV 
  - Operational Flexibility Scenarios: ICA calculated with and without reverse flow constraint at SCADA devices 
 

# Results
The ICA value must be published associated with the limiting system criteria, along with: 
- Circuit ID;
- Circuit Load Profile;
- Section ID;
- Voltage (kV);
- Substation ID;
- Substation Load Profile System; 
- Customer class proportions on circuit;
- Existing generation (MW);
- Queued generation (MW); 
- Total generation (MW); 
- Hosting capacity for uniform generation (MW); 
- Hosting capacity for uniform load (MW); 
- Hosting capacity for generic PV system (MW). 
