import math

vec = [2,4,6,8]
e_vec = [math.e**x for x in vec]
vec2 = [math.log(x/2) for x in e_vec]

for i in range(10):

    vec[0] = vec[0] + 1
    vec2[0] = vec2[0] + 1

    


e_vec = [math.e**x for x in vec]
e_vec2 = [math.e**x for x in vec2]

print [ x/sum(e_vec) for x in e_vec]
print [ x/sum(e_vec2) for x in e_vec2]
