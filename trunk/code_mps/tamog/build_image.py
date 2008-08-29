import Image
import math
my_image = Image.new("L",(100,100))



my_file = open("../tomog_solution")



im = Image.open("test2.jpg")
box = (100, 100, 200, 200)
converted = im.crop(box).convert('L')



data = []
for line in my_file:
    if int(math.floor(float(line))) < 255: data.append(int(math.floor(float(line))))
    else: data.append(255)

old_data= list(converted.getdata())

print data
total = reduce(lambda a, b: a+b, data)
total1 = reduce(lambda a, b: a+b, old_data)

print 'Total lightness in new image: ' + str(total)
print 'Total lightness in original image: ' + str(total1)

my_image.putdata(data)


my_image.save("out_converted3.bmp")
converted.save("out_old2.png")
