import sys
import os

def main():
	args = sys.argv
	if len(args) < 5 or len(args)> 6:
		print 'Usage: python run_test.py <input_prefix> <image_name> <eps> <tomog step size> [optional <angle_increment_size>]'
		sys.exit(1)
		
	img_name = args[2]
	if len(args) == 6:
		cmd_scan = 'python scan_image.py ' + args[1] +' ' + img_name +  ' ./ ' + args[4] + ' ' + args[5]
	else:
		cmd_scan = 'python scan_image.py ' + args[1]+ img_name + ' ./ ' + args[4]
	img_name_no_ext = img_name[:img_name.find('.')]
	input_file_name =args[1] + '_' + img_name_no_ext
	cmd_fastpc = '../../fastpc ' + args[3] + ' ' +input_file_name
	cmd_build = 'python build_image.py ' + img_name
	os.system(cmd_scan)
	print 'Scan completed.'
	os.system(cmd_fastpc)
	print 'fastpc completed.'
	os.system(cmd_build)
	print 'regenerate completed.'

main()
