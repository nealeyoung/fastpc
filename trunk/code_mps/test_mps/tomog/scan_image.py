import Image
import math
import sys
import os


args = sys.argv
if len(args) < 5:
    print 'Usage: python scan_image.py <input_file_prefix> <image_name> <output_location> <tomog step size> [optional <angle_increment_size>]'
    sys.exit(0)

directory_of_this_script =reduce(lambda a,b: a +'/'+ b,args[0].split('/')[:-1]+['.'])

input_img_dir = directory_of_this_script + '/input_images/'
output_img_dir = directory_of_this_script+ '/output_images/'
img_name = args[2]
img_name_no_ext = img_name[:img_name.find('.')]
out_file_name = directory_of_this_script+ '/tomog_input_' + img_name_no_ext

im = Image.open(input_img_dir + img_name)
converted = im.convert('L')
converted.show()
converted.save(output_img_dir + img_name_no_ext + '_converted.png')

width = converted.size[0]
height = converted.size[1]

my_file = open(out_file_name,'w')
data= list(converted.getdata())
my_file.write(str(width)+' ' + str(height) + '\n')
for item in data:
    my_file.write(str(item)+'\n')

my_file.close()

input_file_name = args[3] + '/' + args[1] + '_' + img_name_no_ext

if len(args) < 6:
    cmd_tomog = directory_of_this_script+'/tomog ' + out_file_name + ' '+ input_file_name +' ' + args[4]
else:
    cmd_tomog = directory_of_this_script+'/tomog ' + out_file_name + ' '+ input_file_name +' ' + args[4] + ' ' + args[5]

os.system(cmd_tomog)
