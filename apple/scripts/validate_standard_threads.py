#!/usr/bin/env python3
"""Exercise MIT thread wrappers under host sanitizers and link every iOS slice."""
# SPDX-License-Identifier: MIT
import argparse, os, plistlib, subprocess, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
WORK = Path(os.environ.get("APPLE_BUILD_ROOT", str(ROOT))).expanduser().resolve()
def run(*args):
    subprocess.run(list(map(str, args)), check=True, timeout=120)
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact', type=Path)
    parser.add_argument('--run-simulator', action='store_true')
    args = parser.parse_args()
    test = ROOT/'apple/validation/standard_threads.cpp'
    with tempfile.TemporaryDirectory(prefix='standard-threads-') as folder:
        binary = Path(folder)/'test'
        for sanitizer in ['address,undefined', 'thread']:
            run('clang++', '-std=c++17', '-pthread', '-Wall', '-Wextra', '-Werror',
                '-g', '-fsanitize='+sanitizer, '-I'+str(ROOT), test,
                ROOT/'simgear/threads/SGThread.cxx', '-o', binary)
            run(binary)
        if args.artifact:
            artifact = args.artifact.resolve()
            info = plistlib.loads((artifact/'Info.plist').read_bytes())
            for entry in info['AvailableLibraries']:
                sim = entry.get('SupportedPlatformVariant') == 'simulator'
                sdk = subprocess.check_output(['xcrun', '--sdk', 'iphonesimulator' if sim else 'iphoneos', '--show-sdk-path'], text=True).strip()
                slice = artifact/entry['LibraryIdentifier']
                assert 'SPDX-License-Identifier: MIT' in (slice/entry['HeadersPath']/'simgear/threads/SGThread.hxx').read_text()
                for arch in entry['SupportedArchitectures']:
                    triple = arch+'-apple-ios18.0'+('-simulator' if sim else '')
                    run('xcrun', 'clang++', '-std=c++17', '-target', triple, '-isysroot', sdk,
                        '-I'+str(slice/entry['HeadersPath']), test, slice/entry['LibraryPath'], '-o', binary)
                    print('PASS thread API artifact link', triple, flush=True)
                    if args.run_simulator and sim and arch == os.uname().machine:
                        run('xcrun', 'simctl', 'spawn', 'booted', binary)
if __name__ == '__main__': main()
