import sys
import os

# Get directory to start from
if len(sys.argv) > 1:
	root_dir = sys.argv[1]
else:
	root_dir, script_name = os.path.split(sys.argv[0])
root_dir = os.path.abspath(root_dir)

print "Root directory: " + root_dir

# Function for trimming trailing spaces
def trim_line(line):
	return line.rstrip() + "\n"

def trim_file(filename):
	f = open(filename, "r")
	lines = f.readlines()
	f.close()

	# trim trailing spaces
	lines = map(trim_line, lines)

	# remove empty lines in the end
	while not lines[-1].strip():
		lines.pop()

	f = open(filename, "w")
	f.writelines(lines)
	f.close()

# Function to call in each directory
def walk_callback(arg, dir, file_list):
	if dir.find('\\.git') != -1:
		return

	print "In directroy: " + dir
	for file in file_list:
		name, ext = os.path.splitext(file)
		if ext in ['.c', '.cpp', '.h', '.bat', '.inf',
				'.py', '.cmd', '.vbs'] or \
				file in ['sources', 'makefile']:
			trim_file(os.path.join(dir, file))

# Walk through the directory tree
os.path.walk(root_dir, walk_callback, None)
