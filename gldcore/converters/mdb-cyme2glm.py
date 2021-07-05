"""CYME 'mdb' to GridLAB-D 'glm' converter using OpenFIDO

"""

import sys, os
import openfido

def convert(input_file,output_file=None,options={}):
	"""Convert a CYME MDB file to GLM using github/openfido/cyme-extract

	Parameters:
		input_file (str)	input MDB file name
		output_file (str)	output GLM file name
		options (dict)		options to change config and model options
	"""

	for name, value in options.items():
		if name in openfido.config.keys():
			openfido.config[name] = value
		elif name in openfido.params.keys():
			openfido.params[name] = value
		else:
			openfido.error(f"'{name}' is not a valid mdb-cyme2glm converter parameter")
	openfido.debug(f"input_file = {input_file}")
	openfido.debug(f"output_file = {output_file}")
	openfido.debug(f"config = {config}")
	openfido.debug(f"params = {params}")

	input_path = os.path.realpath(input_file)
	input_folder = os.path.dirname(input_path)
	input_name = os.path.basename(input_path)
	openfido.debug(f"input_folder = {input_folder}")
	openfido.debug(f"input_name = {input_name}")

	output_path = os.path.realpath(output_file)
	output_folder = os.path.dirname(output_path)
	output_name = os.path.basename(output_path)
	openfido.debug(f"output_folder = {output_folder}")
	openfido.debug(f"output_name = {output_name}")

	openfido.run("cyme-extract",
		input_folder = input_folder,
		output_folder = output_folder,
		extract = "non-empty",
		files = input_name,
		glmname = output_name,
		outputs = "glm",
		postproc = "write_glm",
		)