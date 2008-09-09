import Image
import sys

args = sys.argv
if len(args) < 2:
    print 'Usage: python build_image.py <image_name>'
    sys.exit(0)
img_name = args[1]
img_name_no_ext = img_name[:img_name.find('.')]

im = Image.open(img_name)
width = im.size[0]
height = im.size[1]

my_image = Image.new("L",(width, height))
my_file = open("fastpc_solution")

data = []
max_var = 0
for line in my_file:
    val = int(float(line))
    data.append(val)
    if val > max_var:
        max_var = val

#scale all variables if necessary
if max_var > 255:
    scale_ratio = 255.0/float(max_var)
    data = map(lambda x: int(x*scale_ratio), data)

my_image.putdata(data)
my_image.save(img_name_no_ext + "_generated.png")
