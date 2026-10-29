#!/usr/bin/env python3
"""Build full static SimGear using checksum-pinned published iOS dependencies."""
from pathlib import Path
import argparse,json,shutil,subprocess
from dependencies import ROOT,fetch,select
import os
WORK=Path(os.environ.get("APPLE_BUILD_ROOT", str(ROOT))).expanduser().resolve()
OPTIONS=['-DSIMGEAR_SHARED=OFF','-DSIMGEAR_HEADLESS=OFF','-DENABLE_TESTS=OFF','-DENABLE_PKGUTIL=OFF','-DENABLE_SOUND=ON','-DUSE_AEONWAVE=OFF','-DUSE_OPENALSOFT=OFF','-DENABLE_CYCLONE=OFF','-DENABLE_RTI=OFF','-DENABLE_GDAL=OFF','-DENABLE_VIDEO_RECORD=OFF','-DSYSTEM_EXPAT=OFF']
def run(*args):
    print('+',*map(str,args),flush=True);subprocess.run(list(map(str,args)),check=True)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--jobs',type=int,default=6);a=parser.parse_args()
    deps=fetch();built={}
    variants=[('ios-arm64','iphoneos','arm64'),('simulator-arm64','iphonesimulator','arm64'),('simulator-x86_64','iphonesimulator','x86_64')]
    for label,sdk,arch in variants:
        sim=sdk=='iphonesimulator';build=WORK/'out/apple'/label;prefix=build/'install';compat=build/'angle';frameworks=compat/'out/iOS';frameworks.mkdir(parents=True,exist_ok=True)
        for name in ['libEGL','libGLESv2']:
            folder,entry=select(deps[name],sim);source=folder/entry['LibraryPath'];dest=frameworks/source.name
            if not dest.exists():shutil.copytree(source,dest)
            for group in (['EGL','KHR'] if name=='libEGL' else ['GLES2','GLES3']):
                headers=compat/'include'/group;headers.mkdir(parents=True,exist_ok=True)
                for header in (source/'Headers').glob('*.h'):shutil.copy2(header,headers/header.name)
        def dep(name):
            folder,entry=select(deps[name],sim);return folder/entry['HeadersPath'],folder/entry['LibraryPath']
        osginclude,osglib=dep('OpenSceneGraph');boostinclude,_=dep('Boost');curlinclude,curllib=dep('Curl');caresinclude,careslib=dep('CAres');lzmainclude,lzmalib=dep('LZMA')
        # c-ares uses CMake config discovery; provide a small adapter for its binary slice.
        config=build/'cmake/c-ares';config.mkdir(parents=True,exist_ok=True)
        (config/'c-ares-config.cmake').write_text('add_library(c-ares::cares STATIC IMPORTED)\nset_target_properties(c-ares::cares PROPERTIES IMPORTED_LOCATION "'+str(careslib)+'" INTERFACE_INCLUDE_DIRECTORIES "'+str(caresinclude)+'")\nset(c-ares_FOUND TRUE)\n')
        (config/'c-ares-config-version.cmake').write_text('set(PACKAGE_VERSION "1.34.5")\nif(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)\n set(PACKAGE_VERSION_COMPATIBLE FALSE)\nelse()\n set(PACKAGE_VERSION_COMPATIBLE TRUE)\nendif()\n')
        flags=OPTIONS+['-DCMAKE_SYSTEM_NAME=iOS','-DCMAKE_OSX_SYSROOT='+sdk,'-DCMAKE_OSX_ARCHITECTURES='+arch,'-DCMAKE_OSX_DEPLOYMENT_TARGET=18.0','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_INSTALL_PREFIX='+str(prefix),'-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH','-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH','-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH',
            '-DSIMGEAR_ANGLE_ROOT='+str(compat),'-DSIMGEAR_SPM_OSG_LIBRARY='+str(osglib),'-DSIMGEAR_SPM_OSG_INCLUDE='+str(osginclude),'-DBoost_NO_BOOST_CMAKE=ON','-DBoost_INCLUDE_DIR='+str(boostinclude),'-DCURL_INCLUDE_DIR='+str(curlinclude),'-DCURL_LIBRARY='+str(curllib),'-DCURL_NO_CURL_CMAKE=ON','-DLIBLZMA_INCLUDE_DIR='+str(lzmainclude),'-DLIBLZMA_LIBRARY='+str(lzmalib),'-Dc-ares_DIR='+str(config),'-DCMAKE_C_FLAGS=-F'+str(frameworks),'-DCMAKE_CXX_FLAGS=-F'+str(frameworks)+' -DOSG_LIBRARY_STATIC -DOPENTHREADS_LIBRARY_STATIC']
        run('cmake','-S',ROOT,'-B',build,*flags)
        run('cmake','--build',build,'--parallel',a.jobs)
        run('cmake','--install',build)
        archives=[prefix/'lib'/name for name in ['libSimGearCore.a','libSimGearScene.a','libFGExpat.a']]
        assert all(p.exists() for p in archives),archives
        merged=build/'libSimGear.a';run('xcrun','libtool','-static','-o',merged,*archives)
        built[label]={'prefix':prefix,'library':merged}
    out=WORK/'dist';out.mkdir(exist_ok=True);stage=out/'staging'
    if stage.exists():shutil.rmtree(stage)
    stage.mkdir();sim=stage/'libSimGear.a';run('lipo','-create',built['simulator-arm64']['library'],built['simulator-x86_64']['library'],'-output',sim)
    # Preserve generated configuration headers for each architecture.
    def headers(label):
        result=stage/('Headers-'+label);shutil.copytree(built[label]['prefix']/'include',result)
        (result/'CSimGear.h').write_text('#pragma once\n/* C++ API is available via simgear/ headers. */\n')
        (result/'CSimGear').mkdir(exist_ok=True)
        (result/'CSimGear/module.modulemap').write_text('module CSimGear { header "../CSimGear.h" export * }\n')
        return result
    deviceheaders=headers('ios-arm64');simheaders=headers('simulator-arm64')
    other=built['simulator-x86_64']['prefix']/'include'
    for path in other.rglob('*'):
        if path.is_file():
            relative=path.relative_to(other);target=simheaders/relative
            if target.exists() and target.read_bytes()!=path.read_bytes():
                assert relative==Path('simgear/simgear_config.h'), 'Unexpected architecture-dependent header: '+str(relative)
                target.write_text('#if defined(__x86_64__)\n'+path.read_text()+'\n#else\n'+target.read_text()+'\n#endif\n')
    artifact=out/'SimGear.xcframework'
    if artifact.exists():shutil.rmtree(artifact)
    run('xcodebuild','-create-xcframework','-library',built['ios-arm64']['library'],'-headers',deviceheaders,'-library',sim,'-headers',simheaders,'-output',artifact)
    licenses=artifact/'Licenses';licenses.mkdir();shutil.copy2(ROOT/'COPYING',licenses/'COPYING')
    for part in ['expat','tiny_gltf','utf8']:
        directory=ROOT/'3rdparty'/part
        for p in directory.rglob('*'):
            if p.is_file() and (any(word in p.name.upper() for word in ['COPYING','LICENSE'])):
                target=licenses/part/p.relative_to(directory);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
    for name in ['tiny_gltf.h','json.hpp']:
        source=ROOT/'3rdparty/tiny_gltf'/name
        notice=source.read_text().split('#',1)[0]
        target=licenses/'tiny_gltf'/(name+'.notice.txt');target.parent.mkdir(parents=True,exist_ok=True);target.write_text(notice)
    (out/'build-manifest.json').write_text(json.dumps({'source_revision':subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip(),'upstream_revision':'abe3e7c96e08fe1a28941af923e450f566620ab6','minimum_ios':'18.0','linkage':'static','slices':[v[0] for v in variants],'archives':['SimGearCore','SimGearScene','FGExpat'],'options':OPTIONS,'dependencies':json.loads((ROOT/'apple/dependencies.json').read_text())},indent=2)+'\n')
if __name__=='__main__':main()
