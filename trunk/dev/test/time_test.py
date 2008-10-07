import os
import re

test_list =  ["0.01 peter_test 1"]

glpk_command = "../../../glpk-4.30/examples/glpsol --cpxlp "
glpk_list = ["glpk_peter_test"]

total_time = 0
result_string ='failed to get result string'
glpk_result = 'failed to get glpk result'

p = re.compile(r'time = [0-9]*.[0-9]*')
q =  re.compile(r'primal = [0-9]*.[0-9]* dual = [0-9]*.[0-9]* ratio = [0-9]*.[0-9]*.[0-9]*')
r = re.compile(r'[0-9]*.[0-9]* secs')
s = re.compile(r'obj =[ ]*[0-9]*.[0-9]*e[+-][0-9]*')

for index,test in enumerate(test_list):
    
    fastpc_out = os.popen("../fastpc "+test)
    print 'finished fastpc'
    for line in fastpc_out:
        print line
        result_string_temp = q.findall(line)
        if len(result_string_temp)>0:
            result_string = result_string_temp[0]

        temp_list = p.findall(line)
        if len(temp_list)>0:
            break

    
    print 'time for: fastpc ' + test
    thing = temp_list[0].split()
    print thing[-1]
    print 'result for: ' + 'fastpc ' + test
    print result_string
    total_time = float(thing[-1]) + total_time

    glpk_out = os.popen(glpk_command + glpk_list[index])
    print 'finish glpk'
    for line in glpk_out:
        print line
        glpk_result_temp = s.findall(line)
        if len(glpk_result_temp)>0 :
            glpk_result = glpk_result_temp[0]


        glpk_time = r.findall(line)
        if len(glpk_time) > 0:
            break
        
        
    print 'time for: ' + glpk_command + glpk_list[index]
    print glpk_time[0].split()[-2]
    print 'result for' + glpk_command + glpk_list[index]
    print glpk_result

    
print 'total time'
print total_time
