import sys
import os

def main():
	args = sys.argv
	if len(args) < 4 or len(args)> 5:
		print 'Usage: python run_test.py <image_name> <eps> <tomog step size> [optional <angle_increment_size>]'
		sys.exit(1)
		
	img_name = args[1]
	img_name_no_ext = img_name[:img_name.find('.')]
	input_file_name = 'fastpc_input_' + img_name_no_ext
	cmd_scan = 'python scan_image.py ' + img_name
	if len(args) == 5:
		cmd_tomog = './tomog tomog_input_' + img_name_no_ext + ' '+ input_file_name +' ' + args[3] + ' ' + args[4]
	else:
		cmd_tomog = './tomog tomog_input_' + img_name_no_ext + ' '+ input_file_name +' ' + args[3]	
	cmd_fastpc = '../fastpc ' + args[2] + ' ' + input_file_name
	cmd_build = 'python build_image.py ' + img_name
	os.system(cmd_scan)
	print 'Scan completed.'
	os.system(cmd_tomog)
	print 'tomography completed'
	os.system(cmd_fastpc)
	print 'fastpc completed.'
	os.system(cmd_build)
	print 'regenerate completed.'

main()
