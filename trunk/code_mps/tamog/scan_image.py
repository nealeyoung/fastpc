import Image
import math
import sys

###################################
#sizes are wrong for not squares switch them
#size[0] size[1] are switched in this code
###########################################


def gen_row(angle,x,y,step,array,region,bound ):

    distance = 0
    total_density = 0
    variable_track = []
    last_x = -1
    last_y = -1
    
    while(True):
        distance = distance + step
        cur_x = int(math.floor(distance * math.cos(angle) + x ))
        cur_y = int(bound(distance * math.sin(angle) + y))

        if cur_x >  region.size[1]-1 or cur_y > region.size[0]-1 or cur_x < 0 or cur_y <0:
            break
        total_density = array[cur_y][cur_x] + total_density
        if(cur_x == last_x and cur_y ==last_y):
            variable_track[-1][1] = variable_track[-1][1] + 1 
        else:
            variable_track.append([cur_y*region.size[1]+cur_x,1])
            last_x = cur_x
            last_y = cur_y
            

    return variable_track, total_density
    
    


im = Image.open("test2.jpg")
box = (0, 100, 80, 180)
converted = im.crop(box).convert('L')
converted.show()

print converted.size[0]
print converted.size[1]


#new.show()

data= list(converted.getdata())
array = []
for i in range(0,len(data),converted.size[0]):
    array.append(data[i:i+converted.size[0]])

#print array
#print ""


my_file = open("tom_test_small_crazy",'w')



my_string = str(converted.size[0]/5) +' ' + str(converted.size[0]*converted.size[1])
my_spaces = reduce(lambda a,b: a+b, [' ']*(len(my_string)+20))
my_file.write( my_spaces)


non_zeros = 0
rows = 0

#change angle down from left 
for i in range(0,converted.size[0],1):
    for j in range(-90,90,5):
        rows = rows + 1
        equ_array,t_size = gen_row(j*math.pi/180 , 0.0, i, 0.01, array,converted,math.floor)
        if len(equ_array)<1:
            rows =  rows -1
            continue
        for item in equ_array:
            my_file.write(str(rows-1) + ' ' + str(item[0]) + ' ' + str(float(item[1])/float(t_size)) + '\n')
            non_zeros = non_zeros + 1



#change angle down from right
for i in range(0,converted.size[0],1):
    for j in range(90,270,5):
        rows = rows + 1
        equ_array,t_size = gen_row(j*math.pi/180 , converted.size[1], i, 0.01, array,converted,math.floor)
        if len(equ_array)<1:
            rows =  rows -1
            continue
        for item in equ_array:
            my_file.write(str(rows-1) + ' ' + str(item[0]) + ' ' + str(float(item[1])/float(t_size)) + '\n')
            non_zeros = non_zeros + 1

## #change angle up from left side
## for i in range(0,converted.size[0],1):
##     for j in map(lambda a: a*-1,range(0,90,5)):
##         rows = rows + 1
##         equ_array,t_size = gen_row(j*math.pi/180 , 0.0,i, 0.01, array,converted,math.floor)
##         if len(equ_array)<1:
##             rows = rows -1
##             continue
##         for item in equ_array:
##             my_file.write(str(rows-1) + ' ' + str(item[0]) + ' ' + str(float(item[1])/float(t_size)) + '\n')
##             non_zeros = non_zeros + 1


#chang angle from top
for i in range(0,converted.size[1],1):
    for j in map(lambda a: a*-1,range(0,180,5)):
        rows = rows + 1
        equ_array,t_size = gen_row(j*math.pi/180 , i,0.0, 0.01, array,converted,math.floor)
        if len(equ_array)<1:
            rows = rows -1
            continue
        for item in equ_array:
            my_file.write(str(rows-1) + ' ' + str(item[0]) + ' ' + str(float(item[1])/float(t_size)) + '\n')
            non_zeros = non_zeros + 1


#chang angle from bottum
for i in range(0,converted.size[1],1):
    for j in map(lambda a: a*-1,range(0,-180,5)):
        rows = rows + 1
        equ_array,t_size = gen_row(j*math.pi/180 , i,converted.size[0], 0.01, array,converted,math.floor)
        if len(equ_array)<1:
            rows = rows -1
            continue
        for item in equ_array:
            my_file.write(str(rows-1) + ' ' + str(item[0]) + ' ' + str(float(item[1])/float(t_size)) + '\n')
            non_zeros = non_zeros + 1



#keep in color bounds
for var in range(converted.size[0]*converted.size[1]):
    rows = rows + 1
    my_file.write(str(rows-1) + ' ' + str(var) + ' ' + str(1.0/255.0) + '\n')
    non_zeros = non_zeros + 1
    
                  

my_file.seek(0)
my_file.write(str( rows) +' ' + str(converted.size[0]*converted.size[1])+ ' ' + str(non_zeros) + '\n')

    





                  
    
    
    
