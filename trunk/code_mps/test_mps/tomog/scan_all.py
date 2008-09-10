import os
import sys

args = sys.argv
if len(args) < 4:
    print 'Usage: python scan_all.py <output_directory> <glpk_output_directory> <step_size> [<angle_imcrement_size>]'
    sys.exit(1)

directory_of_this_script =reduce(lambda a,b: a +'/' + b,args[0].split('/')[:-1])
input_img_dir =  directory_of_this_script + '/input_images/'
img_files = os.listdir(input_img_dir)

for img_file in img_files:
    if not img_file.startswith('.'):
        if len(args) < 6:
            scan_cmd = 'python '+ directory_of_this_script + '/scan_image.py ' + img_file + ' ' + args[1] + ' ' + args[3]
        else:
            scan_cmd = 'python ' + directory_of_this_script + '/scan_image.py ' + img_file + ' ' + args[1] + ' ' + args[3] + ' ' + args[4]
        os.system(scan_cmd)
        out_file_name = '/fastpc_input_' + img_file[:img_file.find('.')]
        create_glpk_cmd = directory_of_this_script+'/../glpsol_filter < ' + args[1] + out_file_name + ' > ' + args[2] + out_file_name + '_glpk'
        os.system(create_glpk_cmd)
