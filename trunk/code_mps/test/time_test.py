import os
import re

test_list =  [("0.1","test_lp_input_2","1"), ("0.1","lp_test_sort_3","1"), ("0.01","test_lp_input_1","1"),("0.1","test_lp_input_2","2"), ("0.01","lp_test_sort_3","2")]

total_time = 0
result_string ='failed to get result string'
p = re.compile(r'time = [0-9]*.[0-9]*')
q =  re.compile(r'primal = [0-9]*.[0-9]* dual = [0-9]*.[0-9]* ratio = [0-9]*.[0-9]*.[0-9]*')

for test in test_list:

    std_in, std_out = os.popen2(["../fastpc",test[0],test[1],test[2]])

    for line in std_out:

        result_string_temp = q.findall(line)
        if len(result_string_temp)>0:
            result_string = result_string_temp[0]

        temp_list = p.findall(line)
        if len(temp_list)>0:
            break
        
    if len(temp_list)>0:
        print 'time for: ' + reduce(lambda a,b: a+' ' +b,test) 
        thing = temp_list[0].split()
        print thing[-1]
        print 'result for: ' + 'fastpc ' + reduce(lambda a,b: a+' ' +b,test) 
        print result_string
        total_time = float(thing[-1]) + total_time

print 'total time'
print total_time
