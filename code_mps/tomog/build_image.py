import Image
import math
import sys

args = sys.argv
if len(args) < 2:
    print 'Usage: python run_test.py <image_name>'
    sys.exit(0)
img_name = args[1]
img_name_no_ext = img_name[:img_name.find('.')]

my_image = Image.new("L",(80,80))
my_file = open("tomog_solution")

data = []
for line in my_file:
    if int(math.floor(float(line))) < 255: data.append(int(math.floor(float(line))))
    else: data.append(255)

my_image.putdata(data)
my_image.save(img_name_no_ext + "_generated.png")
