import sys
import os

zip_file = sys.argv[1]

set_name = zip_file.replace('.tar.gz','')

unzip_cmd = 'rm -rf ' + set_name + '; tar xvzf ' + zip_file
print unzip_cmd
os.system(unzip_cmd)

msc_files = os.listdir(set_name)

for f in msc_files:
    convert_cmd = 'python convert_msc.py ' + set_name + '/' + f
    print convert_cmd
    os.system(convert_cmd)


