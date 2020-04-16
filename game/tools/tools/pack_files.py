import os, sys, subprocess

script_path = os.path.dirname(os.path.realpath(__file__))

exclude_files = [ "config.cfg", "game_user_cmds.cfg" ]
exclude_folders = [ "workshop", "soundcache" ]
target_folders = [ "materials", "models", "particles", "scenes", "resource", "maps", "sound", "scripts" ]
file_types = [ "vmt", "vtf", "mdl", "phy", "vtx", "vvd", "pcf", "vcd", "res", "bsp", "nav", "cfg", "mp3", "wav", "txt" ]

vpk_path = os.path.join(os.environ["ProgramFiles(x86)"], "Steam/steamapps/common/Alien Swarm/bin/vpk.exe")
if len(sys.argv) >= 3:
	vpk_path = sys.argv[2]

mod_path = os.path.join(script_path, "..")
if len(sys.argv) >= 2:
	mod_path = sys.argv[1]

vpkfiles = [f for f in os.listdir(mod_path) if f.endswith(".vpk")]
for file in vpkfiles:
	os.remove(os.path.join(mod_path, file))

response_path = os.path.join(mod_path, "vpk_list.txt")

out = open(response_path, 'w')
len_cd = len(mod_path) + 1

for user_folder in target_folders:
	for root, dirs, files in os.walk(os.path.join(mod_path, user_folder)):
		dirs[:] = [d for d in dirs if d not in exclude_folders]
		files[:] = [f for f in files if f not in exclude_files]

		for file in files:
			if file.rsplit(".")[-1] in file_types:
				out.write(os.path.join(root[len_cd:].replace("/", "\\"), file) + "\n")

out.close()

out = subprocess.check_output([vpk_path, "-M", "a", "pak01", "@" + response_path], cwd=mod_path)
print(out)