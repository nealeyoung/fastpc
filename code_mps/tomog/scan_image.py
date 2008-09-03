import Image
import math
import sys
import os


args = sys.argv
if len(args) < 2:
    print 'Usage: python scan_image.py <image_name>'
    sys.exit(0)

img_name = args[1]
img_name_no_ext = img_name[:img_name.find('.')]
out_file_name = 'tomog_input_' + img_name_no_ext

im = Image.open(img_name)
box = (0, 0, 95, 95)
converted = im.convert('L').crop(box)
converted.show()
converted.save(img_name_no_ext + '_converted.png')

width = converted.size[0]
height = converted.size[1]

my_file = open(out_file_name,'w')
data= list(converted.getdata())
my_file.write(str(width)+' ' + str(height) + '\n')
for item in data:
    my_file.write(str(item)+'\n')

my_file.close()
