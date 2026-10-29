#!/usr/bin/env python3
"""Resolve release artifacts and build a downstream C++ consumer for every iOS architecture."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='simgear-spm-client-') as directory:
    client=Path(directory);sources=client/'Sources/Client';sources.mkdir(parents=True)
    (sources/'main.cpp').write_text((ROOT/'apple/scripts/simgear_smoke.cpp').read_text())
    (client/'Package.swift').write_text('''// swift-tools-version: 5.9
import PackageDescription
let package = Package(name: "SimGearClient", platforms: [.iOS("18.0")],
    dependencies: [.package(path: "'''+str(ROOT)+'''")],
    targets: [.executableTarget(name: "Client", dependencies: [.product(name: "SimGear", package: "SimGear")])],
    cxxLanguageStandard: .cxx17)
''')
    subprocess.run(['swift','package','resolve'],cwd=client,check=True)
    for arch,sim in [('arm64',False),('arm64',True),('x86_64',True)]:
        sdk=subprocess.check_output(['xcrun','--sdk','iphonesimulator' if sim else 'iphoneos','--show-sdk-path'],text=True).strip()
        triple=arch+'-apple-ios18.0'+('-simulator' if sim else '')
        subprocess.run(['swift','build','--triple',triple,'--sdk',sdk,'-Xcc','-isysroot','-Xcc',sdk,'-Xcxx','-isysroot','-Xcxx',sdk,'-Xlinker','-syslibroot','-Xlinker',sdk,'-c','release'],cwd=client,check=True)
        binpath=subprocess.check_output(['swift','build','--triple',triple,'--sdk',sdk,'-c','release','--show-bin-path'],cwd=client,text=True).strip()
        binary=Path(binpath)/'Client'
        metadata=subprocess.check_output(['xcrun','vtool','-show-build',str(binary)],text=True)
        assert ('IOSSIMULATOR' if sim else 'IOS') in metadata, metadata
        print('PASS: downstream SPM C++ consumer '+triple,flush=True)
