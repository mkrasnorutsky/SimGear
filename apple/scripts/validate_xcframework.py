#!/usr/bin/env python3
"""Compile and link representative Core/Scene APIs with all released dependencies."""
from pathlib import Path
import argparse,plistlib,subprocess,tempfile,os
from dependencies import ROOT,fetch,select
SYSTEM_FRAMEWORKS=['Foundation','UIKit','ImageIO','CoreGraphics','Accelerate','MobileCoreServices','Security','CoreFoundation','CoreServices','OpenAL']
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--run-simulator',action='store_true');a=p.parse_args()
    artifact=ROOT/'dist/SimGear.xcframework';info=plistlib.loads((artifact/'Info.plist').read_bytes());assert len(info['AvailableLibraries'])==2
    packages=fetch()
    for entry in info['AvailableLibraries']:
        sim=entry.get('SupportedPlatformVariant')=='simulator'
        assert entry['SupportedPlatform']=='ios'
        assert set(entry['SupportedArchitectures'])==({'arm64','x86_64'} if sim else {'arm64'})
        folder=artifact/entry['LibraryIdentifier'];archive=folder/entry['LibraryPath'];headers=folder/entry['HeadersPath']
        subprocess.run(['lipo',str(archive),'-verify_arch',*entry['SupportedArchitectures']],check=True)
        include=['-I'+str(headers),'-I'+str(ROOT/'apple/SPM/include'),'-I'+str(ROOT/'apple/validation/osg')]
        libraries=[];frameworks=[]
        for name in ['OpenSceneGraph','FreeType','Boost','Curl','CAres','LZMA']:
            depfolder,depentry=select(packages[name],sim);include+=['-I'+str(depfolder/depentry['HeadersPath'])];libraries+=[str(depfolder/depentry['LibraryPath'])]
        for name in ['libEGL','libGLESv2']:
            depfolder,_=select(packages[name],sim);frameworks+=['-F'+str(depfolder),'-Wl,-rpath,'+str(depfolder)]
        sdk=subprocess.check_output(['xcrun','--sdk','iphonesimulator' if sim else 'iphoneos','--show-sdk-path'],text=True).strip()
        for arch in entry['SupportedArchitectures']:
            triple=arch+'-apple-ios18.0'+('-simulator' if sim else '')
            with tempfile.TemporaryDirectory(prefix='simgear-client-') as directory:
                binary=Path(directory)/'client'
                subprocess.run(['xcrun','clang++','-target',triple,'-isysroot',sdk,'-std=c++17','-DOSG_LIBRARY_STATIC','-DOPENTHREADS_LIBRARY_STATIC',*include,*frameworks,
                    str(ROOT/'apple/scripts/simgear_smoke.cpp'),str(ROOT/'apple/SPM/SimGearSPM.cpp'),str(ROOT/'apple/validation/osg/OpenSceneGraphSPM.cpp'),str(archive),*libraries,
                    '-framework','libGLESv2','-framework','libEGL','-lz','-lresolv',*[item for f in SYSTEM_FRAMEWORKS for item in ['-framework',f]],'-o',str(binary)],check=True)
                print('PASS: SimGear Core/Scene and dependency link '+triple,flush=True)
                if a.run_simulator and sim and arch==os.uname().machine:
                    subprocess.run(['xcrun','simctl','spawn','booted',str(binary)],check=True)
if __name__=='__main__':main()
