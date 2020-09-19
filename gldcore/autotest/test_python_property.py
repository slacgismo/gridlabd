import gridlabd

test = {
	"shortint" : 21,
	"mediumint" : 31,
	"longint" : 41,
	"real" : 4.321,
	"realimag" : (4.321+5.432j),
	"shortstr" : "another short string",
	"mediumstr" : "another medium string",
	"longstr" : "another long string",
	"speed" : ["100 m/s", 100],
	# "random" : {'type':'normal','a':10, 'b':1,'low':9,'high':11,'update_rate':30, 'state':3376415067},
	"random" : {'type':'normal(10,1)','min':9,'max':11,'refresh':30, 'state':3376415067},
}

convert = {
	"random" : (lambda r: f"type:{r['type']}; min:{r['min']}; max:{r['max']}; refresh:{r['refresh']}; state:{r['state']}")
	}

def on_term(t):
	for obj in gridlabd.get("objects"):
		prop = gridlabd.property(obj,"py_object")
		if oclass == "test":
			prop = gridlabd.property(obj,"py_object")
		elif oclass == "check":
			prop = gridlabd.property(obj,obj)
		else:
			continue
		prop.rlock()
		value = prop.get_value()
		initv = prop.get_initial()
		text = str(prop)
		info = repr(prop)
		unit = prop.get_unit()
		prop.unlock()
		if obj in test.keys():
			value = test[obj]
			prop.wlock()
			if type(value) is dict:
				goal = value
				check = value
				init = True
			elif type(test[obj]) is list:
				goal = value[0]
				check = value[1]
				init = False
			else:
				goal = value
				check = value
				init = False
			prop.set_value(goal)
			if init:
				result = prop.get_initial()
			else:
				result = prop.get_value()
			if obj in convert.keys():
				check = convert[obj](check)
			if result != check:
				raise Exception(f"set failed ('{result}' != '{check}')")
			prop.unlock()
			