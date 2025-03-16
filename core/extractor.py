#!python
'''
Ylands Export Extractor
Used with Ylands tools ("EXPORTBLOCKDEFS.ytool" and "EXPORTSCENE.ytool") to
pull JSON data from log file "log_userscript_ct.txt".
Produces clean JSON ready for scene reconstruction.
Configure with accompanying file "config.json".
See README for more details.
Author: BinarySemaphore
Updated: 2025-03-13
'''
__version__ = '0.1.2'

import os
import sys
import json

CONFIG_FILE = "./config.json"
LOG_STRIP = "# "
DATA_INDICATOR_START = "# }"
DATA_INDICATOR_BLOCKDEF = "Export Block Ref (below):"
DATA_INDICATOR_SCENE = "Export Scene (below):"
DATA_TYPE_BLOCKDEF = "BLOCKDEF"
DATA_TYPE_SCENE = "SCENE"

def extract(config, output_filename="out.json"):
    '''
    :param dict config: Required key-values:
                        'ylands_base_dir': "<ylands-install-dir-fullpath>"
                        'ylands_script_log': "<userscript-log-file-relpath>"
                        Optional key-values:
                        'nest_scene': <bool>
                        'pprint': <bool>
                        
    :param str output_filename: JSON filename to save extracted data into
    :returns: exist status
    :rtype: int
    '''
    cf_base_dir = config['ylands_base_dir']
    cf_log = config['ylands_script_log']
    cf_nest = config['nest_scene']
    cf_pprint = config['pprint']
    
    yland_log_lines = []
    yland_log_file = os.path.join(cf_base_dir, cf_log)
    data_start = -1
    data_raw_lines = []
    data_type = None
    
    print("Loading log file \"%s\"..." % yland_log_file)
    with open(yland_log_file, 'r', encoding="ascii", errors="replace") as f:
        yland_log_lines = f.readlines()
    if not yland_log_lines:
        print("No data from log file")
        return 1
    yland_log_lines.reverse()
    print("Loaded\n")
    
    print("Searching for exported data...")
    for index in range(len(yland_log_lines)):
        line = yland_log_lines[index]
        if line.startswith(DATA_INDICATOR_START):
            data_start = index
        if data_start >= 0 and not data_type:
            if DATA_INDICATOR_BLOCKDEF in line:
                data_type = DATA_TYPE_BLOCKDEF
            elif DATA_INDICATOR_SCENE in line:
                data_type = DATA_TYPE_SCENE
        if data_type:
            data_raw_lines = yland_log_lines[data_start:index]
            del yland_log_lines
            break
    if not data_type or not data_raw_lines:
        print("Could not find any export data, please try restarting Ylands and rerunning the export tool")
        return 1
    data_raw_lines.reverse()
    print("Found %s exported data\n" % data_type)
    
    print("Processing %s data to JSON..." % data_type)
    for index in range(len(data_raw_lines)):
        data_raw_lines[index] = data_raw_lines[index].strip(LOG_STRIP)
    data_raw = "".join(data_raw_lines)
    data_raw = data_raw.encode("ascii", errors="replace").decode("ascii")
    data = None
    try:
        data = json.loads(data_raw)
    except json.JSONDecodeError as e:
        print("Failed get JSON data: %s" % e)
        print("Storing raw data in \"error.txt\"")
        with open("error.txt", 'w') as f:
            f.write(data_raw)
        return 2
    del data_raw_lines
    del data_raw
    print("JSON ready\n")
    
    if data_type == "SCENE" and cf_nest:
        print("Reformat %s JSON from flat to nested..." % data_type)
        try:
            data = reformatSceneFlatToNested(data)
        except Exception as e:
            confirm = input("Failed to reformat data, continue without nesting? (y/n): ")
            if confirm != 'y':
                print("Abort and exit")
                return 3
        print("JSON data reformatted\n")
    
    print("Writing JSON to file \"%s\"..." % output_filename)
    while(os.path.exists(output_filename)):
        confirm = input("Output file already exists, overwrite? (y/n): ")
        if confirm == 'y':
            break
        output_filename = input("Rename output file (leave empty to quit): ")
        if not output_filename:
            print("Abort write and exit")
            return 0
    with open(output_filename, 'w') as f:
        if cf_pprint:
            json.dump(data, f, indent=4)
        else:
            json.dump(data, f)
    print("JSON for %s written to file\n" % data_type)
    
    print("Done")
    return 0

