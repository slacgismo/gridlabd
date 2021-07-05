RBSA II Database Updates
__________________________________________________________
This document details updates and changes to the RBSA II Database
Changes and updates are numbered and organized by date



Updates 4/2/2018
__________________________________________________________

[1] Mechanical_DuctingAll.csv - changed two column names:

 - Percentage of Supply Ducts in Unconditioned and Inaccessible Space ---> Percent Supply Ducts in Unconditioned Inaccessible Space
 - Percentage of Supply Ducts in Unconditioned and Inaccessible Space_notes ---> Percent Supply Ducts in Unconditioned Inaccessible Space_notes

[2] DataDictionary.csv expanded to include three new columns: 
 
 - DataType: Describes the SQL data type of a given field
 - MaxLength: The maximum number of characters in a given field
 - MSAccessDataType: Describes the MS Access data type of a given field

[3] Database user manual updated with new Access Import Script and Import Instructions



Updates 4/27/2018
__________________________________________________________

[4] Updates to the SiteDetail.csv file:

 - Added a column: 'Vintage'
 - Added a column: 'Vintage_Notes'

[5] Data collection protocol updated to expand on the procedure for measuring faucet and shower flow rate

[6] A handful of sites had been attributed to an incorrect strata. They have been moved to the correct strata, and case weights have been updated to reflect the change

[7] Updates to the SiteOneLine.csv file:

 - Added a column: 'Site Case Weight_Notes'
 - Converted the 'Site Case Weight' column to numeric
 - Added a column: 'Primary Heating Fuel'

[8] Updates to the BuildingOneLine.csv file:

 - Added a column: 'Site Case Weight_Notes'
 - Converted the 'Site Case Weight' column to numeric

[9] DataDictionary.csv updated to reflect changes in SiteOneLine.csv, SiteDetail.csv, and BuildingOneLine.csv files

[10] Database user manual updated with new MS Access Import Scripts



Updates 5/17/2018
__________________________________________________________

[11] Corrected annual gas consumption for a handful of multifamily sites



Updates 3/29/2019
__________________________________________________________

[12] Added or removed columns to accommodate the updates to the UA methodology. Updates to the Envelope_Walls.csv file:

- Added a column: 'Wall Cavity Insulation R-Value'
- Added a column: 'Wall Cavity Insulation R-Value_notes'
- Added a column: 'Wall Continuous Insulation R-Value'
- Added a column: 'Wall Continuous Insulation R-Value_notes'
- Added a column: 'Basement Wall Below Grade U-Value'
- Added a column: 'Basement Wall Below Grade U-Value_notes'
- Added a column: 'Basement Wall Above Grade U-Value'
- Added a column: 'Basement Wall Above Grade U-Value_notes'
- Added a column: 'Wall U-Value'
- Added a column: 'Wall U-Value_notes'

- Removed a column: 'Wall Insulation U-Value'
- Removed a column: 'Wall Insulation U-Value_notes'
- Removed a column: 'Wall Insulation R-Value'
- Removed a column: 'Wall Insulation R-Value_notes'
- Removed a column: 'Basement Walls - Additional U-Value from Soil'
- Removed a column: 'Basement Walls - Additional U-Value from Soil_notes'
- Removed a column: 'Wall U-Value for Heat Loss without Soil'
- Removed a column: 'Wall U-Value for Heat Loss without Soil_notes'
- Removed a column: 'Wall U-Value for Heat Loss including Soil'
- Removed a column: 'Wall U-Value for Heat Loss including Soil_notes'


[13] Added or removed columns to accommodate the updates to the UA methodology. Updates to the Envelope_Ceilings.csv file:

- Added a column: 'Ceiling U-Value'
- Added a column: 'Ceiling U-Value Notes'

- Removed a column: 'Ceiling U-Value for Heat Loss'
- Removed a column: 'Ceiling U-Value for Heat Loss_notes'
- Removed a column: 'Ceiling Insulation U-Value'
- Removed a column: 'Ceiling Insulation U-Value_notes'

 

[14] Added or removed columns to accommodate the updates to the UA methodology. Updates to the Envelope_Floor_Foundation.csv file:

