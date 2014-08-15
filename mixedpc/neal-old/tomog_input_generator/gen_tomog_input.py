#!/usr/bin/env python

import sys, os, math
from PIL import Image
import Polygon, Polygon.IO

def create_pix_rect(i, j):
    return Polygon.Polygon(((i,j), (i+1, j), (i+1, j+1), (i, j+1)))

def variable(i, j, h):
#    print 'Find variable index for [', i, ',', j, '] : ',  w*i + j
    return h*i + j

def variable_index(v, h):
    i = math.floor(v/h)
    j = v - h*i
    return (i, j) 

def floor(num):
    return int(math.floor(num))

def sqrt(num):
    return math.sqrt(num)

def create_ray_rect(x, y, grid_width, line_width):
    return Polygon.Polygon(((x,y), (x, y+grid_width), (x+line_width, y+grid_width), (x+line_width, y)))

def create_ray_grid(w, h, line_width, M):
    grid_width = w + h
    ray_separation = max(w,h) * 1.0 / M
    center = (w/2, h/2)
    x = -h/2
    y = -w/2
    grid = []
    x_max = w + h/2
    while x < x_max:
        grid.append(create_ray_rect(x, y, grid_width, line_width))
        x += ray_separation
    return grid

def rotate_ray_grid(ray_grid, angle, center_x, center_y):
    for rect in ray_grid:
        rect.rotate(angle, center_x, center_y)

def float_str(value):
    if 'e' in str(value):
        return '{0:10.15f}'.format(value)
    else:
        return str(value)

