#!python
'''
Ylands Nest Scene (JSON tool)
Takes flat/raw exported Ylands scene JSON and nests child groups/entities based on parent.
Author: BinarySemaphore
Updated: 2025-02-20
'''
__version__ = '0.1.1'

import sys
import json

def main(source_filename, output_filename="out.json"):
    data = None
    with open(source_filename, 'r') as f:
        data = json.load(f)
    
    new_data = {}
    keys = list(data.keys())
    for key in keys:
        item = data[key]
        if item.get('parent'):
            parent_key = item['parent']
            parent = data.get(parent_key)
            if not parent or not isinstance(parent, dict):
                raise Exception("Cannot find parent \"%s\" in source data" % parent_key)
            if not parent.get('children'):
                parent['children'] = {}
            parent['children'][key] = item.copy()
            del parent['children'][key]['parent']
        else:
            new_data[key] = item.copy()
    
    with open(output_filename, 'w') as f:
        json.dump(new_data, f)

def argParse():
    config = {
        'output_filename': 'out.json'
    }
    if '-h' in sys.argv or '--help' in sys.argv:
        printHelp()
        exit(0)
    
    if '-o' in sys.argv:
        config['output_filename'] = str(sys.argv[sys.argv.index('-o')+1])
    
    if len(sys.argv) < 2:
        print('Too few arguments.')
        printHelp()
        exit(1)
    
    config['source_filename'] = sys.argv[-1]
    
    return config

def printHelp():
    print(__doc__)
    print("Version: %s\n" % __version__)
    print("Usage:")
    print("  python ylands_nest_scene.py [options] <JSON-file>")
    print("  ./ylands_nest_scene.py [options] <JSON-file>")
    print("\nArguments:")
    print("  <JSON-file> : Exported Ylands scene JSON file.")
    print("\nOptions:")
    print("         -h --help : Display this information/help.")
    print("  -o <output-file> : Specify output filename (default: out.json).")
    print("\nExamples:")
    print("1) ./ylands_nest_scene.py scene_house.json")
    print("    Nests all children in \"scene_house.json\".")
    print("    Saves to default output file of \"./out.json\".")
    print("2) python ylands_nest_scene.py -o myhouse.json scene_house.json")
    print("    Nests all children in \"scene_house.json\".")
    print("    Saves to specified output file of \"./myhouse.json\".")

if __name__ == "__main__":
    config = argParse()
    main(config['source_filename'], config['output_filename'])