- Added a column: 'Foundation Wall Area'
- Added a column: 'Foundation Wall Area_notes'
- Added a column: 'Framed Floor Insulation R-Value'
- Added a column: 'Framed Floor Insulation R-Value_notes'
- Added a column: 'Crawlspace Wall Below Grade U-Value'
- Added a column: 'Crawlspace Wall Below Grade U-Value_notes'
- Added a column: 'Crawlspace Wall Above Grade U-Value'
- Added a column: 'Crawlspace Wall Above Grade U-Value_notes'
- Added a column: 'Foundation Wall U-Value'
- Added a column: 'Foundation Wall U-Value_notes'
- Added a column: 'Foundation Wall Heat Loss Rate (UA)'
- Added a column: 'Foundation Wall Heat Loss Rate (UA)_notes'
- Added a column: 'Framed Floor U-Value'
- Added a column: 'Framed Floor U-Value_notes'
- Added a column: 'Framed Floor Heat Loss Rate (UA)'
- Added a column: 'Framed Floor Heat Loss Rate (UA)_notes'
- Added a column: 'Ground Contact Heat Loss Rate (UA)'
- Added a column: 'Ground Contact Heat Loss Rate (UA)_notes'
- Added a column: 'Heat Loss Components'

- Renamed a column: 'Floor Insulated' --> 'Framed Floor Insulated'
- Renamed a column: 'Floor Insulation Type 1' --> 'Framed Floor Insulation Type 1'
- Renamed a column: 'Floor Insulation Thickness 1' --> 'Framed Floor Insulation Thickness 1'
- Renamed a column: 'Floor Insulation Thickness 1_notes' --> 'Framed Floor Insulation Thickness 1_notes'
- Renamed a column: 'Floor Insulation Condition 1' --> 'Framed Floor Insulation Condition 1'
- Renamed a column: 'Floor Insulation Condition 1_notes' --> 'Framed Floor Insulation Condition 1_notes'
- Renamed a column: 'Floor Insulation Type 2' --> 'Framed Floor Insulation Type 2'
- Renamed a column: 'Floor Insulation Thickness 2' --> 'Framed Floor Insulation Thickness 2'
- Renamed a column: 'Floor Insulation Thickness 2_notes' --> 'Framed Floor Insulation Thickness 2_notes'
- Renamed a column: 'Floor Insulation Condition 2' --> 'Framed Floor Insulation Condition 2'
- Renamed a column: 'Floor Insulation Condition 2_notes' --> 'Framed Floor Insulation Condition 2_notes'
- Renamed a column: 'Floor Insulation Type 3' --> 'Framed Floor Insulation Type 3'
- Renamed a column: 'Floor Insulation Thickness 3' --> 'Framed Floor Insulation Thickness 3'
- Renamed a column: 'Floor Insulation Thickness 3_notes' --> 'Framed Floor Insulation Thickness 3_notes'
- Renamed a column: 'Floor Insulation Condition 3' --> 'Framed Floor Insulation Condition 3'
- Renamed a column: 'Floor Insulation Condition 3_notes' --> 'Framed Floor Insulation Condition 3_notes'
- Renamed a column: 'Floor Insulation R-Value' --> 'Framed Floor Insulation R-Value'
- Renamed a column: 'Floor Insulation R-Value_notes' --> 'Framed Floor Insulation R-Value_notes'
- Renamed a column: 'Floor Heat Loss Rate (UA)' --> 'Final Floor / Foundation Heat Loss Rate (UA)'
- Renamed a column: 'Floor Heat Loss Rate (UA)_notes' --> 'Final Floor / Foundation Heat Loss Rate (UA)_notes'
- Renamed a column: 'Slab F-Value for Heat Loss' --> 'Foundation F-Value'
- Renamed a column: 'Slab F-Value for Heat Loss_notes' --> 'Foundation F-Value_notes'

- Removed a column: 'Floor Insulation U-Value_notes'
- Removed a column: 'Floor U-Value for Heat Loss'
- Removed a column: 'Floor U-Value for Heat Loss_notes'
- Removed a column: 'Slab F-Value for Heat Loss' --> 'Foundation F-Value'
- Removed a column: 'Slab F-Value for Heat Loss_notes' --> 'Foundation F-Value_notes'



[15] Updates to the Water.csv file:

- Added a column: 'Calibration Factor'
- Added a column: 'Calibration Factor_notes'
- Added a column: 'Calibrated Flow Rate'
- Added a column: 'Calibrated Flow Rate_notes'

[16] Updates to the Mechanical_WaterHeater.csv file:

- Updated water heater technology for two records to correct disagreement between fuel type and water heater technology.

[17] Updates to the SiteOneLine.csv file:

- Data updated to reflect changes to UA methodology

[18] Updates to the BuildingOneLine.csv file:

- Data updated to reflect changes to UA methodology