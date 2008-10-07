import os
import sys

args = sys.argv
if len(args) < 5:
    print 'Usage: python scan_all.py <input_prefix> <output_directory> <glpk_output_directory> <step_size> [<angle_imcrement_size>]'
    sys.exit(1)

directory_of_this_script =reduce(lambda a,b: a +'/' + b,args[0].split('/')[:-1])
input_img_dir =  directory_of_this_script + '/input_images/'
img_files = os.listdir(input_img_dir)

for img_file in img_files:
    if not img_file.startswith('.'):
        if len(args) < 6:
            scan_cmd = 'python '+ directory_of_this_script + '/scan_image.py ' + args[1] + ' ' + img_file + ' ' + args[2] + ' ' + args[4] + ' ' + args[1]
        else:
            scan_cmd = 'python ' + directory_of_this_script + '/scan_image.py ' + args[1] + ' ' + img_file + ' ' + args[2] + ' ' + args[4] + ' ' + args[5]
        os.system(scan_cmd)
        out_file_name = '/'+args[1]+'_' + img_file[:img_file.find('.')]
        create_glpk_cmd = directory_of_this_script+'/../glpsol_filter < ' + args[2] + out_file_name + ' > ' + args[3] + out_file_name + '_glpk'
        os.system(create_glpk_cmd)
