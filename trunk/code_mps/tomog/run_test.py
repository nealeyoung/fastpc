import sys
import os

def main():
	args = sys.argv
	if len(args) < 2:
		print 'Usage: python run_test.py <image_name>'
		sys.exit(0)
	img_name = args[1]
	img_name_no_ext = img_name[:img_name.find('.')]
	input_file_name = 'fastpc_input_' + img_name_no_ext
	cmd_scan = 'python scan_image.py ' + img_name
	cmd_fastpc = '../fastpc .05 ' + input_file_name
	cmd_build = 'python build_image.py ' + img_name
	os.system(cmd_scan)
	print 'Scan completed.'
	os.system(cmd_fastpc)
	print 'fastpc completed.'
	os.system(cmd_build)
	print 'regenerate completed.'

main()