def generate_inputs(image_name, img_dir, mixedpc_dir, glpk_dir):
    img = Image.open(img_dir + image_name)
    img = img.convert('L')
    img.save(img_dir + 'L_' + image_name[:image_name.rfind('.')] + '.png')
    (w, h) = img.size
    pixels = img.load()

    print '    Image size: ', w, 'x', h, ' pixels'

    # File name for writing actual solution
    actual_sol_file_name = img_dir + image_name[:image_name.rfind('.')] + '_actual_pixel_values'
    actual_sol_file = open(actual_sol_file_name, 'w')

    # rectangle representing the border of the image
    # before we find pixels that are hit by the ray, 
    # we check if ray intersects with this rectangle
    image_rect = Polygon.Polygon(((0,0), (w,0), (w,h), (0,h)))

    rects = []
    for row in range(w):
        rect_row = []
        for col in range(h):
            rect_row.append(create_pix_rect(row, col))
            actual_sol_file.write(str(pixels[(row, col)]) + ' ')
        rects.append(rect_row)

    actual_sol_file.close()

    # Number of variables in LP
    m = w * h

    # Extra variable needed in case there are some constraints with rhs = 0
    m_actual = m
    extra_var = str(variable(w+1, 0, h))

    # Total number of constraints in LP
    n = 2*m

    # Number of snapshots
    N = int(sqrt(n))

    # Number of constraints per snapshot
    M = N

    # width of rays going through the image
    line_width = 0.2

    # maximum angle for moving the source
    max_angle = math.pi

    # step size to get n constrainsts
    step = max_angle/N

    ray_grid = create_ray_grid(w, h, line_width, M)

    # to draw the rectangles for testing
    draw_rects = False
    if draw_rects:
        rects_flattened = sum(rects, [])
        rects_flattened.append(ray_grid)
        Polygon.IO.writeGnuplot('gnuplot_input', rects_flattened)
        exit()

    # File name for writing LP
    out_file_name = mixedpc_dir + image_name[:image_name.rfind('.')] + '_input'
    glpk_file_name = glpk_dir + image_name[:image_name.rfind('.')] + '_input'

    out_file = open(out_file_name, 'w')
    glpk_file = open(glpk_file_name, 'w')

    first_line = str(n) + '  ' + str(n) + '  ' + str(m) + '  ' + str(n*m) + ' \n'
    out_file.write(first_line)

    glpk_file.write('Maximize \n')
    glpk_file.write('value: ')
    for j in range(m_actual):
        glpk_file.write('+ x' + str(j) + ' ')
    glpk_file.write('\n')
    glpk_file.write('subject to\n')

    # start generating constraints
    # keep track of actual entry count
    entry_count = 0
    n_actual = 0
    m_actual = 0
    snapshots = 1

    while snapshots < N:
        for ray_rect in ray_grid:
            # generate constraint for ray
            entries = set()
            rhs = 0
            image_intersection = ray_rect & image_rect
            if image_intersection:
                total_area = image_intersection.area()
                area = 0
                for row in range(w):
                    if area >= total_area:
                        break
                    for col in range(h):
                        pixel_intersection = ray_rect & rects[row][col]
                        if pixel_intersection:
                            area += pixel_intersection.area()
                            rhs += pixels[(row, col)] * pixel_intersection.area()
                            var = variable(row, col, h)
                            if var >= m:
                                print 'Error: Maximum variable allowed is ', m, ', found ', var
                            entries.add((str(var), pixel_intersection.area()))
                            if (area >= total_area):
                                break

            if len(entries) > 0:
                _val = 0
                for entry in entries: # loop to recompute rhs
                    (_r, _c) = variable_index(int(entry[0]), h)
                    _val += entry[1] * pixels[(_r, _c)]

                entry_count += 2 * len(entries)

                n_actual_str = str(n_actual)
                glpk_str = ''
                if rhs == 0:
                    out_file.write('1 ' + n_actual_str + ' ' + extra_var + ' 1\n')
                    out_file.write('2 ' + n_actual_str + ' ' + extra_var + ' 1\n')
                    glpk_str += '+ 1 x' + extra_var + ' '
                    rhs = 1
                    _val = 1
                    m_actual = m + 1

                eps = 0.00001
                error_shift = 10*abs(rhs - _val)
                error_shift = max(eps, error_shift)
                if rhs > eps:
                    rhs_p = rhs + error_shift
                    rhs_c = rhs - error_shift
                else:
                    rhs_p = rhs + 2*error_shift
                    rhs_c = rhs

                for entry in entries:
                    out_file.write('1 ' + n_actual_str + ' ' + entry[0] + ' ' + float_str(entry[1] / rhs_p) + '\n')
                    out_file.write('2 ' + n_actual_str + ' ' + entry[0] + ' ' + float_str(entry[1] / rhs_c) + '\n')
                    glpk_str += '+ ' + float_str(entry[1]) + ' x' + entry[0] + ' '

                glpk_file.write('con' + n_actual_str + '_1: ' + glpk_str + '<= ' + float_str(rhs_p) + ' \n')
                glpk_file.write('con' + n_actual_str + '_2: ' + glpk_str + '>= ' + float_str(rhs_c) + ' \n')

                n_actual += 1 # this is the index of the constraint

        snapshots += 1
        rotate_ray_grid(ray_grid, step, w/2, h/2)

    glpk_file.write('Bounds \n')
    for j in range(m):
        glpk_file.write('0 <= x' + str(j) + ' <= 255 \n')
    if m_actual > m:
        glpk_file.write('1 <= x' + str(m) + ' <= 1 \n')

    glpk_file.write('end')

    # change the first line to have the actual non-zero count
    out_file.seek(0)
    first_line_actual = str(n_actual) + '  ' + str(n_actual) + '  ' + str(m_actual) + '  ' + str(entry_count)
    diff_digits = len(first_line) - len(first_line_actual) - 1
#    print diff_digits, len(str(n*m)), len(str(entry_count))
    blank = ''
    if diff_digits >= 0:
        for i in range(diff_digits):
            blank += ' '
    else:
        print n*m
        print entry_count
        print 'Error in line 1. Use bigger string in the beginning.'
    out_file.write(first_line_actual + blank)

    out_file.close()
    glpk_file.close()

def main():
    args = sys.argv
    if len(args) < 5:
        print 'Usage: python gen_tomog_input.py <run_identifier> <img_dir> <mixedpc_dir> <glpk_dir>'
        return
    run_identifier = args[1]
    img_dir = args[2]
    mixedpc_dir = args[3]
    glpk_dir = args[4]

    img_files = os.listdir(img_dir)
    for image_name in img_files:
        if image_name.startswith(run_identifier) and not image_name.endswith('pixel_values'):
            print '  Generating input for ', img_dir + image_name
            generate_inputs(image_name, img_dir, mixedpc_dir, glpk_dir)
    print '  Done generating inputs for ', run_identifier

main()
