"""Build Windows x64 binaries with an existing MinGW-w64 toolchain."""
from pathlib import Path
import argparse, os, subprocess, shutil, json
parser=argparse.ArgumentParser()
parser.add_argument('--mingw-root',type=Path,required=True,help='Prefix containing usr/bin or bin')
parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args();project=Path(__file__).resolve().parents[1];out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
bin_dir=args.mingw_root.resolve()/'usr/bin'
if not bin_dir.is_dir():bin_dir=args.mingw_root.resolve()/'bin'
env=os.environ.copy();env['PATH']=str(bin_dir)+os.pathsep+env.get('PATH','')
cxx=bin_dir/'x86_64-w64-mingw32-g++-posix'
if not cxx.exists():cxx=bin_dir/'x86_64-w64-mingw32-g++'
gcc=bin_dir/'x86_64-w64-mingw32-gcc-posix'
if not gcc.exists():gcc=bin_dir/'x86_64-w64-mingw32-gcc'
def run(command):subprocess.run([str(x) for x in command],cwd=project,env=env,check=True)
resource=out/'faceplate.o'
run([bin_dir/'x86_64-w64-mingw32-windres','--preprocessor',gcc,'--preprocessor-arg=-E','--preprocessor-arg=-xc','--preprocessor-arg=-DRC_INVOKED','-I','Resources','-i','Resources/Afterglow.rc','-O','coff','-o',resource])
bundle=out/'Afterglow.vst3';binary=bundle/'Contents/x86_64-win';binary.mkdir(parents=True,exist_ok=True)
common=['-std=c++17','-O3','-Ithird_party','-DUNICODE','-D_UNICODE','-DNOMINMAX','-static','-static-libgcc','-static-libstdc++']
run([cxx,*common,'-shared','Source/Plugin.cpp','Source/Editor.cpp',resource,'-o',binary/'Afterglow.vst3','-luser32','-lgdi32','-lgdiplus','-lole32','-Wl,--no-insert-timestamp','-Wl,--dynamicbase','-Wl,--nxcompat'])
run([cxx,*common,'Tests/HostTests.cpp','-o',out/'AfterglowHostTests.exe','-luser32','-lgdi32','-Wl,--no-insert-timestamp'])
run([cxx,*common,'Tests/RealtimeTests.cpp','-o',out/'AfterglowRealtimeTests.exe','-Wl,--no-insert-timestamp'])
run([cxx,*common,'Tests/EditorTests.cpp','-o',out/'AfterglowEditorTests.exe','-Wl,--no-insert-timestamp'])
strip=bin_dir/'x86_64-w64-mingw32-strip'
if strip.exists():run([strip,'--strip-unneeded',binary/'Afterglow.vst3',out/'AfterglowHostTests.exe',out/'AfterglowRealtimeTests.exe',out/'AfterglowEditorTests.exe'])
resources=bundle/'Contents/Resources';resources.mkdir(parents=True,exist_ok=True)
shutil.copy2(project/'Resources/moduleinfo.json',resources/'moduleinfo.json')
shutil.copy2(project/'third_party/pluginterfaces/LICENSE.txt',resources/'VST3-SDK-LICENSE.txt')
print('Windows VST3:',binary/'Afterglow.vst3')
