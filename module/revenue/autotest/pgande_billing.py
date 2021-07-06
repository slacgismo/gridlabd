from datetime import *
from csv import *
from dateutil import parser
import gridlabd 

csvfile = open("billing.csv","w")
csvwriter = writer(csvfile);
csvwriter.writerow(["datetime","meter","tariff","billing_days","energy","demand","charges"])

def to_float(x):
	return float(x.split(' ')[0])   

def to_datetime(x,format):
	return parser.parse(x)

def compute_bill(gridlabd,**kwargs):

	global csvwriter
	verbose = gridlabd.get_global("verbose")=="TRUE"
	global csvfile

	# get data
	classname = kwargs['classname']
	id = kwargs['id']
	data = kwargs['data']
	bill_name = f"{classname}:{id}"
	bill = gridlabd.get_object(bill_name)
	bill_name = bill["name"]
	baseline = to_float(bill["baseline_demand"])
	tariff = gridlabd.get_object(bill["tariff"])
	meter = gridlabd.get_object(bill["meter"])
	energy = to_float(meter["measured_real_energy"])/1000  # units in kW

	# get duration
	clock = to_datetime(gridlabd.get_global('clock'),'%Y-%m-%d %H:%M:%S %Z')

	if not "lastreading" in data.keys():
		duration = timedelta(0)
	else:
		duration = clock - data["lastreading"]
	data["lastreading"] = clock
	billing_days = (duration.total_seconds()/86400) # seconds in a day 

	# compute energy usage
	if not "lastenergy" in data.keys():
		usage = 0.0
	else:
		usage = energy - data["lastenergy"]
	data["lastenergy"] = energy

	# calculate bill
	tariff_name = tariff["name"]
	meter_name = meter["name"]
	if verbose:
		gridlabd.output(f"Bill '{bill_name}' for meter '{meter_name}' on tariff '{tariff_name}' at time '{clock}':")
		gridlabd.output(f"  Billing days..... %5.0f    days" % (billing_days))
		gridlabd.output(f"  Meter reading.... %7.1f  kWh" % (energy))
	if baseline == 0.0:
		if verbose:
			gridlabd.output(f"  Energy usage..... %7.1f  kWh" % (usage))
		charges = usage * to_float(tariff["energy_charge_base"])
	else:
		tier1 = min(usage,baseline*billing_days)
		tier2 = min(usage-tier1,baseline*billing_days*4)
		tier3 = usage-tier1-tier2
		if verbose:
			gridlabd.output(f"  Tier 1 usage..... %7.1f  kWh" % (tier1))
			if tier2 > 0:
				gridlabd.output(f"  Tier 2 usage..... %7.1f  kWh" % (tier2))
			if tier3 > 0:
				gridlabd.output(f"  Tier 3 usage..... %7.1f  kWh" % (tier3))
		charges = tier1 * to_float(tariff["energy_charge_base"]) + tier2 *	to_float(tariff["energy_charge_100"]) + tier3 * to_float(tariff["energy_charge_400"])	

	# apply discount, if any
	discount = to_float(tariff["discount"])
	if discount > 0:
		charges -= usage * discount;

	# apply daily minimum
	minimum = to_float(tariff["minimum_daily_charge"])
	if charges < minimum * billing_days:
		charges = minimum * billing_days
	if verbose:
		gridlabd.output(f"  Energy charges... %8.2f US$" % (charges))

	# output billing record only if charges are non-zero
	if charges > 0:
		csvwriter.writerow([clock,meter_name,tariff_name,int(billing_days),round(usage,1),0,round(charges,2)])
		csvfile.flush()

	# update billing data
	gridlabd.set_value(bill_name,"total_bill",str(to_float(bill["total_bill"])+charges))
	gridlabd.set_value(bill_name,"billing_days",str(billing_days))
	gridlabd.set_value(bill_name,"energy_charges",str(to_float(bill["energy_charges"])+charges))
	gridlabd.set_value(bill_name,"total_charges",str(to_float(bill["total_charges"])+charges))
