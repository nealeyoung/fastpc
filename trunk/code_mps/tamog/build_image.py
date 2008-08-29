import Image
import math

my_image = Image.new("L",(95,95))
my_file = open("../tomog_solution")

data = []
for line in my_file:
    if int(math.floor(float(line))) < 255: data.append(int(math.floor(float(line))))
    else: data.append(255)

old_data= list(converted.getdata())

#print data
total = reduce(lambda a, b: a+b, data)
total1 = reduce(lambda a, b: a+b, old_data)

print 'Total lightness in new image: ' + str(total)
print 'Total lightness in original image: ' + str(total1)

my_image.putdata(data)

my_image.save("out_3dudes.png")
#converted.save("out_old2.png")