def reformatSceneFlatToNested(data):
    reformated_data = {}
    for key in data:
        item = data[key]
        if item.get('parent'):
            parent_key = item['parent']
            parent = data.get(parent_key)
            if not parent or not isinstance(parent, dict):
                raise Exception("Cannot find parent \"%s\" in source data" % parent_key)
            if not parent.get('children'):
                parent['children'] = {}
            parent['children'][key] = item
            del parent['children'][key]['parent']
        else:
            reformated_data[key] = item
    return reformated_data

def configFromFilePromptIfNeeded(config_file):
    print("Loading config file: \"%s\"..." % config_file)
    
    config_data = None
    with open(config_file, 'r') as f:
        config_data = json.load(f)
    
    config = loadAndValidateConfigDataOfferSave(config_file, config_data)
    
    print("Config loaded: %s\n" % json.dumps(config_data, indent=4))
    return config

def loadAndValidateConfigDataOfferSave(config_file, config_data):
    config = {}
    save_config = False
    base_dir = None
    # Map to internal config key and validation method
    config_mapping = {
        "Ylands Install Location": ("ylands_base_dir", os.path.isdir),
        "Log Location": ("ylands_script_log", os.path.exists),
        "Auto Nest Scenes": ("nest_scene", validateIgnore),
        "Output Pretty": ("pprint", validateIgnore)
    }
    
    for data_key in config_mapping:
        config_key, validate_method = config_mapping[data_key]
        config[config_key] = config_data.get(data_key)
        
        validate_value = config[config_key]
        if config_key == "ylands_base_dir":
            base_dir = config[config_key]
        elif config_key == "ylands_script_log" and validate_value:
            validate_value = os.path.join(base_dir, validate_value)
        
        if validate_value is None or not validate_method(validate_value):
            print("\nInvalid Config \"%s\": \"%s\"" % (data_key, config[config_key]))
            config[config_key] = input("Enter new value for \"%s\": " % data_key)
            save_config = True
    
    if save_config:
        confirm = input("Config was updated, save to config file? (y/n): ")
        if confirm == 'y':
            for data_key in config_mapping:
                config_key, _ignore = config_mapping[data_key]
                config_data[data_key] = config[config_key]
            with open(config_file, 'w') as f:
                json.dump(config_data, f, indent=4)
            print("Updated config saved to \"%s\"" % config_file)
        return loadAndValidateConfigDataOfferSave(config_file, config_data)
    else:
        return config

def validateIgnore(_ignore):
    return True

def argParse():
    config = {
        'output_filename': 'out.json'
    }
    
    if '-h' in sys.argv or '--help' in sys.argv:
        printHelp()
        exit(0)
    
    if '-o' in sys.argv:
        config['output_filename'] = str(sys.argv[sys.argv.index('-o')+1])
    
    return config

def printHelp():
    script_file = os.path.basename(__file__)
    print(__doc__)
    print("Version: %s" % __version__)
    print("\nUsage:")
    print("  python %s [options]" % script_file)
    print("  ./%s [options]" % script_file)
    print("\nArguments:")
    print("  NONE : Configuration in \"%s\"" % CONFIG_FILE)
    print("\nOptions:")
    print("         -h --help : Display this information/help.")
    print("  -o <output-file> : Specify output filename (default: out.json).")
    print("\nExamples:")
    print("1) ./%s" % script_file)
    print("2) python %s -o myhouse.json" % script_file)
    print("\nExample \"%s\"" % CONFIG_FILE)
    print('''{
    "Ylands Install Location": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Ylands",
    "Log Location": "Ylands_Data\\log_userscript_ct.txt",
    "Auto Nest Scenes": true,
    "Output Pretty": false
}''')

if __name__ == "__main__":
    config_term = argParse()
    config_data = configFromFilePromptIfNeeded(CONFIG_FILE)
    status = extract(config_data, config_term['output_filename'])
    exit(status)
